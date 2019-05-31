echo -n "how many times do you wish to run? "
read n

echo -n "whats the algorithm used? "
read alg

mkdir ${alg}
for i in $(seq 1 $n)
do
    time ./waf --run "scratch/v2x_3gpp_small --handoverAlg=$alg --seedValue=$i" > log_${alg}_${i} 2>&1
    mkdir ${alg}/simul${i}
    mv v2x_temp/* $alg/simul$i
done
