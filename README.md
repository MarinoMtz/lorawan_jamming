# NS3 LoRaWAN

This module is a fork of https://github.com/signetlabdei/lorawan 

## Getting started

1. Simulation file is on scratch/lorawan-network-attack-example
2. lorawan module is on src/lorawan

3. To start a simulation we navigate to /ns-3-allinone/ns-3-dev and then:
```
./waf --run "scratch/lorawan-network-attack-multiple-GW --nGateways=2 --nJammers_up=50 --nJammers_dw=0 --JammerType=3 --nDevices=50 --PayloadSize=42 --Specific_SF=1 --ED_SF=7 --simulationTime=3600 --PayloadJamSize_up=42 --JammerTxPower=14 --Random_SF=0 --JammerDutyCycle_up=0.1 --JammerSF=7 --updw=0 --authpre=0 --Exponential=1 --Int_Model=4 --delta=6 --retransmission=1 --maxtx=$h --Conf_UP=1 --Net_Ser=1"
```
Here we have several parameters that allow us to set how the simulation will behave :
```
Program Arguments:
    --nDevices:            Number of end devices to include in the simulation [50]
    --nGateways:           Number of Gateways to include in the simulation [2]
    --Conf_UP:             Confirmed data UP [true]
    --radius:              radius of the disc where nodes will be deployed [500]
    --simulationTime:      The time for which to simulate [3600]
    --appPeriod:           The period in seconds to be used by periodically transmitting applications [50]
    --appPeriodJam:        The period in seconds to be used by periodically transmitting applications [1]
    --printEDs:            Whether or not to print a file containing the ED's positions [true]
    --PayloadSize:         Payload size of the Packet - Lora Node [42]
    --nJammers_up:         Number of Uplink Jammers to include in the simulation [50]
    --nJammers_dw:         Number of Downlink Jammers to include in the simulation [0]
    --PayloadJamSize_up:   Payload size of the Packet - Jamming Node [42]
    --PayloadJamSize_dw:   Payload size of the Packet - Jamming Node [50]
    --JammerType:          Attacker Profile [3]
    --JammerFrequency_up:  Jammer Frequency in MHz [868.1]
    --JammerFrequency_dw:  Jammer Frequency in MHz [869.525]
    --JammerSF:            Jammer SF, if not random [7]
    --JammerTxPower:       Jammer TX Poxer in dBm  [14]
    --Random_SF:           Boolean variable to set whether the Jammer select a random SF to transmit [false]
    --JammerDutyCycle_dw:  Jammer duty cycle [0.5]
    --JammerDutyCycle_up:  Jammer duty cycle [0.1]
    --Exponential:         Exponential inter-arrival time [true]
    --lambda_jam_up:       Lambda to be used by the jammer to jam on the uplink channel [10]
    --lambda_jam_dw:       Lambda to be used by the jammer to jam on the uplink channel [10]
    --Jammer_length_up:    Jammer Packet length uplink [10]
    --Jammer_length_dw:    Jammer Packet length downlink [10]
    --updw:                boolean variable indicating if there will be uplink and downlink jammers in the simulation [false]
    --two_rx:               boolean variable indicating if there are one or two ack receive windows [true]
    --ack_sf:               sf to be used in the first rx (if only one rx is used) or in the second (if two rx) [7]
    --acklength:            ACK Packet length [1]
    --acksamesf:            boolean variable indicating if the ACK is sent on the same SF [false]
    --Specific_SF:          boolean variable indicating if EDs use an specific Spreading Factor [true]
    --ED_SF:               ED's Spreading Factor [7]
    --retransmission:       boolean variable indicating if the ED resends packets or not [true]
    --percentage_rtx:       Percentage of re-transmitted packets [0]
    --maxtx:                Maximum number of transmissions for each packets [1]
    --multi_channel:        Use of multiple channels [false]
    --Net_Ser:             Network Server [true]
    --EWMA:                Boolean variable to set whether or not the Network implements the EWMA Algorithm [false]
    --InterArrival:        Boolean variable to set whether or not the Network server computes the IAT [false]
    --NS_buffer:           Length of Network Server Buffer [10]
    --lambda:              lambda parameter for the EWMA algorithm btw 0-1  [0]
    --authpre:             Authenticated preambles  [false]
    --Int_Model:           1 - ALOHA, 2 - Rx level grater than delta, 3 - Interferer cumulative energy,  4 - Co-channel [4]
    --delta:               delta in dB [6]
    --Filename:            Filename []
    --Path:                Path []
```
4. Then, the results are shown in this way:
```
E 0 4 2.27942  97.7206 0.569855
E 0 3 0  97.7206 0.569855
E 0 1 1.31891  96.4017 0.569855
T 0 19 0 0.569855
E 0 3 3e-08  96.4017 1.88877
R 1 19 1.88879 0
E 0 4 6.27942  92.4017 2.88877
E 0 3 1.20422  91.1974 3.29017
E 0 4 8.67379  88.8031 3.88877
E 0 3 2.40845  87.5989 4.29017
```
The first field corresponds to the type of event (E=energy consumption, R=reception, T=transmission, Jamming, D=dead device, I=interference):
   - Energy consumption : E, Node ID, state (1=Tx, 2=Rx, 3=Stb, 4=sleep), cumulative consumption of the current event, remaining battery level, simulation time (in seconds)
   - Reception : R, gateway ID, packet size (including header), Tx end-device ID, simulation time 
   - Transmission : T, node ID, packet size (including header), simulation time
   - Jamming : J, jammer ID, packet size (including header), simulation time
   - Colission : C, Rx node ID, Sender ID, SF, Start Time, End Time, On the preamble (bool)
   - Dead device : D, Node ID, Cumulative consumption due to Tx events, Cumulative consumption due to Rx events, Cumulative consumption due to Sleep status, Dead Time

