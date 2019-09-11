/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * LoRaWAN Jamming - Copyright (c) 2019 INSA de Rennes
 * LoRaWAN ns-3 module v 0.1.0 - Copyright (c) 2017 University of Padova
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
 * LoRaWAN ns-3 module v 0.1.0 author: Davide Magrin <magrinda@dei.unipd.it>
 * LoRaWAN Jamming author: Ivan Martinez <ivamarti@insa-rennes.fr>
 */


#include "ns3/periodic-sender.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/lora-net-device.h"
#include "ns3/end-device-lora-mac.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PeriodicSender");
NS_OBJECT_ENSURE_REGISTERED (PeriodicSender);

TypeId
PeriodicSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PeriodicSender")
    .SetParent<Application> ()
    .AddConstructor<PeriodicSender> ()
    .SetGroupName ("lorawan")
    .AddAttribute ("Interval", "The interval between packet sends of this app",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&PeriodicSender::GetInterval,
                                     &PeriodicSender::SetInterval),
                   MakeTimeChecker ())
    .AddAttribute ("PacketSize", "The size of the packets this application sends, in bytes",
    			   UintegerValue (20),
				   MakeUintegerAccessor (&PeriodicSender::m_pktSize),
				   MakeUintegerChecker <uint16_t>());
  return tid;
}

PeriodicSender::PeriodicSender () :
  m_interval (Seconds (0)),
  m_initialDelay (Seconds (0)),
  m_pktSize (0),
  m_pktID (0),
  m_ransf(false),
  m_exp(false),
  m_sf(7)
{
//  NS_LOG_FUNCTION_NOARGS ();
  m_randomdelay = CreateObject<UniformRandomVariable> ();
  m_exprandomdelay = CreateObject<ExponentialRandomVariable> ();
}

PeriodicSender::~PeriodicSender ()
{
//  NS_LOG_FUNCTION_NOARGS ();
}

void
PeriodicSender::IncreasePacketID (void)
{
	m_pktID ++;
}

void
PeriodicSender::DecreasePacketID (void)
{
	m_pktID --;
}

uint32_t
PeriodicSender::GetPacketID (void)
{
	return m_pktID;
}

void
PeriodicSender::SetInterval (Time interval)
{
//  NS_LOG_FUNCTION (this << interval);
  m_interval = interval;
}

Time
PeriodicSender::GetInterval (void) const
{
 // NS_LOG_FUNCTION (this);
  return m_interval;
}

void
PeriodicSender::SetInitialDelay (Time delay)
{
//  NS_LOG_FUNCTION (this << delay);
  m_initialDelay = delay;
}

void
PeriodicSender::SetPktSize (uint16_t size)
{

  m_pktSize = size;
  NS_LOG_DEBUG ("Packet of size " << size);

}

void
PeriodicSender::SetSpreadingFactor (uint8_t sf)
{
  m_sf = sf;
}

uint8_t
PeriodicSender::GetSpreadingFactor (void)
{

  m_sf = m_mac->GetObject<EndDeviceLoraMac> ()->GetSpreadingFactor();
  return m_sf;
}

void
PeriodicSender::SetExp (bool Exp)
{

  m_exp = Exp;
  NS_LOG_DEBUG ("IAT Exp " << m_exp);

}

bool PeriodicSender::GetExp (void)
{

  return m_exp;
}

