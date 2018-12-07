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
  m_jammer_type (0)
{
//  NS_LOG_FUNCTION_NOARGS ();
  m_randomdelay = CreateObject<UniformRandomVariable> ();

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
AppJammer::SendPacket (void)
{
// NS_LOG_FUNCTION (this);
// Create and send a new packet

  uint16_t size = 0;
  Ptr<Packet> packet;

  double ppm = 30;
  double error = ppm/1e6;

  NS_LOG_DEBUG ("error " << error << " seconds");

  size = m_pktSize;
  packet = Create<Packet>(size);

  m_mac->Send (packet);

  double interval = m_interval.GetSeconds() + m_randomdelay->GetValue (-m_interval.GetSeconds()*error, m_interval.GetSeconds()*error);
  NS_LOG_DEBUG ("Next event at " << interval);

  m_sendEvent = Simulator::Schedule (m_interval, &AppJammer::SendPacket, this);
  NS_LOG_DEBUG ("Sent a packet of size " << packet->GetSize ());
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
}
}
