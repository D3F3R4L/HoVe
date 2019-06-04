#!/bin/bash


#--------------------------------------------
echo -n "how many times do you wish to run? "
read n
echo -n "whats the algorithm used? "
read alg
echo -n "Video to be used: "
read video

#-------------------------------------------
if [ "$video" == "container" ]
then
  video_id=2

elif [ $video == "highway" ]
then
  video_id=1

elif [ $video == "highway600" ]
then
  video_id=3

elif [ $video == "akiyo" ]
then
  video_id=4

elif [ $video == "masha" ]
then
  video_id=7

elif [ $video == "despacito" ]
then
  video_id=9

elif [ $video == "babyshark" ]
then
  video_id=8

else
    exit 1
fi

#-------------------------------------------------------
sed -i "s/#define video [0-9]/#define video $video_id/g" scratch/v2x_3gpp_small.cc


#------------------------------------------------------
./waf configure
mkdir ${alg}
for i in $(seq 1 $n)
do
    time ./waf --run "scratch/v2x_3gpp_small --handoverAlg=$alg --seedValue=$i" > log_${alg}_${i} 2>&1
    mkdir ${alg}/simul${i}
    mv v2x_temp/* log_${alg}_${i} $alg/simul$i
done
