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

#include "lora-energy-consumption-helper.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "string.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/object.h"
#include <limits>

#include "ns3/lora-phy.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoraEnergyConsumptionHelper");

/***************************************
 *    LoraEnergyConsumptionHelper::Event    *
 ***************************************/

// Event Constructor
LoraEnergyConsumptionHelper::Event::Event (int status, Time duration, double txPowerdBm,
                                      uint8_t spreadingFactor, double frequencyMHz, uint32_t node_id):
  m_status (status),
  m_startTime (Simulator::Now ()),
  m_endTime (m_startTime + duration),
  m_sf (spreadingFactor),
  m_txPowerdBm (txPowerdBm),
  m_frequencyMHz (frequencyMHz),
  m_node_id (node_id)

{
  // NS_LOG_FUNCTION_NOARGS ();
}

// Event Destructor
LoraEnergyConsumptionHelper::Event::~Event ()
{
  // NS_LOG_FUNCTION_NOARGS ();
}

// Getters

int
LoraEnergyConsumptionHelper::Event::GetStatus (void) const
{
  return m_status;

}

Time
LoraEnergyConsumptionHelper::Event::GetStartTime (void) const
{
  return m_startTime;
}

Time
LoraEnergyConsumptionHelper::Event::GetEndTime (void) const
{
  return m_endTime;
}

Time
LoraEnergyConsumptionHelper::Event::GetDuration (void) const
{
  return m_endTime - m_startTime;
}

double
LoraEnergyConsumptionHelper::Event::GetTxPowerdBm (void) const
{
  return m_txPowerdBm;
}

uint8_t
LoraEnergyConsumptionHelper::Event::GetSpreadingFactor (void) const
{
  return m_sf;
}

Ptr<Packet>
LoraEnergyConsumptionHelper::Event::GetPacket (void) const
{
  return m_packet;
}

double
LoraEnergyConsumptionHelper::Event::GetFrequency (void) const
{
  return m_frequencyMHz;
}

double
LoraEnergyConsumptionHelper::Event::GetNodeId (void) const
{
  return m_node_id;
}

void
LoraEnergyConsumptionHelper::Event::Print (std::ostream &stream) const
{
  stream << "(" << m_startTime.GetSeconds () << " s - " <<
  m_endTime.GetSeconds () << " s), SF" << unsigned(m_sf) << ", " <<
  m_txPowerdBm << " dBm, "<< m_frequencyMHz << " MHz";
}

std::ostream &operator << (std::ostream &os, const LoraEnergyConsumptionHelper::Event &event)
{
  event.Print (os);

  return os;
}

/****************************
 *  LoraEnergyConsumptionHelper  *
 ****************************/

NS_OBJECT_ENSURE_REGISTERED (LoraEnergyConsumptionHelper);

TypeId
LoraEnergyConsumptionHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoraEnergyConsumptionHelper")
    .SetParent<Object> ()
    .SetGroupName ("lorawan");
  return tid;
}

LoraEnergyConsumptionHelper::LoraEnergyConsumptionHelper ()
{
  NS_LOG_FUNCTION (this);
}

LoraEnergyConsumptionHelper::~LoraEnergyConsumptionHelper ()
{
  NS_LOG_FUNCTION (this);
}


// LoRa transmission consumption vector in mA for each Spreading Factor
const double LoraEnergyConsumptionHelper::consotx[6] = {83 , 83, 83, 83, 83, 83};
// 														SF7  SF8  SF9  SF10 SF11 SF12
// LoRa reception consumption vector in mA for each Spreading Factor

const double LoraEnergyConsumptionHelper::consorx[6] = {32, 32, 32, 32, 32, 32};
// 														SF7  SF8  SF9  SF10 SF11 SF12
// LoRa consumption vector in mA for standby
const double LoraEnergyConsumptionHelper::consostb = 32;

// LoRa consumption vector in mA for Sleep
const double LoraEnergyConsumptionHelper::consosleep = 0.0045;


Ptr<LoraEnergyConsumptionHelper::Event>
LoraEnergyConsumptionHelper::Add (int status, Time duration, double txPower,
                             uint8_t spreadingFactor,
                             double frequencyMHz, uint32_t node_id)
{

  NS_LOG_FUNCTION (this << duration.GetSeconds () << txPower << unsigned
                   (spreadingFactor) << frequencyMHz);

  // Create an event based on the parameters
  Ptr<LoraEnergyConsumptionHelper::Event> event =
    Create<LoraEnergyConsumptionHelper::Event> (status, duration, txPower, spreadingFactor,
                                           frequencyMHz , node_id);


   return event;
}

double
LoraEnergyConsumptionHelper::TxConso (Ptr<LoraEnergyConsumptionHelper::Event> event)
{
  NS_LOG_FUNCTION (this << event);

  // Gather information about the event

  //double rxPowerDbm = event->GetTxPowerdBm ();
  uint8_t sf = event->GetSpreadingFactor ();

  // Handy information about the time frame when the packet was received
  Time duration = event->GetDuration ();


  double event_conso = 0;

  switch(sf) {
      case 7 : event_conso = consotx [0] * duration.GetHours ();
      	  	   break;
      case 8 : event_conso = consotx [1] * duration.GetHours ();
               break;
      case 9 : event_conso = consotx [2] * duration.GetHours ();
               break;
      case 10 : event_conso = consotx [3] * duration.GetHours ();
               break;
      case 11 : event_conso = consotx [4] * duration.GetHours ();
               break;
      case 12 : event_conso = consotx [5] * duration.GetHours ();
               break;
      default : event_conso = consotx [5] * duration.GetHours ();
               break;
   }

  NS_LOG_FUNCTION (this << duration.GetSeconds());
  NS_LOG_FUNCTION (this << event_conso);

  return event_conso;
}


double
LoraEnergyConsumptionHelper::RxConso (Ptr<LoraEnergyConsumptionHelper::Event> event)
{
  NS_LOG_FUNCTION (this << event);


  // Gather information about the event

  //double rxPowerDbm = event->GetTxPowerdBm ();
  uint8_t sf = event->GetSpreadingFactor ();

  // Handy information about the time frame when the packet was received
  Time duration = event->GetDuration ();


  double event_conso = 0;


  switch(sf) {
      case 7 : event_conso = consorx [0] * duration.GetHours ();
      	  	   break;
      case 8 : event_conso = consorx [1] * duration.GetHours ();
               break;
      case 9 : event_conso = consorx [2] * duration.GetHours ();
               break;
      case 10 : event_conso = consorx [3] * duration.GetHours ();
               break;
      case 11 : event_conso = consorx [4] * duration.GetHours ();
               break;
      case 12 : event_conso = consorx [5] * duration.GetHours ();
               break;
      default : event_conso = consorx [5] * duration.GetHours ();
               break;
   }

  NS_LOG_FUNCTION (this << duration.GetSeconds());
  NS_LOG_FUNCTION (this << event_conso);

  return event_conso;

}

double
LoraEnergyConsumptionHelper::StbConso (Ptr<LoraEnergyConsumptionHelper::Event> event)
{
  NS_LOG_FUNCTION (this << event);

  // Handy information about the time frame when the packet was received
  Time duration = event->GetDuration ();

  double event_conso = consostb * duration.GetHours ();

  return event_conso;

}

double
LoraEnergyConsumptionHelper::SleepConso (Ptr<LoraEnergyConsumptionHelper::Event> event)
{
  NS_LOG_FUNCTION (this << event);

  // Handy information about the time frame when the packet was received
  Time duration = event->GetDuration ();

  double event_conso = consosleep * duration.GetHours ();
  return event_conso;

}

}
