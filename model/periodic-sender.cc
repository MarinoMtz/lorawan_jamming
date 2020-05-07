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

#include <numeric>

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
				   MakeUintegerChecker <uint16_t>())
	.AddTraceSource ("SendPacket",
					 "Trace source indicating a packet  "
					 "transmission",
					  MakeTraceSourceAccessor
					  (&PeriodicSender::m_sendpacket));
  return tid;
}

PeriodicSender::PeriodicSender () :
  m_interval (Seconds (0)),
  m_initialDelay (Seconds (0)),
  m_pktSize (0),
  m_pktID (0),
  m_dutycycle (0.01),
  m_simtime (Seconds(0)),
  cumultime(0),
  m_ransf(false),
  m_exp(false),
  m_retransmissions(false),
  m_sf(7),
  m_mean(0),
  m_rxnumber(1),
  m_percentage_rtx(100)
{
//  NS_LOG_FUNCTION_NOARGS ();
  m_randomdelay = CreateObject<UniformRandomVariable> ();
  m_exprandomdelay = CreateObject<ExponentialRandomVariable> ();
  m_prioritypkt = CreateObject<UniformRandomVariable> ();
}

PeriodicSender::~PeriodicSender ()
{
//  NS_LOG_FUNCTION_NOARGS ();
}

void
PeriodicSender::SetSimTime (Time simtime)
{
//  NS_LOG_FUNCTION (this << interval);
	m_simtime = simtime;
}

Time
PeriodicSender::GetSimTime (void) const
{
 // NS_LOG_FUNCTION (this);
  return m_simtime;
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
  //NS_LOG_DEBUG ("Packet of size " << size);

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
  //NS_LOG_DEBUG ("IAT Exp " << m_exp);

}

void
PeriodicSender::SetRetransmissions (bool retrans, int rxnumber, double percentage_rtx)
{
	m_retransmissions = retrans;
	m_rxnumber = rxnumber;
	m_percentage_rtx = percentage_rtx;
}

bool
PeriodicSender::GetRetransmissions (void)
{

  return m_retransmissions;
}

bool
PeriodicSender::GetExp (void)
{

  return m_exp;
}

void
PeriodicSender::UnconfirmedTraffic (void)
{

double cumul_time = 0;
double sendtime = 0;

double simtime = GetSimTime().GetSeconds();

// Send the packets without considering ACK nor re-transmissions

while (cumul_time < simtime){

	sendtime = SentTime();
	cumul_time = cumul_time + sendtime;
	m_sendEvent = Simulator::Schedule (Seconds(cumul_time), &PeriodicSender::SendPacketMacUnconfirmed, this);
	//NS_LOG_DEBUG ("Unconfirmed Traffic - send-time " << sendtime << " " << cumul_time );
	}

double sum_of_elems = std::accumulate(send_times.begin(), send_times.end(), 0);
//NS_LOG_DEBUG ("Mean - ed " << sum_of_elems/send_times.size());

}

void
PeriodicSender::SendPacketMacUnconfirmed ()
{
    //NS_LOG_FUNCTION (this);


	uint32_t ID = 0;
	Ptr<Packet> packet;

	packet = Create<Packet>(m_pktSize);

	IncreasePacketID();
	ID = GetPacketID();

	Time timeonair;
	LoraTxParameters params;
	params.sf = GetSpreadingFactor ();

	LoraTag tag;
	packet->PeekPacketTag (tag);
	tag.SetSpreadingFactor (params.sf);
	tag.SetPktID (ID);
	tag.Setntx(1);
	packet->AddPacketTag (tag);

	//NS_LOG_DEBUG ("Sent a packet (MAC LEVEL) at " << Simulator::Now ().GetSeconds ());

	bool sent;
	sent = m_mac->Send (packet);

	Time simtime = GetSimTime ();

	if (Simulator::Now () < simtime){

	if (not sent)
	  {
		  if (sendtries[ID-1] <= 5){
		  //sendtries[ID-1] ++;
		  //double sendtime = m_exprandomdelay->GetValue (m_mean,m_mean*100);
		  //m_sendEvent = Simulator::Schedule (Seconds(sendtime), &PeriodicSender::SendPacketMacUnconfirmed, this);
		  NS_LOG_DEBUG ("Not send " << Simulator::Now ().GetSeconds ());
		  DecreasePacketID();
		  }
		  else {
			DecreasePacketID();
		  }
	  }
	}

  //NS_LOG_DEBUG ("Sending packet at " << Simulator::Now ().GetSeconds() << " Packet ID " << ID);
  //NS_LOG_DEBUG ("Sent counter " << sent);

}