5. Output files: appart from the LOG, there is also a possibility of printing some files that contain some parameters regarding the simulation:
   - Devices.dat: contains the position and the SF of each End-Device and Jammer, it is stored at "scratch/Devices.dat"

## Configuration parameters

In addition to the simulation parameters that can be set when the simulation is launched, there are some others that can be changed only by editing the corresponding .cc file:

1. Reception Windows time stamps: In Class A Devices there are two reception Windows, the first one start one second after the transmission event, and the second one two seconds after. Then the duration of both are set by default as the minimal time required to detect a preamble at the corresponding SF, these time-stamps can be set in end-device-lora-mac.cc:
``` 
  m_receiveDelay1 (Seconds (1)),            // LoraWAN default
  m_receiveDelay2 (Seconds (2)),            //WAN default
  m_receiveWindowDuration (Seconds (0.2)),  // This will be set as the time necessary to detect a preamble at the corresponding sf
```

2. Sub-band parameters for jammer nodes: We included three changes regarding the sub-band rules that are aplied to jammer devices: Duty Cycle and Tx Power, and maximum payload length, it can be set by editing the ApplyCommonEuConfigurationsJm function in the lora-mac-helper.cc file:
``` 
  channelHelper.AddSubBand (868, 868.6, 1, 14); // (firstFrequency, lastFrequency, dutyCycle, maxTxPowerDbm)
  channelHelper.AddSubBand (868.7, 869.2, 1, 14); // (firstFrequency, lastFrequency, dutyCycle, maxTxPowerDbm)
  channelHelper.AddSubBand (869.4, 869.65, 1, 27); // (firstFrequency, lastFrequency, dutyCycle, maxTxPowerDbm)
```
```
loraMac->SetMaxAppPayloadForDataRate (std::vector<uint32_t> {59,59,59,123,230,230,230,230}); //maximal payload length for each spreading factor (12,11,10,9,8,7,7)
```

3. Spreading factor selection for end-devices and jammer nodes: 
The algorithm that handles the sf selection is located in the lora-mac-helper.cc file et the SetSpreadingFactorsUp(Jm) function, it select the appropiated sf based on the distance from each gateway and the jammer/end-device node.

4. Energy consumption: 
In order to handle the energy consumption of end-devices, the Energy Consumption helper was created, it computes the energy consumption of each state based on a pre-defined energy consumption per second, it can be modified by editing the lora-energy-consumption-helper.cc :
``` 
// LoRa transmission consumption vector in mA for each Spreading Factor
const double LoraEnergyConsumptionHelper::consotx[6] = {83 , 83, 83, 83, 83, 83};
// 							SF7  SF8  SF9  SF10 SF11 SF12
// LoRa reception consumption vector in mA for each Spreading Factor
const double LoraEnergyConsumptionHelper::consorx[6] = {32, 32, 32, 32, 32, 32};
// 							SF7  SF8  SF9  SF10 SF11 SF12
// LoRa consumption vector in mA for standby
const double LoraEnergyConsumptionHelper::consostb = 32;
// LoRa consumption vector in mA for Sleep
const double LoraEnergyConsumptionHelper::consosleep = 0.0045;
```

5. Battery Capacity: 
The Battery level is handled at the Lora-end-device-phy.cc, it calls the Energy consumption helper each time a change in the end-device status is made, the battery capacity can be set by editing the corresponding variable:
```
const double EndDeviceLoraPhy::battery_capacity = 100; // Standard Battery Capacity
```

## Time on air and Reception Windows

The time on air is the most important parameter in the simulation. It allows us to compute the packet and receive windows' duration. The formulas used to compute it are based on the [LoRa modem designer guide](https://www.semtech.com/uploads/documents/LoraDesignGuide_STD.pdf) and are handled at the [LoRa PHY](model/lora-phy.cc) file. It is computed as a funtion of the transmission parameters (spreading factor, bandwith, coding rate and packet size) and is composed of two time slots: the preamble time and the time to transmit the payload. 

For the case of the receive windows' duration, as indicated by the [LoRaWAN Specification v1.0.3](https://lora-alliance.org/resource-hub/lorawantm-specification-v103) they must be set as at least the time required to detect a downlink preambule. 

For this simulation we follow the model proposed by [Luis Casals et al.](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5677147/pdf/sensors-17-02364.pdf) where, in the case of the first receive window this time corresponds to the symbol timeout parameter (SymbTimeout), which is set to 8 symbols for DR0 and DR1 (SF12 and SF11) and 12 symbols for the rest of the DRs, and in the case of the second receive window the CAD (Channel Activity Detection) feature is used. The following table shows the symbol duration, the corresponding preamble duration and the first and second receive windows. We assume an 8-symbol preamble length, a CR of 4/5 and a bandwidth of 125 kHz. 

> We do not consider the cases of DR6 and DR7 that are based on FSK

| SF | DR | Tsym | Tpreamble | Trx1 | Trx2 |
| --- | --- | --- | --- | --- | --- |
| 7 | 5 | 0.001024 | 0.012544 | 0.012288 | 0.00128 |
| 8 | 4 | 0.002048 | 0.025088 | 0.024576 | 0.002304 |
| 9 | 3 | 0.050176 | 0.050176 | 0.049152 | 0.004352 |
| 10 | 2 | 0.100352 | 0.100352 | 0.098304 | 0.008448 |
| 11 | 1 | 0.016384 | 0.200704 | 0.131072 | 0.01664 |
| 12 | 0 | 0.032768 | 0.401488 | 0.262144 | 0.033024 |

## Time on air and Reception Windows


## License

This software is licensed under the terms of the GNU GPLv2 (the same license
that is used by ns-3). See the LICENSE.md file for more details.)
