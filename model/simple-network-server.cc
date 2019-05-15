/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Padova
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 */

#include "ns3/simple-network-server.h"
#include "ns3/simulator.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/lora-mac-header.h"
#include "ns3/lora-frame-header.h"
#include "ns3/lora-tag.h"
#include "ns3/log.h"
#include <vector>
#include <algorithm>

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimpleNetworkServer");

NS_OBJECT_ENSURE_REGISTERED (SimpleNetworkServer);

TypeId
SimpleNetworkServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleNetworkServer")
    .SetParent<Application> ()
    .AddConstructor<SimpleNetworkServer> ()
    .SetGroupName ("lorawan")
	.AddTraceSource ("ReceivePacket",
                   "Trace source indicating a packet"
                   "reception with information concerning"
                   "PktID, EDID, GWID and time stamp",
                   MakeTraceSourceAccessor
                     (&SimpleNetworkServer::m_packetrx))
	.AddTraceSource ("InterArrivalTime",
					 "Trace source the inter-arrival and"
					 "arrival time of each End-Device",
				   MakeTraceSourceAccessor
					 (&SimpleNetworkServer::m_arrivaltime))
					 ;
  return tid;
}

SimpleNetworkServer::SimpleNetworkServer():

		m_devices(0),
		m_gateways(0),
		m_jammers(0),
		m_devices_pktid(0),
		m_stop_time(0),
		m_interarrivaltime(false),
		m_devices_interarrivaltime(0),
		m_last_arrivaltime_known(0),
		m_devices_ewma(0),
		m_devices_ewma_total(0),
		m_buffer_length(0),
		m_target(0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

SimpleNetworkServer::~SimpleNetworkServer()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
SimpleNetworkServer::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
SimpleNetworkServer::StopApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_INFO ("Stop at  " << Simulator::Now ().GetSeconds ());

  //Fire the trace sources
  m_packetrx(m_devices_pktreceive,m_devices_pktduplicate,m_gateways_pktreceive,m_gateways_pktduplicate);
  m_arrivaltime(m_devices_arrivaltime_total,m_devices_interarrivaltime_total,m_devices_ucl,m_devices_lcl,m_devices_ewma_total);

}

void
SimpleNetworkServer::SetStopTime (Time Stop)
{
	  m_stop_time = Stop;
	  Simulator::Schedule (m_stop_time,
	                       &SimpleNetworkServer::StopApplication, this);

}

void
SimpleNetworkServer::SetParameters (uint32_t GW, uint32_t ED, uint32_t JM, int buffer_length, double target, double lambda)
{

	  NS_LOG_INFO ("Number of ED " << ED);
	  NS_LOG_INFO ("Number of GW " << GW);
	  NS_LOG_INFO ("Number of GW " << JM);

	  m_devices = ED;
	  m_gateways = GW;
	  m_jammers = JM;

	  m_buffer_length = buffer_length;
	  m_target = target;
	  m_lambda = lambda;


	  // Initialize the end-device related vectors
	  m_devices_pktid.resize(m_devices, vector<uint32_t>(m_buffer_length));
	  m_devices_pktreceive.resize(m_devices,0);
	  m_devices_pktduplicate.resize(m_devices,0);

	  // Initialize the gateway related vectors

	  m_gateways_pktreceive.resize(m_gateways,0);
	  m_gateways_pktduplicate.resize(m_gateways,0);

	  //Initialize the vectors related to the inter-arrival time

	  m_devices_interarrivaltime.resize(m_devices, vector<double>(m_buffer_length));
	  m_devices_arrivaltime.resize(m_devices,vector<double>(m_buffer_length));
	  m_last_arrivaltime_known.resize(m_devices,double(0));

	  //Initialize the vectors related to EWMA to detect attacks
	  m_devices_ewma.resize(m_devices, vector<double>(m_buffer_length));

	  m_ucl.resize(m_devices,0);
	  m_lcl.resize(m_devices,0);

	  // ucl, lcl and ewma for tracing purposes

	  m_devices_ucl.resize(m_devices, vector<double>(0));
	  m_devices_lcl.resize(m_devices, vector<double>(0));
	  m_devices_ewma_total.resize(m_devices, vector<double>(0));
	  m_devices_interarrivaltime_total.resize(m_devices, vector<double>(0));
	  m_devices_arrivaltime_total.resize(m_devices, vector<double>(0));

}

