#!/bin/bash
#PBS -q ECE6110
#PBS -l nodes=1:fourcore
#PBS -l walltime=00:10:00
#PBS -N Rishipal
# The below chnages the working directory to the location of
# your testmpi program
# The below tells MPI to run your jobs on 16 processors
cd ns-3.21
echo "" > p3-output.txt
echo "numNodes\t\t\ttxPower\t\t\troutingProtocol\t\t\tdataRate\t\t\tbytesRx\t\t\tbytesTx\t\t\tEfficiency" >> p3-output.txt
clear
for numNodes in 20 30 40 50 60 70 80 90 100 200 300 400 500 600 700 800 900 1000
do
for txPower in 1 2 4 8 10 15 20 25 50 100 200 300 400 500
do
for routingProtocol in "olsr" "aodv"
do
for trafficIntensity 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0
do
./waf --run "p3 --numNodes=$numNodes --txPower=$txPower --routingProtocol=$routingProtocol --trafficIntensity=$trafficIntensity"
done
done
done
done
