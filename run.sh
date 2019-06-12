#!/bin/bash

if [ $# -eq 3 ]
then
	n=$1
	alg=$2
	video=$video

else
	echo -n "how many times do you wish to run? "
	read n
	echo -n "whats the algorithm used? "
	read alg
	echo -n "Video to be used: "
	read video

fi

case $video in 
"container")
  video_id=2;;
"highway")
  video_id=1;;
"highway600")
  video_id=3;;
"akiyo")
  video_id=4;;
"masha")
  video_id=7;;
"despacito")
  video_id=9;;
"babyshark")
  video_id=8;;
*)
    exit 1
esac

echo $video_id
exit 1

sed -i "s/#define video [0-9]/#define video $video_id/g" scratch/v2x_3gpp_small.cc

./waf configure
mkdir ${alg}
for i in $(seq 1 $n)
do
    time ./waf --run "scratch/v2x_3gpp_small --handoverAlg=$alg --seedValue=$i" > log_${alg}_${i} 2>&1
    mkdir ${alg}/simul${i}
    mv v2x_temp/* log_${alg}_${i} $alg/simul$i
done
