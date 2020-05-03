#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Exercise4");

static void CwndChange(uint32_t oldCwnd, uint32_t newCwnd) {
    std::cout << Simulator::Now().GetSeconds() << "\t" << newCwnd << std::endl;
}

int main(int argc, char *argv[]) {
    double udpRateMbps = 0.5; // UDP source rate in Mb/s, default: 0.5 Mb/s

    CommandLine cmd;
    cmd.AddValue("udpRateMbps", "Datarate of UDP source in bps", udpRateMbps);
    cmd.Parse(argc, argv);

    int udpRate = udpRateMbps * 1000 * 1000; // UDP source rate in b/s

    // Create nodes
    NS_LOG_INFO("Create nodes.");
    Ptr<Node> nSrc1 = CreateObject<Node>();
    Ptr<Node> nSrc2 = CreateObject<Node>();
    Ptr<Node> nRtr = CreateObject<Node>();
    Ptr<Node> nDst = CreateObject<Node>();

    NodeContainer nodes = NodeContainer(nSrc1, nSrc2, nRtr, nDst);

    NodeContainer nSrc1nRtr = NodeContainer(nSrc1, nRtr);
    NodeContainer nSrc2nRtr = NodeContainer(nSrc2, nRtr);
    NodeContainer nRtrnDst  = NodeContainer(nRtr, nDst);

    // Create P2P channels
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer dSrc1dRtr = p2p.Install(nSrc1nRtr);
    NetDeviceContainer dSrc2dRtr = p2p.Install(nSrc2nRtr);
    NetDeviceContainer dRtrdDst  = p2p.Install(nRtrnDst);

    InternetStackHelper stack;
    stack.Install(nodes);

    // Add IP addresses
    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer iSrc1iRtr = ipv4.Assign(dSrc1dRtr);
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer iSrc2iRtr = ipv4.Assign(dSrc2dRtr);
    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer iRtriDst = ipv4.Assign(dRtrdDst);

    // Set up the routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Implement TCP & UDP sinks to the destinations
    uint16_t sinkPortTcp = 8080;
    uint16_t sinkPortUdp = 9090;
    Address sinkAddressTcp(InetSocketAddress(iRtriDst.GetAddress(1), sinkPortTcp));
    Address sinkAddressUdp(InetSocketAddress(iRtriDst.GetAddress(1), sinkPortUdp));

    // Install packet sinks to the destinations
    PacketSinkHelper packetSinkHelperTcp("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPortTcp));
    ApplicationContainer sinkAppTcp = packetSinkHelperTcp.Install(nDst);
    PacketSinkHelper packetSinkHelperUdp("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPortUdp));
    ApplicationContainer sinkAppUdp = packetSinkHelperUdp.Install(nDst);

    sinkAppTcp.Start(Seconds(0));
    sinkAppTcp.Stop(Seconds(30));
    sinkAppUdp.Start(Seconds(0));
    sinkAppUdp.Stop(Seconds(30));

    // Implement TCP application
    OnOffHelper onoffTcp("ns3::TcpSocketFactory", sinkAddressTcp);
    onoffTcp.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoffTcp.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoffTcp.SetAttribute("DataRate", DataRateValue(500000));
    ApplicationContainer sourceAppTcp = onoffTcp.Install(nSrc1);
    sourceAppTcp.Start(Seconds(5));
    sourceAppTcp.Stop(Seconds(20));

    //Connect the trace source and the trace sink
    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(nSrc1, TcpSocketFactory::GetTypeId());
    ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&CwndChange));
    nSrc1->GetApplication(0)->GetObject<OnOffApplication>()->SetSocket(ns3TcpSocket);

    // Implement UDP application
    OnOffHelper onoffUdp("ns3::UdpSocketFactory", sinkAddressUdp);
    onoffUdp.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoffUdp.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoffUdp.SetAttribute("DataRate", DataRateValue(udpRate));
    ApplicationContainer sourceAppUdp = onoffUdp.Install(nSrc2);
    sourceAppUdp.Start(Seconds(1));
    sourceAppUdp.Stop(Seconds(30));

    Simulator::Run();
    Simulator::Stop(Seconds(30));

    Simulator::Destroy();

    return 0;
}
