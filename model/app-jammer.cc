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
#include <vector>
#include <numeric>

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

  m_simtime (Seconds(0)),
  m_interval (Seconds (0)),
  m_initialDelay (Seconds (0)),
  m_pktSize (0),
  m_jammer_type (0),
  m_dutycycle (0.01),
  m_ransf(false),
  m_exp(false),
  m_sf(7),
  send_times(0),
  cumultime(0),
  m_lambda(0),
  m_frequency(868.1),
  m_ranch(0)
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
AppJammer::SetSimTime (Time simtime)
{
//  NS_LOG_FUNCTION (this << interval);
	m_simtime = simtime;
}

Time
AppJammer::GetSimTime (void) const
{
 // NS_LOG_FUNCTION (this);
  return m_simtime;
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
  //NS_LOG_DEBUG ("IAT Exp " << m_exp);

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
	//NS_LOG_DEBUG ("Random SF " << m_ransf);
}

void
AppJammer::SetRanChannel (bool RanCh)
{

  m_ranch = RanCh;
  //NS_LOG_DEBUG ("Random Channel " << m_ranch);

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
AppJammer::SetLambda (double lambda)
{

  m_lambda = lambda;
  //NS_LOG_DEBUG ("Lambda " << m_lambda);

}

void
AppJammer::SetFrequency (double Freq)
{
  m_frequency = Freq;
  //NS_LOG_DEBUG ("Lambda " << m_lambda);
}

double
AppJammer::GetLambda (void)
{
 // NS_LOG_FUNCTION (this);
  return m_lambda;
}

void
AppJammer::SendPacket (void)
{
// NS_LOG_FUNCTION (this);
// Create and send a new packet

  double ppm = 30;
  double lambda;
  double mean;
  bool Exp = GetExp();
  bool RanSF = GetRanSF();
  double dutycycle = GetDC ();
  double jamtime;
  double simtime = GetSimTime ().GetSeconds();
  vector<double> Frequencies (3,0);
  Frequencies[0] = 868.1;
  Frequencies[1] = 868.3;
  Frequencies[2] = 868.5;

  Ptr<Packet> packet;

  packet = Create<Packet>(m_pktSize);

  //NS_LOG_DEBUG ("error " << error << " seconds");

  Time timeonair;
  LoraTxParameters params;

  NS_LOG_DEBUG ("Random SF ? " << RanSF);

  cumultime = 0;
  //m_simtime.GetSeconds()
  while (cumultime < m_simtime.GetSeconds())
  {

	  if (RanSF)
	  {
		  SetSpreadingFactor (m_randomsf->GetValue (7,13));
		  NS_LOG_DEBUG ("Setting SF " << unsigned(GetSpreadingFactor ()));
	  }

	  if (m_ranch)
	  {
		  m_frequency = Frequencies [m_randomsf->GetValue (0,3)];
		  NS_LOG_DEBUG ("Setting Freq " << m_frequency);

	  }

	  params.sf = GetSpreadingFactor ();

  // Compute the time on air as a function of the DC and SF

	  timeonair = m_phy->GetOnAirTime (packet,params);
	  lambda = dutycycle/timeonair.GetSeconds();

	  mean = timeonair.GetSeconds()/dutycycle;


	  jamtime = m_exprandomdelay->GetValue (mean,mean*100);

	  send_times.push_back (jamtime);
	  //NS_LOG_DEBUG ("No Exponential - Sent a packet at " << cumultime + jamtime);
	  //NS_LOG_DEBUG ("Packet scheduled at " << cumultime);
	  cumultime = cumultime + jamtime;
	  m_sendEvent = Simulator::Schedule (Seconds(cumultime), &AppJammer::SendPacketMac, this , params.sf, m_frequency);
  	}

	double sum_of_elems = std::accumulate(send_times.begin(), send_times.end(), 0);

	//NS_LOG_DEBUG ("Mean - jam " << mean);
	//NS_LOG_DEBUG ("duty-cycle " << GetDC ());
	//NS_LOG_DEBUG ("Mean - jam " << simtime/send_times.size());
	//NS_LOG_DEBUG ("Jammer sent " << send_times.size());
}


void
AppJammer::SendPacketMac (uint8_t sf, double freq)
{
  //NS_LOG_FUNCTION (this);

	uint16_t size = 0;
	Ptr<Packet> packet;

	size = m_pktSize;
	packet = Create<Packet>(size);

	Time timeonair;
	LoraTxParameters params;
	params.sf = sf;

	LoraTag tag;
	packet->RemovePacketTag (tag);
	tag.SetSpreadingFactor (params.sf);
	tag.SetFrequency(freq);
	tag.SetJammer (uint8_t (1));
	packet->AddPacketTag (tag);

	NS_LOG_DEBUG ("JAM: Sent a packet (MAC LEVEL) " << " with Freq " << freq << " and SF " << unsigned (params.sf));
	m_mac->Send (packet);

  //NS_LOG_DEBUG ("Sent counter " << sent);
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
	//Simulator::Cancel (m_sendEvent);

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
