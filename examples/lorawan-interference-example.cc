/*
 * This script simulates a complex scenario with multiple gateways and end
 * devices. The metric of interest for this script is the throughput of the
 * network.
 */

#include "ns3/end-device-lora-phy.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/lora-energy-consumption-helper.h"
#include "ns3/gateway-lora-mac.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/node-container.h"
#include "ns3/mobility-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/periodic-sender-helper.h"

#include "ns3/command-line.h"
#include <algorithm>
#include <ctime>


//for tags
#include "ns3/tag-sender.h"


using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("ComplexLorawanNetworkExample");

// Network settings
int nDevices = 1;
int nJammers = 1;
int nGateways = 1;
double radius = 7500;
double simulationTime = 600;
int appPeriodSeconds = 5;
int appPeriodJamSeconds = 10;
//std::vector<int> sfQuantity (6);
int noMoreReceivers = 0;
int interfered = 0;
int received = 0;
int underSensitivity = 0;
int PayloadSize=20;
int PayloadJamSize=20;

// Output control
bool printEDs = true;
bool Trans = true;
bool SimTime = true;
bool buildingsEnabled = false;

/**********************
 *  Global Callbacks  *
 **********************/

enum PacketOutcome {
  RECEIVED,
  INTERFERED,
  NO_MORE_RECEIVERS,
  UNDER_SENSITIVITY,
  UNSET
};


Ptr<Packet> EnableChecking;

struct PacketStatus {
  Ptr<Packet const> packet;
  uint32_t senderId;
  int outcomeNumber;
  int packetId=0;
  std::vector<enum PacketOutcome> outcomes;
};

std::map<Ptr<Packet const>, PacketStatus> packetTracker;

void
CheckReceptionByAllGWsComplete (std::map<Ptr<Packet const>, PacketStatus>::iterator it)
{
  // Check whether this packet is received by all gateways
  if ((*it).second.outcomeNumber == nGateways)
    {
      // Update the statistics
	  PacketStatus status = (*it).second;
      for (int j = 0; j < nGateways; j++)
        {
          switch (status.outcomes.at (j))
            {
            case RECEIVED:
              {
                received += 1;
                break;
              }
            case UNDER_SENSITIVITY:
              {
                underSensitivity += 1;
                break;
              }
            case NO_MORE_RECEIVERS:
              {
                noMoreReceivers += 1;
                break;
              }
            case INTERFERED:
              {
                interfered += 1;
                break;
              }
            case UNSET:
              {
                break;
              }
            }
        }
      // Remove the packet from the tracker
      //packetTracker.erase (it);
    }
}

void
TransmissionCallback (Ptr<Packet const> packet, uint32_t systemId)
{
  //NS_LOG_INFO ("T " << systemId);
  // Create a packetStatus


  PacketStatus status;
  status.packet = packet;
  status.senderId = systemId;
  status.outcomeNumber = 0;
  status.outcomes = std::vector<enum PacketOutcome> (nGateways, UNSET);
  packetTracker.insert (std::pair<Ptr<Packet const>, PacketStatus> (packet, status));

  // create a tag.
  MyTag tag;
  tag.SetSimpleValue (systemId);
  //tag.SetSimpleValue (0xFF);
  packet->AddPacketTag (tag);

  // Print

  if (systemId < nDevices)
  {
	  std::cout << "T " << systemId << " " << packet->GetSize () << " " << Simulator::Now ().GetSeconds () << std::endl;
  }
  else
  {
	  std::cout << "J " << systemId << " " << packet->GetSize () << " " << Simulator::Now ().GetSeconds () << std::endl;
  }

}