void
SimpleNetworkServer::SetInterArrival (void)
{
	m_interarrivaltime = true;
}

void
SimpleNetworkServer::AddGateway (Ptr<Node> gateway, Ptr<NetDevice> netDevice)
{
  NS_LOG_FUNCTION (this << gateway);

  // Get the PointToPointNetDevice
  Ptr<PointToPointNetDevice> p2pNetDevice;
  for (uint32_t i = 0; i < gateway->GetNDevices (); i++)
    {
      p2pNetDevice = gateway->GetDevice (i)->GetObject<PointToPointNetDevice> ();
      if (p2pNetDevice != 0)
        {
          // We found a p2pNetDevice on the gateway
          break;
        }
    }

  // Get the gateway's LoRa MAC layer (assumes gateway's MAC is configured as first device)
  Ptr<GatewayLoraMac> gwMac = gateway->GetDevice (0)->GetObject<LoraNetDevice> ()->
    GetMac ()->GetObject<GatewayLoraMac> ();
  NS_ASSERT (gwMac != 0);

  // Get the Address
  Address gatewayAddress = p2pNetDevice->GetAddress ();

  // Check whether this device already exists
  if (m_gatewayStatuses.find (gatewayAddress) == m_gatewayStatuses.end ())
    {
      // The device doesn't exist

      // Create new gatewayStatus
      GatewayStatus gwStatus = GatewayStatus (gatewayAddress, netDevice, gwMac);
      // Add it to the map
      m_gatewayStatuses.insert (std::pair<Address, GatewayStatus>
                                  (gatewayAddress, gwStatus));
      NS_LOG_DEBUG ("Added a gateway to the list");
    }
}

void
SimpleNetworkServer::AddNodes (NodeContainer nodes)
{
  NS_LOG_FUNCTION_NOARGS ();

  // For each node in the container, call the function to add that single node
  NodeContainer::Iterator it;
  for (it = nodes.Begin (); it != nodes.End (); it++)
    {
      AddNode (*it);
    }
}

void
SimpleNetworkServer::AddNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);

  // Get the LoraNetDevice
  Ptr<LoraNetDevice> loraNetDevice;
  for (uint32_t i = 0; i < node->GetNDevices (); i++)
    {
      loraNetDevice = node->GetDevice (i)->GetObject<LoraNetDevice> ();
      if (loraNetDevice != 0)
        {
          // We found a LoraNetDevice on the node
          break;
        }
    }
  // Get the MAC
  Ptr<EndDeviceLoraMac> edLoraMac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();

  // Get the Address
  LoraDeviceAddress deviceAddress = edLoraMac->GetDeviceAddress ();
  // Check whether this device already exists
  if (m_deviceStatuses.find (deviceAddress) == m_deviceStatuses.end ())
    {
      // The device doesn't exist

      // Create new DeviceStatus
      DeviceStatus devStatus = DeviceStatus (edLoraMac);
      // Add it to the map
      m_deviceStatuses.insert (std::pair<LoraDeviceAddress, DeviceStatus>
                                 (deviceAddress, devStatus));
      NS_LOG_DEBUG ("Added to the list a device with address " <<
                    deviceAddress.Print ());
    }
}

