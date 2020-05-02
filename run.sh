waf_path="../waf"
udpRateMbpsList=("0.5" "1" "2")

# export "NS_LOG=Exercise4=level_info"
for udpRateMbps in ${udpRateMbpsList[@]}; do
    ${waf_path} --run "Exercise4 --udpRateMbps=${udpRateMbps}" | grep "^[0-9]" > "Exercise4Log-${udpRateMbps}.dat"
done