void
PacketReceptionCallback (Ptr<Packet const> packet, uint32_t systemId)
{
  // Remove the successfully received packet from the list of sent ones
  // NS_LOG_INFO ("A packet was successfully received at gateway " << systemId);


	  // create a copy of the packet
	  Ptr<Packet> aCopy = packet->Copy ();

	  // read the tag from the packet copy
	  MyTag sender_tag;
	  aCopy->PeekPacketTag (sender_tag);


  std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  (*it).second.outcomes.at (systemId - nJammers - nDevices) = RECEIVED;
  (*it).second.outcomeNumber += 1;

  CheckReceptionByAllGWsComplete (it);

  // Print
  std::cout << "R " << systemId << " " << packet->GetSize () << " " << Simulator::Now ().GetSeconds ()  << " ";

  sender_tag.Print (std::cout);
  std::cout << std::endl;

//  aCopy->PrintPacketTags (std::cout);

}

void
InterferenceCallback (Ptr<Packet const> packet, uint32_t systemId)
{
  //NS_LOG_INFO ("A packet was lost because of interference at gateway " << systemId);

    //packet->AddPacketTag (const ns3::Tag);
  std::cout << "I " << systemId << " " << packet->GetSize () << " " << Simulator::Now ().GetSeconds () << std::endl;

  std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  (*it).second.outcomes.at (systemId - nJammers - nDevices) = INTERFERED;
  (*it).second.outcomeNumber += 1;

  CheckReceptionByAllGWsComplete (it);
}

void
NoMoreReceiversCallback (Ptr<Packet const> packet, uint32_t systemId)
{
  // NS_LOG_INFO ("A packet was lost because there were no more receivers at gateway " << systemId);

  std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  (*it).second.outcomes.at (systemId - nJammers - nDevices) = NO_MORE_RECEIVERS;
  (*it).second.outcomeNumber += 1;

  CheckReceptionByAllGWsComplete (it);
}

void
UnderSensitivityCallback (Ptr<Packet const> packet, uint32_t systemId)
{
  // NS_LOG_INFO ("A packet arrived at the gateway under sensitivity at gateway " << systemId);

  std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  (*it).second.outcomes.at (systemId - nJammers - nDevices) = UNDER_SENSITIVITY;
  (*it).second.outcomeNumber += 1;

  CheckReceptionByAllGWsComplete (it);
}

time_t oldtime = std::time (0);

// Periodically print simulation time
void PrintSimulationTime (void)
{
  //NS_LOG_INFO ("Time: " << Simulator::Now().GetHours());
  std::cout << "Simulated time: " << Simulator::Now ().GetHours () << " hours" << std::endl;
  std::cout << "Real time from last call: " << std::time (0) - oldtime << " seconds" << std::endl;
  oldtime = std::time (0);
  Simulator::Schedule (Minutes (30), &PrintSimulationTime);
}


void
EnergyConsumptionCallback (uint32_t NodeId, int ConsoType, double event_conso, double battery_level)
{
  NS_LOG_INFO ("The energy consumption of Node " << NodeId << event_conso << "Conso type " << " " << ConsoType << "at " << Simulator::Now ().GetSeconds ());
	//Conso Type: 1 for TX, 2 for RX, 3 for Standby and 4 Sleep

  std::cout << "E " << NodeId << " " << ConsoType << " " << event_conso << " " << " " << battery_level << " " << Simulator::Now ().GetSeconds () << std::endl;

}

void
DeadDeviceCallback (uint32_t NodeId, double cumulative_tx_conso, double cumulative_rx_conso, double cumulative_stb_conso, double cumulative_sleep_conso, Time dead_time)
{
  NS_LOG_INFO ("The Node " << NodeId << "was dead at " << dead_time.GetSeconds () << "at " << Simulator::Now ().GetSeconds ());
		//Conso Type: 1 for TX, 2 for RX, 3 for Standby and 4 Sleep

  std::cout << "D " << NodeId << " " << cumulative_tx_conso << " " << cumulative_rx_conso << " " << cumulative_stb_conso << " " << cumulative_sleep_conso << " " << dead_time.GetSeconds () << std::endl;

}