bool
SimpleNetworkServer::Receive (Ptr<NetDevice> device, Ptr<const Packet> packet,
                              uint16_t protocol, const Address& address)
{
  NS_LOG_FUNCTION (this << packet << protocol << address);

  // Create a copy of the packet
  Ptr<Packet> myPacket = packet->Copy ();

  // Extract the headers
  LoraMacHeader macHdr;
  myPacket->RemoveHeader (macHdr);
  LoraFrameHeader frameHdr;
  frameHdr.SetAsUplink ();
  myPacket->RemoveHeader (frameHdr);
  LoraTag tag;
  myPacket->RemovePacketTag (tag);

  // Get the packet ID and Gw ID
  uint32_t pkt_ID = tag.GetPktID();
  uint32_t gw_ID = tag.GetGWID();
  uint32_t ed_ID = tag.GetSenderID();

  PacketCounter(pkt_ID,gw_ID,ed_ID);

  // Register which gateway this packet came from
  double rcvPower = tag.GetReceivePower ();
  m_deviceStatuses.at (frameHdr.GetAddress ()).UpdateGatewayData (address, rcvPower);

  // Determine whether the packet requires a reply
  if (macHdr.GetMType () == LoraMacHeader::CONFIRMED_DATA_UP
		  //&&      !m_deviceStatuses.at (frameHdr.GetAddress ()).HasReply ()
		  )
    {
     NS_LOG_DEBUG ("Scheduling a reply for this device");

     DeviceStatus::Reply reply;
     reply.hasReply = true;

     LoraMacHeader replyMacHdr = LoraMacHeader ();
     replyMacHdr.SetMajor (0);
     replyMacHdr.SetMType (LoraMacHeader::UNCONFIRMED_DATA_DOWN);
     reply.macHeader = replyMacHdr;

     LoraFrameHeader replyFrameHdr = LoraFrameHeader ();
     replyFrameHdr.SetAsDownlink ();
     replyFrameHdr.SetAddress (frameHdr.GetAddress ());
     replyFrameHdr.SetAck (true);
     reply.frameHeader = replyFrameHdr;

     Ptr<Packet> replyPacket = Create<Packet> (10);
     reply.packet = replyPacket;

     m_deviceStatuses.at (frameHdr.GetAddress ()).SetReply (reply);
     m_deviceStatuses.at (frameHdr.GetAddress ()).SetFirstReceiveWindowFrequency (tag.GetFrequency ());

     // Schedule a reply on the first receive window
     Simulator::Schedule (Seconds (1), &SimpleNetworkServer::SendOnFirstWindow,
                           this, frameHdr.GetAddress ());
    }
  return true;
}

void
SimpleNetworkServer::SendOnFirstWindow (LoraDeviceAddress address)
{
  NS_LOG_FUNCTION (this << address);

  // Decide on which gateway we'll transmit our reply
  double firstReceiveWindowFrequency = m_deviceStatuses.at
      (address).GetFirstReceiveWindowFrequency ();

  Address gatewayForReply = GetGatewayForReply (address,
                                                firstReceiveWindowFrequency);

  if (gatewayForReply != Address ())
    {
      NS_LOG_FUNCTION ("Found a suitable GW!");

      // Get the packet to use in the reply
      Ptr<Packet> replyPacket = m_deviceStatuses.at (address).GetReplyPacket ();
      NS_LOG_DEBUG ("Packet size: " << replyPacket->GetSize ());

      // Tag the packet so that the Gateway sends it according to the first
      // receive window parameters
      LoraTag replyPacketTag;
      uint8_t dataRate = m_deviceStatuses.at (address).GetFirstReceiveWindowDataRate ();
      double frequency = m_deviceStatuses.at (address).GetFirstReceiveWindowFrequency ();
      replyPacketTag.SetDataRate (dataRate);
      replyPacketTag.SetFrequency (frequency);

      replyPacket->AddPacketTag (replyPacketTag);

      NS_LOG_INFO ("Sending reply through the gateway with address " << gatewayForReply);

      // Inform the gateway of the transmission
      m_gatewayStatuses.find (gatewayForReply)->second.GetNetDevice ()->
      Send (replyPacket, gatewayForReply, 0x0800);
    }
  else
    {
      NS_LOG_FUNCTION ("No suitable GW found, scheduling second window reply");

      // Schedule a reply on the second receive window
      Simulator::Schedule (Seconds (1), &SimpleNetworkServer::SendOnSecondWindow, this,
                           address);
    }
}