void
PeriodicSender::ConfirmedTraffic (uint32_t ntx, uint32_t ID, bool confirmed, uint8_t retx)
{
	Time sendtime = Seconds(0);
	sendtime = Seconds(SentTime());
	Time simtime = GetSimTime ();

	//NS_LOG_DEBUG ("noack " << unsigned(retx));

	if (Simulator::Now () < simtime){

		if (ntx == 0 && ID == 0 && confirmed == false) // Sending first packet
		{
			IncreasePacketID();
			m_sendEvent = Simulator::Schedule (sendtime, &PeriodicSender::SendPacketMacConfirmed, this, 1, 1, 0);
			return;
		}
		if (confirmed || ntx >= m_rxnumber || retx){

			IncreasePacketID();
			ID = GetPacketID();
			ntx = 1;
			m_sendEvent = Simulator::Schedule (sendtime, &PeriodicSender::SendPacketMacConfirmed, this, ID, ntx, retx);
		}
		else {
			m_sendEvent = Simulator::Schedule (sendtime, &PeriodicSender::SendPacketMacConfirmed, this, ID, ntx + 1, retx);
		}
	}
}

void
PeriodicSender::SendPacketMacConfirmed (uint32_t ID, uint32_t ntx, uint8_t retx)
{
    //NS_LOG_FUNCTION (this);

	// Decide whether or not the packet will be ackited or not

	if (ntx == 1){ // First time we sent this packet, we decide if it'll be ackited
		double ack = m_prioritypkt->GetValue (0,100);
		if (ack <= m_percentage_rtx) {retx = 0;}
		else {retx = 1;}

		//NS_LOG_DEBUG ("To be ACK ? " << unsigned(retx) << " "<< ack);
	}

	//NS_LOG_DEBUG ("re-tx app ? " << unsigned(retx));
	//NS_LOG_DEBUG ("ntx app ? " << unsigned(ntx));
	//NS_LOG_DEBUG ("m_rxnumber ? " << unsigned(m_rxnumber));

	Ptr<Packet> packet;

	packet = Create<Packet>(m_pktSize);

	Time timeonair;
	LoraTxParameters params;
	params.sf = GetSpreadingFactor ();

	LoraTag tag;
	//packet->RemovePacketTag (tag);
	tag.SetSpreadingFactor (params.sf);
	tag.SetPktID (ID);
	tag.Setntx(ntx);
	tag.SetRetx(retx);
	packet->AddPacketTag (tag);

	NS_LOG_DEBUG ("Sent a packet (MAC LEVEL) at " << Simulator::Now ().GetSeconds () << " with SF = " << unsigned (params.sf));
	//NS_LOG_DEBUG ("APP: Packet ID " << ID << " Priority? " << unsigned (retx));

	bool sent;
	sent = m_mac->Send (packet);

	Time simtime = GetSimTime ();
	//Time sendtime = Seconds(SentTime());

	if (Simulator::Now () < simtime){

		if (not sent)
			{
			DecreasePacketID();
			NS_LOG_DEBUG ("Not send " << Simulator::Now ().GetSeconds ());
			 //m_sendEvent = Simulator::Schedule (Simulator::Now (), &PeriodicSender::SendPacketMacConfirmed, this, ID, ntx);
			}
	}

	//NS_LOG_DEBUG ("Sending packet at " << Simulator::Now ().GetSeconds() << " Packet ID " << ID);

}

