/*
 * This script simulates a complex scenario with multiple gateways and end
 * devices. The metric of interest for this script is the throughput of the
 * network.
 */

#include "ns3/end-device-lora-phy.h"
#include "ns3/jammer-lora-phy.h"
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
#include "ns3/one-shot-sender-helper.h"
#include "ns3/attack-helper.h"
#include "ns3/app-jammer.h"
#include "ns3/app-jammer-helper.h"

#include "ns3/rng-seed-manager.h"
#include "ns3/network-server-helper.h"
#include "ns3/forwarder-helper.h"
#include "ns3/lora-tag.h"
#include "ns3/object.h"

#include "ns3/command-line.h"
#include "ns3/random-variable-stream.h"

#include <vector>
#include <algorithm>
#include <ctime>

#include <ns3/simple-network-server.h>

using namespace std;
using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("LorawanNetworkAttackExample");

// Network settings
uint32_t nDevices = 0;
uint32_t nGateways = 1;
uint32_t nJammers = 1;
double radius = 1000;

// Uniform random variable to allocate nodes
Ptr<UniformRandomVariable> rnd_alloc = CreateObject<UniformRandomVariable> ();

double simulationTime = 1000;
double appPeriodSeconds = 50;

double appPeriodJamSeconds = 1;
double JammerType = 3;
double JammerFrequency = 868.1;
double JammerTxPower = 25;
double JammerDutyCycle = 0.5;
double JammerSF = 7;


double noMoreReceivers = 0;
double collision_jm = 0;
double collision_ed = 0;
double edreceived = 0;
double gwreceived_jm = 0;
double gwreceived_ed = 0;
double underSensitivity_jm = 0;
double underSensitivity_ed = 0;
double edsent = 0;
double gwsent = 0;
double jmsent = 0;
double dropped_jm = 0;
double dropped_ed = 0;
double cumulative_time_jm = 0;
double cumulative_time_ed = 0;
double ce_ed = 0;
double ce_jm = 0;

int PayloadSize=1;
double PayloadJamSize=50;


bool Conf_UP = false;
bool Net_Ser = false;
bool Random_SF = true;
bool All_SF = false;
bool Exponential = false;

// EDs Spreading Factor selection
bool Specific_SF = true;
double ED_SF = 7;


// Output control
bool printEDs = false;
bool Trans = true;
bool SimTime = true;
bool buildingsEnabled = false;

// variables for the NS algorithms
bool InterArrival = true;
int NS_buffer = 10;
double lambda = 0;

//Authenticated preambles at the GW level
bool authpre = false;

// Detection algs at the NetServer level
bool ewma_training = false;
double ucl = 15;
double lcl = 3;

// Interference model, -- set up at the GW level (phy)
// 1 - Pure_ALOHA
// 2 - Co-channel rejection Matrix

int Int_Model = 1;


vector<uint32_t> pkt_success_ed(nDevices+nJammers,0);
vector<uint32_t> pkt_drop_ed(nDevices+nJammers,0);
vector<uint32_t> pkt_loss_ed(nDevices+nJammers,0);
vector<uint32_t> pkt_send(nDevices+nJammers,0);

vector<uint32_t> pkt_loss_gw(nGateways,0);
vector<uint32_t> pkt_drop_gw(nGateways,0);
vector<uint32_t> pkt_success_gw(nGateways,0);


enum PrintType {
	GR,
	ET,
	JT,
	GT,
	ER,
	GD,
	C,
	U
};


string Filename;
string Path;

/**********************
 *  Global Callbacks  *
 **********************/

