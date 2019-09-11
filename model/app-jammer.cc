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

#include "ns3/app-jammer.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/lora-net-device.h"
#include "ns3/lora-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AppJammer");
NS_OBJECT_ENSURE_REGISTERED (AppJammer);

TypeId
AppJammer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AppJammer")
    .SetParent<Application> ()
    .AddConstructor<AppJammer> ()
    .SetGroupName ("lorawan")
    .AddAttribute ("Interval", "The interval between packet sends of this app",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&AppJammer::GetInterval,
                                     &AppJammer::SetInterval),
                   MakeTimeChecker ())
    .AddAttribute ("PacketSize", "The size of the packets this application sends, in bytes",
    			   UintegerValue (20),
				   MakeUintegerAccessor (&AppJammer::m_pktSize),
				   MakeUintegerChecker <uint16_t>());
  return tid;
}

AppJammer::AppJammer () :
  m_interval (Seconds (0)),
  m_initialDelay (Seconds (0)),
  m_pktSize (0),
  m_jammer_type (0),
  m_dutycycle (0.01),
  m_ransf(false),
  m_exp(false),
  m_sf(7)
{
//  NS_LOG_FUNCTION_NOARGS ();
  m_exprandomdelay = CreateObject<ExponentialRandomVariable> ();
  m_randomsf = CreateObject<UniformRandomVariable> ();

}

AppJammer::~AppJammer ()
{
//  NS_LOG_FUNCTION_NOARGS ();
}

void
AppJammer::SetInterval (Time interval)
{
//  NS_LOG_FUNCTION (this << interval);
  m_interval = interval;
}

Time
AppJammer::GetInterval (void) const
{
 // NS_LOG_FUNCTION (this);
  return m_interval;
}

void
AppJammer::SetInitialDelay (Time delay)
{
//  NS_LOG_FUNCTION (this << delay);
  m_initialDelay = delay;
}

void
AppJammer::SetPktSize (uint16_t size)
{

  m_pktSize = size;
  NS_LOG_DEBUG ("Packet of size " << size);

}

void
AppJammer::SetDC (double dutycycle)
{

  m_dutycycle = dutycycle;
  NS_LOG_DEBUG ("Duty-Cycle " << m_dutycycle);

}

double
AppJammer::GetDC (void)
{
 // NS_LOG_FUNCTION (this);
  return m_dutycycle;
}

void
AppJammer::SetExp (bool Exp)
{

  m_exp = Exp;
  NS_LOG_DEBUG ("IAT Exp " << m_exp);

}

bool
AppJammer::GetExp (void)
{
 // NS_LOG_FUNCTION (this);
  return m_exp;
}

void
AppJammer::SetRanSF (bool ransf)
{
 // NS_LOG_FUNCTION (this);
	m_ransf = ransf;
	NS_LOG_DEBUG ("Random SF " << m_ransf);
}

bool
AppJammer::GetRanSF (void)
{
 // NS_LOG_FUNCTION (this);
  return m_ransf;
}

void
AppJammer::SetSpreadingFactor (uint8_t sf)
{
  m_sf = sf;
}

uint8_t
AppJammer::GetSpreadingFactor ()
{
  return m_sf;
}

void
AppJammer::SendPacket (void)
{
// NS_LOG_FUNCTION (this);
// Create and send a new packet

  uint16_t size = 0;
  Ptr<Packet> packet;

  double ppm = 30;
  double error = ppm/1e6;

  double lambda;
  double mean;
  bool Exp = GetExp();
  bool RanSF = GetRanSF();
  double dutycycle = GetDC ();
  double jamtime;


  NS_LOG_DEBUG ("error " << error << " seconds");

  size = m_pktSize;
  packet = Create<Packet>(size);

  Time timeonair;
  LoraTxParameters params;


  if (RanSF==true)
  {
	  SetSpreadingFactor (m_randomsf->GetValue (7,12));
  }

  params.sf = GetSpreadingFactor ();

  // Tag the packet with information about its Spreading Factor, the preamble and the sender ID

  LoraTag tag;
  packet->RemovePacketTag (tag);
  tag.SetSpreadingFactor (params.sf);
  tag.SetJammer (uint8_t (1));
  packet->AddPacketTag (tag);


  NS_LOG_DEBUG ("SF " << unsigned(params.sf));
  NS_LOG_DEBUG ("Packet ID app" << unsigned(packet->GetUid()));
  NS_LOG_INFO ("Packet size app " << packet->GetSize());
  NS_LOG_DEBUG ("preamble bits " << params.nPreamble);


  m_mac->Send (packet);

  // Compute the time on air as a function of the DC and SF

  timeonair = m_phy->GetOnAirTime (packet,params);
  NS_LOG_DEBUG ("time on air " << timeonair.GetSeconds());
  NS_LOG_DEBUG ("DC " << GetDC ());


  if (Exp)
  {
	  lambda = dutycycle/timeonair.GetSeconds();
	  mean = 1/lambda;
	  //mean=100*(1-dutycycle)* timeonair.GetSeconds();
	  jamtime = m_exprandomdelay->GetValue (mean,mean*100);
	  m_sendEvent = Simulator::Schedule (Seconds(jamtime), &AppJammer::SendPacket, this);
	  NS_LOG_DEBUG ("Exponential - Sent a packet at  " << Simulator::Now ().GetSeconds ());
	  NS_LOG_DEBUG ("Exponential - mean  " << mean);
	  NS_LOG_DEBUG ("Exponential - jamtime  " << jamtime);
  }
  	  else
  	  {
  		jamtime = timeonair.GetSeconds()*(1/dutycycle-1);
  		m_sendEvent = Simulator::Schedule (Seconds(jamtime), &AppJammer::SendPacket, this);
  		NS_LOG_DEBUG ("No Exponential - Sent a packet at " << Simulator::Now ().GetSeconds ());
	  }


  //Simulator::Schedule (duration+Seconds(jamtime)+NanoSeconds(15), &JammerLoraPhy::Send, this, packet, txParams, frequencyMHz, txPowerDbm);

  //double interval = m_interval.GetSeconds() + m_exprandomdelay->GetValue (-m_interval.GetSeconds()*error, m_interval.GetSeconds()*error);
  //NS_LOG_DEBUG ("Next event at " << interval);

  //m_sendEvent = Simulator::Schedule (m_interval, &AppJammer::SendPacket, this);
  //NS_LOG_DEBUG ("Sent a packet of size " << packet->GetSize ());
}

void
AppJammer::StartApplication (void)
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

	m_sendEvent = Simulator::Schedule (m_initialDelay, &AppJammer::SendPacket, this);
	NS_LOG_DEBUG ("Event Id: " << m_sendEvent.GetUid ());

}

void
AppJammer::StopApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Simulator::Cancel (m_sendEvent);

  Ptr<LoraNetDevice> loraNetDevice = m_node->GetDevice (0)->GetObject<LoraNetDevice> ();
  m_mac = loraNetDevice->GetMac ();
  Ptr<JammerLoraMac> m_mac = loraNetDevice->GetMac ()->GetObject<JammerLoraMac> ();
  m_mac->SetAppFinish();
}
}
