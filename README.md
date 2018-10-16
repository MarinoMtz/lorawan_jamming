# NS3 LoRaWAN

This module is a fork of https://github.com/signetlabdei/lorawan 

## Getting started

1. Simulation file is on scratch/lorawan-network-attack-example
2. lorawan module is on src/lorawan

3. To start a simulation we navigate to /ns-3-allinone/ns-3-dev and then:
```
./waf --run "scratch/lorawan-network-attack-example --appPeriod=20 --nJammers=40 --appPeriodJam=10 --nDevices=2 --PayloadSize=30 --PayloadJamSize=10"
```
Here we have 6 parameters that allow us to set how the simulation will behave :
   - nbJammers : number of attackers in the network
   - nbDevices : number of end-devices in the network
   - appPeriod : period between two messages sent by end-devices
   - appPeriodJam : period between two messages sent by attackers
   - PayloadSize : size of the payload of the end-devices in bytes, the total length of the package will be payload size + 9 bytes (header)
   - PayloadJamSize : size of the payload of the attackers in bytes, the total length of the package will be payload size + 9 bytes (header)

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
   - Reception : R, gateway ID, packet size (including header), simulation time, Tx end-device ID
   - Transmission : T, node ID, packet size (including header), simulation time
   - Jamming : J, jammer ID, packet size (including header), simulation time
   - Interference : I, node ID, packet size (including heqder), simulaton time
   - Dead device : D, Node ID, Cumulative consumption due to Tx events, Cumulative consumption due to Rx events, Cumulative consumption due to Sleep status, Dead Time

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
// LoRa transmission consumption vector in mA/s for each Spreading Factor
const double LoraEnergyConsumptionHelper::consotx[6] = {1, 1, 1, 1, 1, 1}; //SF7  SF8  SF9  SF10 SF11 SF12
// LoRa reception consumption vector in mA/s for each Spreading Factor
const double LoraEnergyConsumptionHelper::consorx[6] = {2, 2, 2, 2, 2, 2}; // SF7  SF8  SF9  SF10 SF11 SF12
// LoRa consumption vector in mA/s for standby
const double LoraEnergyConsumptionHelper::consostb = 3;
// LoRa consumption vector in mA/s for Sleep
const double LoraEnergyConsumptionHelper::consosleep = 4;
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

| DR | SF | Tsym | Tpreamble | Trx1 | Trx2 |
| --- | --- | --- | --- |
| 5 | 7 | 0.001024 | 0.012544 |
| 4 | 8 | 0.002048 | 0.025088 |
| 3 | 9 | 0.050176 | 0.050176 |
| 2 | 10 | 0.100352 | 0.100352 |
| 1 | 11 | 0.016384 | 0.200704 |
| 0 | 12 | 0.032768 | 0.401488 |

## Time on air and Reception Windows