void
PrintTrace (int Type, uint32_t NodeId, uint32_t SenderID, uint32_t Size, double frequencyMHz, uint8_t sf, Time colstart, Time colend, bool onthepreable, string filename)
{

	const char * c = filename.c_str ();
	ofstream Plot;
	Plot.open (c, ios::app);
	switch (Type)
	{
		case GR : Plot << "GR " << NodeId << " " << SenderID << " " << Size << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds () << endl;
		break;
		case ER : Plot << "ER " << NodeId << " " << SenderID << " " << Size << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds () << endl;
		break;
		case ET : Plot << "ET " << NodeId << " " << Size << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds () << endl;
		break;
		case GT : Plot << "ET " << NodeId << " " << Size << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds () << endl;
		break;
		case JT : Plot << "JT " << NodeId << " " << Size << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds () << endl;
		break;
		case C: Plot << "C " << NodeId << " " << SenderID  << " " << Size << " " << frequencyMHz << " " << unsigned(sf) << " " << colstart.GetSeconds() << " " << colend.GetSeconds() << " " << onthepreable << endl;
		break;
		case GD : Plot << "GD " << NodeId << " " << SenderID << " " << Size << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds () << endl;
		break;
		case U : Plot << "U " << NodeId << " " << SenderID << " " << Size << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds () << endl;
		break;
	}
	Plot.close ();
}

void
PrintResults(uint32_t nGateways, uint32_t nDevices, uint32_t nJammers, double receivedProb_ed, double collisionProb_ed,  double noMoreReceiversProb_ed, double underSensitivityProb_ed, double receivedProb_jm, double collisionProb_jm,  double noMoreReceiversProb_jm, double underSensitivityProb_jm, double gwreceived_ed, double gwreceived_jm, double edsent, double jmsent, double cumulative_time_ed, double cumulative_time_jm, double ce_ed, double ce_jm, string filename)
{

	const char * c = filename.c_str ();
	ofstream Plot;
	Plot.open (c, ios::app);
	Plot << nGateways << " " << nDevices << " " << nJammers << " " << receivedProb_ed << " " << collisionProb_ed
			<< " " << noMoreReceiversProb_ed << " " << underSensitivityProb_ed << " " << receivedProb_jm
			<< " " << collisionProb_jm << " " << noMoreReceiversProb_jm << " " << underSensitivityProb_jm
			<< " " << gwreceived_ed << " " << gwreceived_jm << " " << edsent << " " << jmsent
			<< " " << cumulative_time_ed << " " << cumulative_time_jm << endl;

    //cumulative_time_ed << " " << cumulative_time_jm << endl;
	Plot.close ();

}

void
EDTransmissionCallback (Ptr<Packet const> packet, uint32_t systemId, double frequencyMHz, uint8_t sf)
{
  //NS_LOG_INFO ("T " << systemId);

  NS_LOG_INFO ("T " << systemId << " " << packet->GetSize () << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds ());
//  PrintTrace (ET, systemId, 0, packet->GetSize (), frequencyMHz, sf, Seconds(0), Seconds(0), 0, "scratch/Trace.dat");
	  edsent += 1;

	  pkt_send [systemId] += 1;
}

void
JMTransmissionCallback (Ptr<Packet const> packet, uint32_t systemId, double frequencyMHz, uint8_t sf)
{

  NS_LOG_INFO ( "J " << systemId << " " << packet->GetSize () << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds ());
//  PrintTrace (JT, systemId, 0, packet->GetSize (), frequencyMHz, sf, Seconds(0), Seconds (0), 0, "scratch/Trace.dat");
  jmsent += 1;

  pkt_send [systemId] += 1;
}

void
GWTransmissionCallback (Ptr<Packet const> packet, uint32_t systemId, double frequencyMHz, uint8_t sf)
{

  //NS_LOG_INFO ("G " << systemId << " " << packet->GetSize () << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds ());
  //PrintTrace (GT, systemId, 0, packet->GetSize (), frequencyMHz, sf, Seconds(0), Seconds(0), 0, "scratch/Trace.dat");
  gwsent += 1;

}

void
GWReceivedurationCallback( Ptr<Packet const> packet, Time duration, uint32_t systemId, uint32_t SenderID, double frequencyMHz, uint8_t sf)
{

  //NS_LOG_INFO ("DU " << systemId << " " << packet->GetSize () << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds ());

  double time_on_air = 0;

  time_on_air = duration.GetSeconds();

  LoraTag tag;
  packet->PeekPacketTag (tag);
  uint8_t jammer = tag.GetJammer ();

  if (jammer == uint8_t(0))
  {
	  cumulative_time_ed += time_on_air;
  }

  else
  {
	  cumulative_time_jm += time_on_air;
  }

}