void
PrintEndDevices (NodeContainer endDevices, NodeContainer Jammers, NodeContainer gateways, std::string filename)
{
  const char * c = filename.c_str ();
  std::ofstream spreadingFactorFile;
  spreadingFactorFile.open (c);

  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      int sf = int(mac->GetDataRate ());
      Vector pos = position->GetPosition ();
      spreadingFactorFile << pos.x << " " << pos.y << " " << sf << std::endl;
    }

  for (NodeContainer::Iterator j = Jammers.Begin (); j != Jammers.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      int sf = int(mac->GetDataRate ());
      Vector pos = position->GetPosition ();
      spreadingFactorFile << pos.x << " " << pos.y << " " << sf << std::endl;
    }

  // Also print the gateways
  for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      Vector pos = position->GetPosition ();
      spreadingFactorFile << pos.x << " " << pos.y << " GW" << std::endl;
    }
  spreadingFactorFile.close ();
}


int main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.AddValue ("nDevices", "Number of end devices to include in the simulation", nDevices);
  cmd.AddValue ("nJammers", "Number of Jammers to include in the simulation", nJammers);
  cmd.AddValue ("radius", "The radius of the area to simulate", radius);
  cmd.AddValue ("simulationTime", "The time for which to simulate", simulationTime);
  cmd.AddValue ("appPeriod", "The period in seconds to be used by periodically transmitting applications", appPeriodSeconds);
  cmd.AddValue ("appPeriodJam", "The period in seconds to be used by periodically transmitting applications", appPeriodJamSeconds);
  cmd.AddValue ("printEDs", "Whether or not to print a file containing the ED's positions", printEDs);
  cmd.AddValue ("PayloadSize", "Payload size of the Packet - Lora Node", PayloadSize);
  cmd.AddValue ("PayloadJamSize", "Payload size of the Packet - Jamming Node", PayloadJamSize);
  cmd.Parse (argc, argv);

//	Set up logging
//  LogComponentEnable ("ComplexLorawanNetworkExample", LOG_LEVEL_ALL);
//  LogComponentEnable("LoraChannel", LOG_LEVEL_INFO);
//  LogComponentEnable("LoraPhy", LOG_LEVEL_ALL);
//  LogComponentEnable("EndDeviceLoraPhy", LOG_LEVEL_ALL);
//  LogComponentEnable("GatewayLoraPhy", LOG_LEVEL_ALL);
//  LogComponentEnable("LoraInterferenceHelper", LOG_LEVEL_ALL);
//  LogComponentEnable("LoraMac", LOG_LEVEL_ALL);
//  LogComponentEnable("EndDeviceLoraMac", LOG_LEVEL_ALL);
//  LogComponentEnable("GatewayLoraMac", LOG_LEVEL_ALL);
//  LogComponentEnable("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
//  LogComponentEnable("LogicalLoraChannel", LOG_LEVEL_ALL);
//  LogComponentEnable("LoraHelper", LOG_LEVEL_ALL);
//  LogComponentEnable("LoraPhyHelper", LOG_LEVEL_ALL);
//  LogComponentEnable("LoraMacHelper", LOG_LEVEL_ALL);
//  LogComponentEnable("PeriodicSenderHelper", LOG_LEVEL_ALL);
//  LogComponentEnable("PeriodicSender", LOG_LEVEL_ALL);
//  LogComponentEnable("LoraMacHeader", LOG_LEVEL_ALL);
//  LogComponentEnable("LoraFrameHeader", LOG_LEVEL_ALL);
//	LogComponentEnable("LoraMacHeader", LOG_LEVEL_ALL);
 // LogComponentEnable("LoraEnergyConsumptionHelper", LOG_LEVEL_ALL);


  /**********
  *  Setup  *
  **********/

  // Create the time value from the period
  Time appPeriod = Seconds (appPeriodSeconds);
  Time appPeriodJam = Seconds (appPeriodJamSeconds);

  // Mobility
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                 "rho", DoubleValue (radius),
                                 "X", DoubleValue (0.0),
                                 "Y", DoubleValue (0.0));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  /************************
  *  Create the channel  *
  ************************/

  // Create the lora channel object
  Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
  loss->SetPathLossExponent (3.76);
  loss->SetReference (1, 8.1);

  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();
  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);

  /************************
  *  Create the helpers  *
  ************************/

  // Create the LoraPhyHelper
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);

  // Create the LoraMacHelper
  LoraMacHelper macHelper = LoraMacHelper ();

  // Create the LoraHelper
  LoraHelper helper = LoraHelper ();


  /************************
  *  Create End Devices  *
  ************************/

  // Create a set of nodes
  NodeContainer endDevices;
  endDevices.Create (nDevices);

  // Assign a mobility model to each node
  mobility.Install (endDevices);

  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = endDevices.Begin ();
       j != endDevices.End (); ++j)
    {
      Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      Vector position = mobility->GetPosition ();
      position.z = 1;
      position.x = 8000;
      position.y = 0;
      mobility->SetPosition (position);
    }