void
SimpleNetworkServer::SendOnSecondWindow (LoraDeviceAddress address)
{
  NS_LOG_FUNCTION (this << address);

  double secondReceiveWindowFrequency = m_deviceStatuses.at
      (address).GetSecondReceiveWindowFrequency ();

  // Decide on which gateway we'll transmit our reply
  Address gatewayForReply = GetGatewayForReply (address, secondReceiveWindowFrequency);

  if (gatewayForReply != Address ())
    {
      // Get the packet to use in the reply
      Ptr<Packet> replyPacket = m_deviceStatuses.at (address).GetReplyPacket ();
      NS_LOG_DEBUG ("Packet size: " << replyPacket->GetSize ());

      // Tag the packet so that the Gateway sends it according to the first
      // receive window parameters
      LoraTag replyPacketTag;
      uint8_t dataRate = m_deviceStatuses.at (address).GetSecondReceiveWindowDataRate ();
      double frequency = m_deviceStatuses.at (address).GetSecondReceiveWindowFrequency ();
      replyPacketTag.SetDataRate (dataRate);
      replyPacketTag.SetFrequency (frequency);

      replyPacket->AddPacketTag (replyPacketTag);

      NS_LOG_INFO ("Sending reply through the gateway with address " <<
                   gatewayForReply);

      // Inform the gateway of the transmission
      m_gatewayStatuses.find (gatewayForReply)->second.GetNetDevice ()->
      Send (replyPacket, gatewayForReply, 0x0800);
    }
  else
    {
      // Schedule a reply on the second receive window
      NS_LOG_INFO ("Giving up on this reply, no GW available for second window");
    }
}

Address
SimpleNetworkServer::GetGatewayForReply (LoraDeviceAddress deviceAddress,
                                         double frequency)
{
  NS_LOG_FUNCTION (this);

  // Check which gateways can send this reply
  // Go in the order suggested by the DeviceStatus
  std::list<Address> addresses = m_deviceStatuses.at
      (deviceAddress).GetSortedGatewayAddresses ();

  for (auto it = addresses.begin (); it != addresses.end (); ++it)
    {
      if (m_gatewayStatuses.at (*it).IsAvailableForTransmission (frequency))
        {
          m_gatewayStatuses.at (*it).SetNextTransmissionTime (Simulator::Now ());
          return *it;
        }
    }

  return Address ();
}

void
SimpleNetworkServer::PacketCounter(uint32_t pkt_ID, uint32_t gw_ID, uint32_t ed_ID)
{
	NS_LOG_INFO ("End-Device ID " << unsigned(ed_ID));
	NS_LOG_INFO ("Gateway ID " << unsigned(gw_ID));
	NS_LOG_INFO ("Packet ID " << unsigned(pkt_ID));

	// Verify if the packet has been already received or not

	bool AR = AlreadyReceived(m_devices_pktid[ed_ID],pkt_ID);

	// Increase the corresponding receive counter (GW and ED)

	if (not (AR))
	{
		m_devices_pktreceive[ed_ID] ++;
		// Send this information to compute the Inter-Arrival Time
		if (m_interarrivaltime == true) {
			  InterArrivalTime (ed_ID, Simulator::Now ().GetSeconds ());
		}
	}
	else
	{
		m_devices_pktduplicate[ed_ID]++;
		m_gateways_pktduplicate[gw_ID-m_devices-m_jammers]++;
	}

	m_gateways_pktreceive[gw_ID-m_devices-m_jammers] ++;

	// insert the Packet ID on at the end of the temporal matrix

	for (uint32_t i = 1; i < m_devices_pktid[ed_ID].size(); i++)
	 {
		m_devices_pktid[ed_ID][i-1] = m_devices_pktid[ed_ID][i];
	 }

	m_devices_pktid[ed_ID][m_devices_pktid[ed_ID].size()-1] = pkt_ID;

//	for (uint32_t i = 0; i < m_devices_pktid[ed_ID].size(); i++)
//	   {
//		NS_LOG_INFO ("pos " << unsigned(i) << " value " << m_devices_pktid[ed_ID][i]);
//	   }


//	for (uint32_t i = 0; i < m_devices_pktreceive.size(); i++)
//	   {
//		NS_LOG_INFO ("pos dev " << unsigned(i) << " value " << m_devices_pktreceive[i]);
//	   }

//	for (uint32_t i = 0; i < m_gateways_pktreceive.size(); i++)
//	   {
//		NS_LOG_INFO ("pos gate " << unsigned(i) << " value " << m_gateways_pktreceive[i]);
//	   }

}