void
GWCaptureEffectCallback (Ptr<Packet const> packet, uint32_t systemId, uint32_t SenderID, double frequencyMHz, bool CE)
{
	//NS_LOG_INFO ("CE " << systemId << " " << SenderID << " " << frequencyMHz << " " << Simulator::Now ().GetSeconds ());

	LoraTag tag;
	packet->PeekPacketTag (tag);
	uint8_t jammer = tag.GetJammer ();

	if (jammer == uint8_t(0))
	  {
		  ce_ed += 1;
	  }

	else
	  {
		  ce_jm += 1;
	  }
}


void
GatewayReceiveCallback (Ptr<Packet const> packet, uint32_t systemId, uint32_t SenderID, double frequencyMHz, uint8_t sf)
{
  // Remove the successfully received packet from the list of sent ones
  // NS_LOG_INFO ("A packet was successfully received at gateway " << systemId);

  //NS_LOG_INFO ("R " << systemId << " " << SenderID << " " << packet->GetSize () << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds ());
  //PrintTrace (GR, systemId, SenderID, packet->GetSize (), frequencyMHz, sf, Seconds(0), Seconds(0), 0, "scratch/Trace.dat");

  LoraTag tag;
  packet->PeekPacketTag (tag);
  uint8_t jammer = tag.GetJammer ();
  //packet->AddPacketTag (tag);

  if (jammer == uint8_t(0))
  {
	  gwreceived_ed += 1;
  }

  else
  {
	  gwreceived_jm += 1;
  }

}

void
EnDeviceReceiveCallback (Ptr<Packet const> packet, uint32_t systemId, uint32_t SenderID, double frequencyMHz, uint8_t sf)
{
  // Remove the successfully received packet from the list of sent ones
  // NS_LOG_INFO ("A packet was successfully received at gateway " << systemId);

  //NS_LOG_INFO ("R " << systemId << " " << SenderID << " " << packet->GetSize () << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds ());
  //PrintTrace (ER, systemId, SenderID, packet->GetSize (), frequencyMHz, sf, Seconds (0), Seconds(0) ,0 , "scratch/Trace.dat");
  edreceived += 1;

}

void
CollisionCallback (Ptr<Packet const> packet, uint32_t systemId, uint32_t SenderID, uint8_t sf, double frequencyMHz, Time colstart, Time colend, bool onthepreable)

{
 //NS_LOG_INFO ("A packet was lost because of interference at gateway " << systemId);

 //NS_LOG_INFO( "C " << systemId << " " << SenderID  << " " << packet->GetSize () << " " << frequencyMHz << " " << unsigned(sf) << " " << colstart.GetSeconds() << " " << colend.GetSeconds() << " " << onthepreable);
 //PrintTrace (C, systemId, SenderID, packet->GetSize (), frequencyMHz, sf, colstart, colend, onthepreable, "scratch/Trace.dat");

 LoraTag tag_1;
 packet->PeekPacketTag (tag_1);
 uint8_t jammer = tag_1.GetJammer ();
 //packet->AddPacketTag (tag);

 pkt_loss_ed [SenderID] ++;
 pkt_loss_gw [systemId-nDevices-nJammers] ++;

 if (jammer == uint8_t(0))
  {
	  collision_ed += 1;
  }

 else
  {
	  collision_jm += 1;
  }
}

void
NoMoreReceiversCallback (Ptr<Packet const> packet, uint32_t systemId, uint32_t SenderID, double frequencyMHz, uint8_t sf)
{
  // NS_LOG_INFO ("A packet was lost because there were no more receivers at gateway " << systemId);

  //NS_LOG_INFO ( "D " << systemId << " " << SenderID << " " << packet->GetSize () << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds ());
  //PrintTrace (GD, systemId, SenderID, packet->GetSize (), frequencyMHz, sf, Seconds(0), Seconds(0), 0, "scratch/Trace.dat");

  LoraTag tag;
  packet->PeekPacketTag (tag);
  uint8_t jammer = tag.GetJammer ();
  //packet->AddPacketTag (tag);

  pkt_drop_ed [SenderID] += 1;
  pkt_drop_gw [systemId-nDevices-nJammers] ++;

  if (jammer == uint8_t(0))
	  {
	  	  dropped_ed += 1;
	  }

  else
	  {
		 dropped_jm += 1;
	  }
}

