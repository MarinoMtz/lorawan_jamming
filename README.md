# NS3 LoRaWAN

This module is a fork of https://github.com/signetlabdei/lorawan 

## Getting started

1. Simulation file is on scratch/lorawan-network-attack-example
2. lorawan module is on src/lorawan

3. To start a simulation we navigate till /ns-3-allinone/ns-3-dev and then:

./waf --run "scratch/lorawan-network-attack-example --appPeriod=20 --nJammers=40 --appPeriodJam=10 --nDevices=2 --PayloadSize=30 --PayloadJamSize=10"

4. Then the results are shown in this way:

R 42 29 599.614 30   0

Type of event (Rx, Tx, Jamm), Node ID, packet size (including header), Time stamp, Tx Node

The last one (0) is created by the sistem by default in all node each time I create a tag, still dunno what it exactly means.


