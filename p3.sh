#!/bin/sh

for numNodes in 50 200 400 600 800 1000 
do
	for txPower in 1 20 50 150 300 500
	do
		for routingProtocol in "AODV" "OLSR"
		do
			for dataRate in 1
			do
				echo "" >> p3_out.txt
				./waf --run "p3 --numNodes=$numNodes --txPower=$txPower --routingProtocol=$routingProtocol --dataRate=$dataRate" >>p3_out.txt 
			done
		done
	done
done