void
UnderSensitivityCallback (Ptr<Packet const> packet, uint32_t systemId, uint32_t SenderID, double frequencyMHz, uint8_t sf)
{
  NS_LOG_INFO ("A packet arrived at the gateway under sensitivity at gateway " << systemId);
  //NS_LOG_INFO ( "U " << systemId << " " << SenderID << " " << packet->GetSize () << " " << frequencyMHz << " " << unsigned(sf) << " " << Simulator::Now ().GetSeconds ());
  //PrintTrace (U, systemId, SenderID, packet->GetSize (), frequencyMHz, sf, Seconds(0), Seconds(0), 0, "scratch/Trace.dat");

  LoraTag tag;
  packet->PeekPacketTag (tag);
  uint8_t jammer = tag.GetJammer ();
  //packet->AddPacketTag (tag);

	  if (jammer == uint8_t(0))
		  {
		  	  underSensitivity_ed += 1;
		  }

	  else
		  {
		  	  underSensitivity_jm += 1;
		  }
}

void
EnergyConsumptionCallback (uint32_t NodeId, int ConsoType, double event_conso, double battery_level)
{
  // NS_LOG_INFO ("The energy consumption of Node " << NodeId << event_conso << "Conso type " << " " << ConsoType << "at " << Simulator::Now ().GetSeconds ());
	//Conso Type: 1 for TX, 2 for RX, 3 for Standby and 4 Sleep

  //cout << "E " << NodeId << " " << ConsoType << " " << event_conso << " " << " " << battery_level << " " << Simulator::Now ().GetSeconds () << endl;

}

void
DeadDeviceCallback (uint32_t NodeId, double cumulative_tx_conso, double cumulative_rx_conso, double cumulative_stb_conso, double cumulative_sleep_conso, Time dead_time)
{
  // NS_LOG_INFO ("The Node " << NodeId << "was dead at " << dead_time.GetSeconds () << "at " << Simulator::Now ().GetSeconds ());
		//Conso Type: 1 for TX, 2 for RX, 3 for Standby and 4 Sleep
}

void
NSReceiveCallback (vector<uint32_t> ED_RX, vector<uint32_t> ED_RXD, vector<uint32_t> GW_RX, vector<uint32_t> GW_RXD)
{
	//NS_LOG_INFO ("ED_RX " << unsigned(ED_RX[0]) << " ED_RX " << unsigned (ED_RX[1]) << " GW_RX " << unsigned (GW_RX[0]) << " GW_RX " << unsigned (GW_RX[1]));


	for (uint32_t i = 0; i != GW_RX.size(); i++)
		pkt_success_gw[i] = GW_RX[i];

	for (uint32_t i = 0; i != ED_RX.size(); i++)
		pkt_success_ed [i] = ED_RX[i];
		//}

	for (uint32_t i = 0; i < ED_RX.size(); i++)
	   {
		NS_LOG_INFO ("pos dev " << unsigned(i) << " value " << ED_RX[i]);
	   }

	for (uint32_t i = 0; i < GW_RX.size(); i++)
	   {
		NS_LOG_INFO ("pos gate " << unsigned(i) << " value " << GW_RX[i]);
	   }
}

void
NSInterArrivalTime (vector<vector<double> > arrival, vector<vector<double> > inter_arrival,
		vector<vector<double> > ucl, vector<vector<double> > lcl, vector<vector<double> > ewma)
{
	NS_LOG_INFO ("Arrival Time ");

	for (uint32_t j = 0; j < nDevices; j++)
	{
		for (uint32_t i = 0; i < arrival[j].size(); i++)
		{
			NS_LOG_INFO ("node " << j << " arrival " << arrival[j][i] << " interarrival " << inter_arrival[j][i]
						 << " EWMA " << ewma[j][i] << " UCL " << ucl[j][i] << " LCL " << lcl[j][i]);
		}
	}
}

void
NSEWMA(vector<double> ucl, vector<double> lcl)
{
	NS_LOG_INFO ("Learned parameters ");


}