// Create the LoraNetDevices of the end devices
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LoraMacHelper::ED);
  helper.Install (phyHelper, macHelper, endDevices);

  // Now end devices are connected to the channel

  // Connect trace sources
  for (NodeContainer::Iterator j = endDevices.Begin ();
       j != endDevices.End (); ++j)
    {
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();
      phy->TraceConnectWithoutContext ("StartSending",
                                       MakeCallback (&TransmissionCallback));
      phy->TraceConnectWithoutContext ("EnergyConsumptionCallback",
                                       MakeCallback (&EnergyConsumptionCallback));
      phy->TraceConnectWithoutContext ("DeadDeviceCallback",
                                       MakeCallback (&DeadDeviceCallback));
    }




  /************************
  *  Create Jammers  *
  ************************/

// Create a set of Jammers
  NodeContainer Jammers;
  Jammers.Create (nJammers);

  // Assign a mobility model to each node
  mobility.Install (Jammers);

  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = Jammers.Begin ();
       j != Jammers.End (); ++j)
    {
      Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      Vector position = mobility->GetPosition ();
      position.z = 1;
      position.x = 12;
      position.y = 0;
      mobility->SetPosition (position);
    }

  // Create the LoraNetDevices of the end devices
  phyHelper.SetDeviceType (LoraPhyHelper::JM);
  macHelper.SetDeviceType (LoraMacHelper::JM);
  helper.Install (phyHelper, macHelper, Jammers);

  // Now Jammers are connected to the channel

  // Connect trace sources
  for (NodeContainer::Iterator j = Jammers.Begin ();
       j != Jammers.End (); ++j)
    {
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();
      phy->TraceConnectWithoutContext ("StartSending",
                                       MakeCallback (&TransmissionCallback));
      phy->TraceConnectWithoutContext ("EnergyConsumptionCallback",
                                       MakeCallback (&EnergyConsumptionCallback));
    }


  /*********************
  *  Create Gateways  *
  *********************/

  NodeContainer gateways;
  gateways.Create (nGateways);

  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  allocator->Add (Vector (0.0, 0.0, 0.0));
  mobility.SetPositionAllocator (allocator);
  mobility.Install (gateways);

  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = gateways.Begin ();
       j != gateways.End (); ++j)
    {
      Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      Vector position = mobility->GetPosition ();
      position.z = 15;
      mobility->SetPosition (position);
    }

  // Create a netdevice for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LoraMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);

  /************************
  *  Configure Gateways  *
  ************************/

  // Install reception paths on gateways
  for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); j++)
    {

      Ptr<Node> object = *j;
      // Get the device
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<GatewayLoraPhy> gwPhy = loraNetDevice->GetPhy ()->GetObject<GatewayLoraPhy> ();

      // Set up height of the gateway
      Ptr<MobilityModel> gwMob = (*j)->GetObject<MobilityModel> ();
      Vector position = gwMob->GetPosition ();
      position.z = 15;
      gwMob->SetPosition (position);

      // Global callbacks (every gateway)
      gwPhy->TraceConnectWithoutContext ("ReceivedPacket",
                                         MakeCallback (&PacketReceptionCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseInterference",
                                         MakeCallback (&InterferenceCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseNoMoreReceivers",
                                         MakeCallback (&NoMoreReceiversCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseUnderSensitivity",
                                         MakeCallback (&UnderSensitivityCallback));
    }

  /**********************************************
  *  Set up the end device's spreading factor  *
  **********************************************/

  macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);
//  NS_LOG_DEBUG ("Completed configuration");


  /**********************************************
  *  Set up the end jammer's spreading factor  *
  **********************************************/

  macHelper.SetSpreadingFactorsUpJm (Jammers, gateways, channel);

  // Verify if we can change this to set manually an specific SF for the Jam nodes

  /*********************************************
  *  Install applications on the end devices  *
  *********************************************/


  Time appStopTime = Seconds (simulationTime);
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  appHelper.SetPeriod (Seconds (appPeriodSeconds));
  appHelper.SetPacketSize (PayloadSize);
  ApplicationContainer appContainer = appHelper.Install (endDevices);

  appContainer.Start (Seconds (0));
  appContainer.Stop (appStopTime);


  /*********************************************
  *  Install applications on the Jammer *
  *********************************************/

  Time appJamStopTime = Seconds (simulationTime);
  PeriodicSenderHelper appJamHelper = PeriodicSenderHelper ();
  appJamHelper.SetPeriod (Seconds (appPeriodJamSeconds));
  appJamHelper.SetPacketSize (PayloadJamSize);

  ApplicationContainer appJamContainer = appJamHelper.Install (Jammers);

  appJamContainer.Start (Seconds (1));
  appJamContainer.Stop (appJamStopTime);

  /**********************
   * Print output files *
   *********************/

  if (printEDs)
    {

	  PrintEndDevices (endDevices, Jammers, gateways,
                   "scratch/Devices.dat");
    }

  /****************
  *  Simulation  *
  ****************/

  Simulator::Stop (appJamStopTime);

  // PrintSimulationTime ();

  Simulator::Run ();

  Simulator::Destroy ();

  /************
  *  Results  *
  *************
  double receivedProb = double(received)/nDevices;
  double interferedProb = double(interfered)/nDevices;
  double noMoreReceiversProb = double(noMoreReceivers)/nDevices;
  double underSensitivityProb = double(underSensitivity)/nDevices;

  double receivedProbGivenAboveSensitivity = double(received)/(nDevices - underSensitivity);
  double interferedProbGivenAboveSensitivity = double(interfered)/(nDevices - underSensitivity);
  double noMoreReceiversProbGivenAboveSensitivity = double(noMoreReceivers)/(nDevices - underSensitivity);
  std::cout << nDevices << " " << double(nDevices)/simulationTime << " " << receivedProb << " " << interferedProb << " " << noMoreReceiversProb << " " << underSensitivityProb <<
  " " << receivedProbGivenAboveSensitivity << " " << interferedProbGivenAboveSensitivity << " " << noMoreReceiversProbGivenAboveSensitivity << std::endl;
*/

  /*

  // Print the packetTracker contents
   std::cout << "Packet outcomes" << std::endl;
   std::map<Ptr<Packet const>, PacketStatus>::iterator i;
   for (i = packetTracker.begin (); i != packetTracker.end (); i++)
     {
       PacketStatus status = (*i).second;
       std::cout.width (4);
       std::cout << status.senderId << "\t";
       for (int j = 0; j < nGateways; j++)
         {
           switch (status.outcomes.at (j))
             {
             case RECEIVED:
               {
                 std::cout << "R ";
                 break;
               }
             case UNDER_SENSITIVITY:
               {
                 std::cout << "U ";
                 break;
               }
             case NO_MORE_RECEIVERS:
               {
                 std::cout << "N ";
                 break;
               }
             case INTERFERED:
               {
                 std::cout << "I ";
                 break;
               }
             case UNSET:
               {
                 std::cout << "E ";
                 break;
               }
             }
         }
       std::cout << std::endl;
     }
*/

  return 0;
}
