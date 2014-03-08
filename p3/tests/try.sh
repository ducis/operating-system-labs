echo =*=*= try Task 1:
ps
./t0 sample0
sleep 2
echo =*=*= h0 opens sample0:
./h0 sample0 &
ps && ./t0 sample0 && sleep 10
echo =*=*= another h0 opens sample0:
./h0 sample0 &
ps && ./t0 sample0 && sleep 10
echo =*=*= yet another h0 opens sample0:
./h0 sample0 &
ps && ./t0 sample0 && sleep 10
echo =*=*= try Task 2:
echo =*=*= sample1: && ./t0 sample1 && sleep 5
echo =*=*= sample2: && ./t0 sample2 && sleep 5
echo =*=*= sample3: && ./t0 sample3 && sleep 5
echo =*=*= sample4: && ./t0 sample4 && sleep 5
echo =*=*= sample5: && ./t0 sample5 && sleep 5
echo =*=*= sample6: && ./t0 sample6 && sleep 5
echo =*=*= sample7: && ./t0 sample7 && sleep 5
echo =*=*= sample8: && ./t0 sample8 && sleep 5
echo =*=*= sample9: && ./t0 sample9 && sleep 5