void
PrintEndDevices (NodeContainer endDevices, NodeContainer Jammers, NodeContainer gateways, string filename)
{
  const char * c = filename.c_str ();
  ofstream Plot;
  Plot.open (c);

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
      uint32_t DeviceID = object->GetId();
      Plot << "ED " << DeviceID << " " << pos.x << " " << pos.y << " " << sf << " " << 	pkt_send [DeviceID] << " "<< pkt_success_ed [DeviceID] << " " << pkt_loss_ed [DeviceID] << " " << pkt_drop_ed [DeviceID]<< endl;

    }

  for (NodeContainer::Iterator j = Jammers.Begin (); j != Jammers.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<JammerLoraMac> mac = loraNetDevice->GetMac ()->GetObject<JammerLoraMac> ();
      int sf = int(mac->GetDataRate ());
      Vector pos = position->GetPosition ();
      uint32_t DeviceID = object->GetId();
      Plot << "JM " << DeviceID << " " << pos.x << " " << pos.y << " " << sf << " " << pkt_send [DeviceID] << " " << pkt_success_ed [DeviceID] << " " << pkt_loss_ed [DeviceID] << " " << pkt_drop_ed [DeviceID]<< endl;

    }

  // Also print the gateways
  for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      Vector pos = position->GetPosition ();
      //Plot <<"GW " << pos.x << " " << pos.y << endl;
    }
  Plot.close ();
}


int main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.AddValue ("nDevices", "Number of end devices to include in the simulation", nDevices);
  cmd.AddValue ("nJammers", "Number of Jammers to include in the simulation", nJammers);
  cmd.AddValue ("nGateways", "Number of Gateways to include in the simulation", nGateways);
  cmd.AddValue ("Conf_UP", "Confirmed data UP", Conf_UP);
  cmd.AddValue ("radius", "radius of the disc where nodes will be deployed", radius);
  cmd.AddValue ("simulationTime", "The time for which to simulate", simulationTime);
  cmd.AddValue ("appPeriod", "The period in seconds to be used by periodically transmitting applications", appPeriodSeconds);
  cmd.AddValue ("appPeriodJam", "The period in seconds to be used by periodically transmitting applications", appPeriodJamSeconds);
  cmd.AddValue ("printEDs", "Whether or not to print a file containing the ED's positions", printEDs);
  cmd.AddValue ("PayloadSize", "Payload size of the Packet - Lora Node", PayloadSize);
  cmd.AddValue ("PayloadJamSize", "Payload size of the Packet - Jamming Node", PayloadJamSize);
  cmd.AddValue ("JammerType", "Attacker Profile", JammerType);
  cmd.AddValue ("JammerFrequency", "Jammer Frequency in MHz (if Type 2: the frequency the device is listening for; if type 3: the frequency the device will transmit)", JammerFrequency);
  cmd.AddValue ("JammerSF", "Jammer SF, if not random", JammerSF);
  cmd.AddValue ("JammerTxPower", "Jammer TX Poxer in dBm ", JammerTxPower);
  cmd.AddValue ("Random_SF", "Boolean variable to set whether the Jammer select a random SF to transmit", Random_SF);
  cmd.AddValue ("All_SF", "Boolean variable to set whether the Jammer transmits in all SF at the same time (Jammers 3 and 4)", All_SF);
  cmd.AddValue ("JammerDutyCycle", "Jammer duty cycle", JammerDutyCycle);
  cmd.AddValue ("Exponential", "Exponential inter-arrival time", Exponential);

  // imput variables related to ED
  cmd.AddValue ("Specific_SF", " boolean variable indicating if EDs use an specific Spreading Factor", Specific_SF);
  cmd.AddValue ("ED_SF", "ED's Spreading Factor", ED_SF);

  // imput variables related to the NS
  cmd.AddValue ("InterArrival", "Boolean variable to set whether or not the Network server computes the IAT", InterArrival);
  cmd.AddValue ("Net_Ser", "Network Server", Net_Ser);
  cmd.AddValue ("NS_buffer", "Length of Network Server Buffer", NS_buffer);
  cmd.AddValue ("lambda", "lambda parameter for the EWMA algorithm btw 0-1 ", lambda);

  // authenticated preamble
  cmd.AddValue ( "authpre", "Authenticated preambles ", authpre);

  // Interference model

  cmd.AddValue ("Int_Model", "1 - ALOHA, 2 - Co-channel", Int_Model);
  cmd.AddValue ("Filename", "Filename", Filename);
  cmd.AddValue ("Path", "Path", Path);

  cmd.Parse (argc, argv);

  pkt_success_ed.resize(nDevices+nJammers, 0);
  pkt_drop_ed.resize(nDevices+nJammers, 0);
  pkt_loss_ed.resize(nDevices+nJammers, 0);
  pkt_send.resize(nDevices+nJammers, 0);

  pkt_loss_gw.resize(nGateways,0);
  pkt_drop_gw.resize(nGateways,0);
  pkt_success_gw.resize(nGateways,0);