void
PeriodicSender::SendPacket (void)
{
  // NS_LOG_FUNCTION (this);
  // Create and send a new packet

  NS_LOG_DEBUG ("Starting up application with a Exp =  " << GetExp() << " and SF = " << unsigned(GetSpreadingFactor()));

  uint16_t size = 0;
  uint32_t ID = 0;
  Ptr<Packet> packet;

  double ppm = 30;
  double error = ppm/1e6;

  double lambda;
  double mean;
  bool Exp = GetExp();
  double dutycycle = 0.01;
  double interval = GetInterval().GetSeconds();

  size = m_pktSize;
  packet = Create<Packet>(size);

  IncreasePacketID();
  ID = GetPacketID();


  Time timeonair;
  LoraTxParameters params;

  params.sf = GetSpreadingFactor ();

  NS_LOG_DEBUG ("SpreadingFactor " << unsigned(params.sf));

  // Tag the packet with its Spreading Factor and the Application Packet ID
  LoraTag tag;
  packet->RemovePacketTag (tag);
  tag.SetPktID (ID);
  tag.SetSpreadingFactor (params.sf);
  packet->AddPacketTag (tag);

  NS_LOG_DEBUG ("SF at Application level " << unsigned(params.sf));
  NS_LOG_DEBUG ("Packet ID app" << unsigned(ID));
  NS_LOG_INFO ("Packet size app " << packet->GetSize());


  bool sent;
  sent = m_mac->Send (packet);

  if (not sent)
  {
	  DecreasePacketID();
  }

  else {

  }
  // Compute the time on air as a function of the DC and SF

  timeonair = m_phy->GetOnAirTime (packet,params);
  NS_LOG_DEBUG ("time on air " << timeonair.GetSeconds());
  NS_LOG_DEBUG ("preamble bits " << params.nPreamble);


  // Compute the interval as a function of the duty-cycle

  if (GetInterval().GetSeconds() <= timeonair.GetSeconds()/dutycycle)
  {
	  //interval = 100*(1-dutycycle)* timeonair.GetSeconds();
	  interval = timeonair.GetSeconds()/dutycycle;
	  SetInterval(Seconds(interval));

	  NS_LOG_DEBUG ("DC " << dutycycle);
	  NS_LOG_DEBUG ("interval " << interval);
  }

  NS_LOG_DEBUG ("Interval set as " << GetInterval().GetSeconds());

  if (Exp)
  {
	  mean = interval;
	  interval = m_exprandomdelay->GetValue (mean,mean*100);
	  m_sendEvent = Simulator::Schedule (Seconds(interval), &PeriodicSender::SendPacket, this);
	  NS_LOG_DEBUG ("Exponential - Sent a packet at  " << Simulator::Now ().GetSeconds ());
	  NS_LOG_DEBUG ("Exponential - mean  " << mean);
	  NS_LOG_DEBUG ("Exponential - interval  " << interval);
  }
  	  else
  	  {
  		m_sendEvent = Simulator::Schedule (Seconds(interval), &PeriodicSender::SendPacket, this);
  		NS_LOG_DEBUG ("No Exponential - Sent a packet at " << Simulator::Now ().GetSeconds ());
	  }



  //double interval = m_interval.GetSeconds() + m_randomdelay->GetValue (-m_interval.GetSeconds()*error, m_interval.GetSeconds()*error);
  //NS_LOG_DEBUG ("Next event at " << interval);

  // Schedule the next SendPacket event
  //m_sendEvent = Simulator::Schedule (Seconds (interval), &PeriodicSender::SendPacket, this);
  //m_sendEvent = Simulator::Schedule (m_interval, &PeriodicSender::SendPacket, this);

}

void
PeriodicSender::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  // Make sure we have a MAC layer
  if (m_mac == 0)
    {
      // Assumes there's only one device
      Ptr<LoraNetDevice> loraNetDevice = m_node->GetDevice (0)->GetObject<LoraNetDevice> ();

      m_mac = loraNetDevice->GetMac ();
      NS_ASSERT (m_mac != 0);
    }

  // Schedule the next SendPacket event
  Simulator::Cancel (m_sendEvent);
  NS_LOG_DEBUG ("Starting up application with a first event with a " <<
                m_initialDelay.GetSeconds () << " seconds delay");

  m_sendEvent = Simulator::Schedule (m_initialDelay, &PeriodicSender::SendPacket, this);

  NS_LOG_DEBUG ("Event Id: " << m_sendEvent.GetUid ());
}

void
PeriodicSender::StopApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Simulator::Cancel (m_sendEvent);
}
}

