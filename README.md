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

-nbJammers : number of attackers in the network

-nbDevices : number of end-devices in the network

-appPeriod : period between two messages sent by end-devices

-appPeriodJam : period between two messages sent by attackers

-PayloadSize : size of the payload of the end-devices in bytes, the total length of the package will be payload size + 9 bytes (header)

-PayloadJamSize : size of the payload of the attackers in bytes, the total length of the package will be payload size + 9 bytes (header)

4. Then, the results are shown in this way:

```
E 0 4 2.27942  97.7206 0.569855
```

The first field corresponds to the type of event (E=energy consumption, R=reception, T=transmission, Jamming, D=dead device, I=interference)

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

4. Energy consumption

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

5. Battery Capacity

The Battery level is handled at the Lora-end-device-phy.cc, it calls the Energy consumption helper each time a change in the end-device status is made, the battery capacity can be set by editing the corresponding variable:

```
const double EndDeviceLoraPhy::battery_capacity = 100; // Standard Battery Capacity
```