//	Set up logging
//  LogComponentEnable ("LorawanNetworkAttackExample", LOG_LEVEL_ALL);
//  LogComponentEnable("LoraChannel", LOG_LEVEL_ALL);
//  LogComponentEnable("LoraPhy", LOG_LEVEL_ALL);
//  LogComponentEnable("EndDeviceLoraPhy", LOG_LEVEL_ALL);
//  LogComponentEnable("JammerLoraPhy", LOG_LEVEL_ALL);
//  LogComponentEnable("GatewayLoraPhy", LOG_LEVEL_ALL);
//  LogComponentEnable("SimpleNetworkServer", LOG_LEVEL_ALL);
//  LogComponentEnable("AppJammer", LOG_LEVEL_ALL);
//  LogComponentEnable("LoraInterferenceHelper", LOG_LEVEL_ALL);
//  LogComponentEnable("LoraMac", LOG_LEVEL_ALL);
//  LogComponentEnable("EndDeviceLoraMac", LOG_LEVEL_ALL);
//  LogComponentEnable("JammerLoraMac", LOG_LEVEL_ALL);
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
//  LogComponentEnable("LoraEnergyConsumptionHelper", LOG_LEVEL_ALL);

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
  loss->SetReference (1, 7.7);

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
      position.z = 10;
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
                                       MakeCallback (&EDTransmissionCallback));
      phy->TraceConnectWithoutContext ("EnergyConsumptionCallback",
                                       MakeCallback (&EnergyConsumptionCallback));
      phy->TraceConnectWithoutContext ("DeadDeviceCallback",
                                       MakeCallback (&DeadDeviceCallback));
      phy->TraceConnectWithoutContext ("ReceivedPacket",
                                         MakeCallback (&EnDeviceReceiveCallback));
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
                                       MakeCallback (&JMTransmissionCallback));
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
      position.x = 0;
      position.y = 0;
      position.z = 1;
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

      // Set up the interference model of the simulation

      gwPhy->SetInterferenceModel (Int_Model);

      if (authpre){
    	  gwPhy->Authpreamble();
      }

      // Set up height of the gateway
      Ptr<MobilityModel> gwMob = (*j)->GetObject<MobilityModel> ();
      Vector position = gwMob->GetPosition ();
      position.z = 15;
      gwMob->SetPosition (position);

      // Global callbacks (every gateway)
      gwPhy->TraceConnectWithoutContext ("ReceivedPacket",
                                         MakeCallback (&GatewayReceiveCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseInterference",
                                         MakeCallback (&CollisionCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseNoMoreReceivers",
                                         MakeCallback (&NoMoreReceiversCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseUnderSensitivity",
                                         MakeCallback (&UnderSensitivityCallback));
      gwPhy->TraceConnectWithoutContext ("DurationCallback",
                                       MakeCallback (&GWReceivedurationCallback));
      gwPhy->TraceConnectWithoutContext ("CaptureEffectCallback",
                                       MakeCallback (&GWCaptureEffectCallback));
    }


  /**********************************************
  *  Set up the end device's spreading factor  *
  **********************************************/

  if (Specific_SF){
	  macHelper.SetSpreadingFactors(endDevices,ED_SF);
  }else
  {
	  macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);
  }


  //  NS_LOG_DEBUG ("Completed configuration");


  /***********************************
  *  Set up the jammer's parameters  *
  ************************************/

  // Create the AttackHelper
  AttackHelper AttackProfile = AttackHelper ();

  AttackProfile.SetType (Jammers, JammerType);
  AttackProfile.ConfigureForEuRegionJm (Jammers);
  //AttackProfile.ConfigureBand (Jammers, 868, 868.6, JammerDutyCycle, 14, 802.3, 0, 5);
  AttackProfile.SetFrequency(Jammers,JammerFrequency);
  AttackProfile.SetTxPower(Jammers,JammerTxPower);
  AttackProfile.SetPacketSize (Jammers,PayloadJamSize);

  /*********************************************
  *  Install applications on the Jammer *
  *********************************************/


  if (JammerType == 3  || JammerType == 4 )
  {
	  Time appJamStopTime = Seconds (simulationTime);
	  AppJammerHelper appJamHelper = AppJammerHelper ();

	  AttackProfile.ConfigureBand (Jammers, JammerDutyCycle);

	  appJamHelper.SetPacketSize (PayloadJamSize);
   	  appJamHelper.SetPeriod (Seconds (appPeriodJamSeconds));
   	  appJamHelper.SetDC (JammerDutyCycle);
   	  appJamHelper.SetExp (Exponential);
   	  appJamHelper.SetRanSF (Random_SF);
   	  appJamHelper.SetSpreadingFactor (JammerSF);

	  ApplicationContainer appJamContainer = appJamHelper.Install (Jammers);

	  appJamContainer.Start (Seconds (0));
	  appJamContainer.Stop (appJamStopTime);

  }

  /*********************************************
  *  Install applications on the end devices  *
  *********************************************/

  Time appStopTime = Seconds (simulationTime);
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  appHelper.SetPeriod (Seconds (appPeriodSeconds));
  appHelper.SetPacketSize (PayloadSize);

  appHelper.SetExp (Exponential);
  appHelper.SetSpreadingFactor (ED_SF);

  ApplicationContainer appContainer = appHelper.Install (endDevices);

  appContainer.Start (Seconds (0));
  appContainer.Stop (appStopTime);


  ///////////////
  // Create NS //
  ///////////////

  if (Net_Ser == true)
  {

	  NodeContainer networkServers;
	  networkServers.Create (1);

	  // Install the SimpleNetworkServer application on the network server
	  NetworkServerHelper networkServerHelper;
	  if (InterArrival){networkServerHelper.SetInterArrival();}

	  //Set parameters for EWMA, target = application period, buffer_length
	  networkServerHelper.SetEWMA (NS_buffer, ewma_training, appPeriod.GetSeconds(), lambda, ucl, lcl);
	  networkServerHelper.SetStopTime (Seconds(simulationTime));
	  networkServerHelper.SetGateways (gateways);
	  networkServerHelper.SetJammers (Jammers);
	  networkServerHelper.SetEndDevices (endDevices);
	  networkServerHelper.Install (networkServers);

	  // Install the Forwarder application on the gateways
	  ForwarderHelper forwarderHelper;
	  forwarderHelper.Install (gateways);

	  if (Conf_UP == true)
	  {
		  macHelper.SetMType (endDevices, LoraMacHeader::CONFIRMED_DATA_UP);
	  }
	  else
	  {
		  macHelper.SetMType (endDevices, LoraMacHeader::UNCONFIRMED_DATA_UP);
	  }

	  Ptr<SimpleNetworkServer> ns = networkServerHelper.GetNS();

      ns->TraceConnectWithoutContext ("ReceivePacket",
                                       MakeCallback (&NSReceiveCallback));
      ns->TraceConnectWithoutContext ("InterArrivalTime",
                                       MakeCallback (&NSInterArrivalTime));
      ns->TraceConnectWithoutContext ("EwmaParameters",
                                       MakeCallback (&NSEWMA));
  }

  /****************
  *  Simulation  *
  ****************/

  Simulator::Stop (Seconds (simulationTime*3));

  // PrintSimulationTime ();

  Simulator::Run ();

  if (printEDs)
    {
	  PrintEndDevices (endDevices, Jammers, gateways, "scratch/Devices.dat");
    }

  Simulator::Destroy ();


// *************
// *  Results  *
// *************


  double receivedProb_ed = gwreceived_ed/(edsent);
  double collisionProb_ed = collision_ed/(edsent);
  double noMoreReceiversProb_ed = dropped_ed/(edsent);
  double underSensitivityProb_ed = underSensitivity_ed/(edsent);

  double receivedProb_jm = gwreceived_jm/(jmsent);
  double collisionProb_jm = collision_jm/(jmsent);
  double noMoreReceiversProb_jm = dropped_jm/(jmsent);
  double underSensitivityProb_jm = underSensitivity_jm/(jmsent);

  //  double receivedProbGivenAboveSensitivity = gwreceived/(edsent - underSensitivity);
  //  double interferedProbGivenAboveSensitivity = collision/(edsent - underSensitivity);
  //  double noMoreReceiversProbGivenAboveSensitivity = noMoreReceivers/(edsent - underSensitivity);

  //  cout << edsent << " " << gwreceived << " " << collision << " " << dropped << " " << noMoreReceiversProb << " " << underSensitivityProb << endl;

  //  cout << nDevices <<  " " << collision_ed << " " << dropped_ed << " " << gwreceived_ed << " " << underSensitivity_ed << " " << edsent << " "  << collision_ed + dropped_ed + gwreceived_ed + underSensitivity_ed  << " " << collisionProb_ed << " " << noMoreReceiversProb_ed  << " " << receivedProb_ed << endl;

  //  cout << nJammers  << " " << collision_jm << " " << dropped_jm << " " << gwreceived_jm << " " << underSensitivity_jm << " " << jmsent << " "  << collision_jm + dropped_jm + gwreceived_jm + underSensitivity_jm  << " " << collisionProb_jm << " " << noMoreReceiversProb_jm  << " " << receivedProb_jm << endl;

	  string Result_File = Path + "/" + Filename;

	  cout << "Jammer Type " << JammerType << endl;
	  cout << "Jammer DutyCycle " << JammerDutyCycle << endl;
	  cout << "Number of Jammers " << nJammers << endl;
	  cout << "Number of Devices " << nDevices << endl;
	  cout << "Sent ed " << edsent << endl;
	  cout << "Sent jm " << jmsent << endl;
	  cout << "Success ed " << gwreceived_ed << endl;
	  cout << "Success jm " << gwreceived_jm << endl;
	  cout << "collision ed " << collision_ed << endl;
	  cout << "collision jm " << collision_jm << endl;
	  cout << "underSensitivity ed " << underSensitivity_ed << endl;
	  cout << "underSensitivity jm " << underSensitivity_jm << endl;
	  cout << "cumulative time ed " << cumulative_time_ed << endl;
	  cout << "cumulative time jm " << cumulative_time_jm << endl;

	  cout << "dropped ed " << dropped_ed << endl;
	  cout << "dropped jm " << dropped_jm << endl;

	  for (uint32_t i = 0; i != nGateways; i++)
	    {

		  cout << "loss GW - " << i <<  " " << pkt_loss_gw [i] << endl;
		  cout << "drop GW - " << i <<  " "  << pkt_drop_gw [i] << endl;
		  cout << "received GW - " << i <<  " " << pkt_success_gw [i] << endl;

	    }

	   cout << "success ED - " << accumulate(pkt_success_ed.begin(), pkt_success_ed.end(), 0) << endl;


	 // for(uint32_t i = 0; i < pkt_success_ed.size(); i++)
	 //   {
	 //	  	cout << "success ED - " << i <<  " " << pkt_success_ed [i] << endl;
	 //   	 cout << pkt_success_ed[i] << endl;
	 //   }


	  PrintResults ( nGateways, nDevices, nJammers, receivedProb_ed, collisionProb_ed, noMoreReceiversProb_ed,
			  underSensitivityProb_ed, receivedProb_jm, collisionProb_jm, noMoreReceiversProb_jm,
			  underSensitivityProb_jm, gwreceived_ed, gwreceived_jm, edsent, jmsent, cumulative_time_ed,
			  cumulative_time_jm, ce_ed, ce_jm, Result_File);


  return 0;

}