void
SimpleNetworkServer::InterArrivalTime(uint32_t ed_ID, double arrival_time)
{

	// Only packet that hasn't been received arrive here!
	NS_LOG_FUNCTION ("Ready to calculate the IAT - ED "  << ed_ID << " Time " << arrival_time << " Size " << m_devices_arrivaltime[ed_ID].size());


	// shift the corresponding vectors

	for (uint32_t i = 1; i < m_devices_arrivaltime[ed_ID].size(); i++)
	 {
		m_devices_arrivaltime[ed_ID][i-1] = m_devices_arrivaltime[ed_ID][i];
		m_devices_interarrivaltime[ed_ID][i-1] = m_devices_interarrivaltime[ed_ID][i];
	 }

	// compute the inter-arrival time of this packet
	m_devices_arrivaltime[ed_ID][m_devices_arrivaltime[ed_ID].size()-1] = arrival_time;
	m_devices_interarrivaltime[ed_ID][m_devices_interarrivaltime[ed_ID].size()-1] = arrival_time - m_last_arrivaltime_known[ed_ID];

	// set the arrival and inter arrival time for tracing purposes
	m_devices_arrivaltime_total[ed_ID].push_back(arrival_time);
	m_devices_interarrivaltime_total[ed_ID].push_back(arrival_time - m_last_arrivaltime_known[ed_ID]);

	// update the last received arrival time vector

	m_last_arrivaltime_known[ed_ID] = arrival_time;

	//for (uint32_t i = 0; i < m_devices_arrivaltime[ed_ID].size(); i++)
	//   {
	//	NS_LOG_INFO ("arrival " << m_devices_arrivaltime[ed_ID][i] << " interarrival " << m_devices_interarrivaltime[ed_ID][i]);
	//   }

	// Compute the EWMA

	EWMA(m_devices_interarrivaltime[ed_ID],ed_ID);


}

void
SimpleNetworkServer::EWMA(vector<double> IAT, uint32_t ed_ID)
{

	// Only packet that hasn't been received arrive here!
	NS_LOG_FUNCTION ("Ready to calculate the EWMA - Size "  << m_devices_ewma[ed_ID].size());

    // shift the corresponding vectors

	for (uint32_t i = 1; i < m_devices_ewma[ed_ID].size(); i++)
	 {
		m_devices_ewma[ed_ID][i-1] = m_devices_ewma[ed_ID][i];
	 }

	// compute the EWMA of this packet

	double ewma = m_lambda*IAT.back()+(1-m_lambda)*m_devices_ewma[ed_ID][m_devices_ewma[ed_ID].size()-1];

	// push back the ewma (only for tracing purposes)
	m_devices_ewma_total[ed_ID].push_back(ewma);

	// insert the value at the end of the corresponding vector

	m_devices_ewma[ed_ID][m_devices_ewma[ed_ID].size()-1] = ewma;

	for (uint32_t i = 0; i < m_devices_ewma[ed_ID].size(); i++)
	 {
	   NS_LOG_INFO ("ewma " << m_devices_ewma[ed_ID][i] << " interarrival " << m_devices_interarrivaltime[ed_ID][i]);
	 }

	//Compute the non-biased variance of the IAT

	int zeros = count(IAT.begin(), IAT.end(), 0);
	double mean = accumulate(IAT.begin(), IAT.end(), 0)/(IAT.size()-zeros);
	double var_iat = 0;

	for(uint32_t j = 0; j < IAT.size(); j++)
	 {
	   var_iat += pow(IAT[j] - mean,2);
	 }

	var_iat = var_iat/(IAT.size()-1);

	//we compute the variance of the EWMA

	double var_ewma = var_iat*(m_lambda/(2-m_lambda));

	//Compute the UCL and LCL

	m_ucl[ed_ID] = m_target+3*sqrt(var_ewma);
	m_lcl[ed_ID] = m_target-3*sqrt(var_ewma);

	NS_LOG_FUNCTION ("m_ucl" << m_ucl[ed_ID]);
	NS_LOG_FUNCTION ("m_ucl" << m_lcl[ed_ID]);

	//push back the value

	m_devices_ucl[ed_ID].push_back(m_ucl[ed_ID]);
	m_devices_lcl[ed_ID].push_back(m_lcl[ed_ID]);

}

bool
SimpleNetworkServer::AlreadyReceived (vector<uint32_t> vec_pkt_ID, uint32_t pkt_ID)
{
	std::vector<uint32_t>::iterator it = std::find(vec_pkt_ID.begin(), vec_pkt_ID.end(), pkt_ID);
	if (it != vec_pkt_ID.end())
		return true;
	else
		return false;
}

}