double
PeriodicSender::GetNextTxTime (void)
{
	// NS_LOG_FUNCTION (this);
	// Create and send a new packet
    // NS_LOG_DEBUG ("Starting up application with a Exp =  " << GetExp() << " and SF = " << unsigned(GetSpreadingFactor()));

	uint16_t size = 0;
	Ptr<Packet> packet;

	double ppm = 30;
	//double error = ppm/1e6;
	double lambda;

	bool Exp = GetExp();
	double dutycycle = 0.01;
	double interval = GetInterval().GetSeconds();
	double simtime = GetSimTime ().GetSeconds();
	double sendtime;

	packet = Create<Packet>(m_pktSize+1);

	NS_LOG_DEBUG ("m_pktSize " << m_pktSize);

	//m_pktSize-8
	Time timeonair;
	LoraTxParameters params;

	params.sf = GetSpreadingFactor ();

	// Tag the packet with its Spreading Factor and the Application Packet ID
	//LoraTag tag;
	//packet->RemovePacketTag (tag);
	//tag.SetSpreadingFactor (params.sf);
	//packet->AddPacketTag (tag);

	// Compute the time on air as a function of the DC and SF
	timeonair = m_phy->GetOnAirTime (packet,params);
	//NS_LOG_DEBUG ("preamble bits " << params.nPreamble);

	// Compute the interval as a function of the duty-cycle
	m_mean = timeonair.GetSeconds()/dutycycle;
	//m_mean = ToA/dutycycle;

	//NS_LOG_DEBUG ("Lambda - ED " << lambda);
	//NS_LOG_DEBUG ("simtime " << simtime);

	//if (Exp) {
	sendtime = m_exprandomdelay->GetValue (m_mean,m_mean*100);

	//}
    //else {sendtime = timeonair.GetSeconds()*(1/dutycycle-1);}

	NS_LOG_DEBUG ("SF " << unsigned(params.sf) << " ToA " << timeonair.GetSeconds());

    return sendtime;
}

void
PeriodicSender::StartApplication (void)
{
  //NS_LOG_FUNCTION (this);

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
  //NS_LOG_DEBUG ("Starting up application with a first event with a " <<
  //              m_initialDelay.GetSeconds () << " seconds delay");

  bool retransmissions;

  retransmissions = GetRetransmissions();

  if (retransmissions)
  {
	  m_sendEvent = Simulator::Schedule (m_initialDelay, &PeriodicSender::ConfirmedTraffic, this, 0 , 0, false, 1);
  }

  else {
	  m_sendEvent = Simulator::Schedule (m_initialDelay, &PeriodicSender::UnconfirmedTraffic, this);
  }

  //NS_LOG_DEBUG ("Event Id: " << m_sendEvent.GetUid ());

  // Create the vector of send times

  cumultime = 0;
  double sendtime = 0;

  double simtime = GetSimTime().GetSeconds();
  double n_tries = 0;
  double n = 2;
  double tries = 0;
  ///////////////////////////////////////////////
  ///// Create the transmission times vector /////
  ///////////////////////////////////////////////

  while (cumultime < simtime*n){

  	sendtime = GetNextTxTime();
  	send_times.push_back (sendtime);
  	cumultime = cumultime + sendtime;
  	n_tries ++;
  	//m_sendEvent = Simulator::Schedule (Seconds(cumultime), &PeriodicSender::SendPacketMacUnconfirmed, this);
  	//Simulator::Schedule (m_initialDelay, &PeriodicSender::SendPacket, this);
  	//NS_LOG_DEBUG ("Times " << sendtime << " " << cumultime << " " << simtime);
  	}
  	tries = n_tries/n;

	double sum_of_elems = std::accumulate(send_times.begin(), send_times.end(), 0);
	//NS_LOG_DEBUG ("Number of tries " << tries);
	//NS_LOG_DEBUG ("m_mean " << m_mean);
	//NS_LOG_DEBUG ("real mean " << simtime/tries);
	//NS_LOG_DEBUG ("simtime " << simtime);
	//NS_LOG_DEBUG ("m_pktSize " << m_pktSize);
	//NS_LOG_DEBUG ("ToA " << m_mean*0.01);

}

double
PeriodicSender::SentTime (void)
{
	double sendtime = send_times[0];
	send_times.erase(send_times.begin());
	//NS_LOG_DEBUG ("Returned Send-time " << sendtime << " Size " << send_times.size());
	return sendtime;
}

void
PeriodicSender::StopApplication (void)
{
   //NS_LOG_FUNCTION_NOARGS ();


	NS_LOG_DEBUG ("mean " << m_mean);
	Simulator::Remove (m_sendEvent);
	Simulator::Cancel (m_sendEvent);
}
}
