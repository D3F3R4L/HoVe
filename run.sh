#!/bin/bash

#default values
n=1
video="highway"
ue_n=60
enb_n=100

# if the directory is named after the algorthm used, 
#use the name as input unless explicitly changed
dir=${PWD##*/}
if [ $dir ==  "hove" ] ||
   [ $dir ==  "skip" ] ||
   [ $dir ==  "ser"  ] ||
   [ $dir ==  "a2a4" ] ||
   [ $dir ==  "a3" ]
then
    alg=$dir
else
    alg="hove"
fi

# check if arguments were passed
if [ $# -gt 0 ]
then
  # read command line arguments
  for i in "$@"
  do
    case $i in
        -n=*|--simul-number=*)
        n="${i#*=}"
        shift
        ;;
        -alg=*|--algorithm=*)
        alg="${i#*=}"
        shift
        ;;
        -v=*|--video=*)
        video="${i#*=}"
        shift
        ;;
        -uen=*|--ue-number=*)
        ue_n="${i#*=}"
        shift
        ;;
        -enbn=*|--enb-number=*)
        enb_n="${i#*=}"
        shift
        ;;
        *)
        ;;
    esac
  done

else
  # if no arguments, read one by one
	echo -n "how many times do you wish to run? "
	read n
	echo -n "whats the algorithm used? "
	read alg
	echo -n "Video to be used: "
	read video
fi

# translate video to id
case $video in 
"highway")
  video_id=1;;
"container")
  video_id=2;;
"highway600")
  video_id=3;;
"akiyo")
  video_id=4;;
"masha")
  video_id=7;;
"babyshark")
  video_id=8;;
"despacito")
  video_id=9;;
*)
    exit 1
esac

# change id in file
sed -i "s/\(#define video \)[0-9]/\1$video_id/g" scratch/v2x_3gpp_small.cc
sed -i "s/\(node_ue = \)[0-9]\{,3\}/\1$ue_n/g" scratch/v2x_3gpp_small.cc
sed -i "s/\(low_power = \)[0-9]\{,3\}/\1$enb_n/g" scratch/v2x_3gpp_small.cc

# run simulation
./waf configure
mkdir ${alg}
for i in $(seq 1 $n)
do
    time ./waf --run "scratch/v2x_3gpp_small --handoverAlg=$alg --seedValue=$i" > log_${alg}_${i} 2>&1
    mkdir ${alg}_${enb_n}/simul${i}
    mv v2x_temp/* log_${alg}_${i} $alg_${enb_n}/simul$i
done
