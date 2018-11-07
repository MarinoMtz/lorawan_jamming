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

#include <algorithm>
#include "ns3/end-device-lora-phy.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/lora-tag.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EndDeviceLoraPhy");

NS_OBJECT_ENSURE_REGISTERED (EndDeviceLoraPhy);

TypeId
EndDeviceLoraPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EndDeviceLoraPhy")
    .SetParent<LoraPhy> ()
    .SetGroupName ("lorawan")
    .AddConstructor<EndDeviceLoraPhy> ()
    .AddTraceSource ("LostPacketBecauseWrongFrequency",
                     "Trace source indicating a packet "
                     "could not be correctly decoded because"
                     "the ED was listening on a different frequency",
                     MakeTraceSourceAccessor (&EndDeviceLoraPhy::m_wrongFrequency),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("LostPacketBecauseWrongSpreadingFactor",
                     "Trace source indicating a packet "
                     "could not be correctly decoded because"
                     "the ED was listening for a different Spreading Factor",
                     MakeTraceSourceAccessor (&EndDeviceLoraPhy::m_wrongSf),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("EndDeviceState",
                     "The current state of the device",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraPhy::m_state),
                     "ns3::TracedValueCallback::EndDeviceLoraPhy::State");
  return tid;
}

// Initialize the device with some common settings.
// These will then be changed by helpers.
EndDeviceLoraPhy::EndDeviceLoraPhy () :

  m_state (SLEEP),
  m_frequency (868.1),
  m_sf (7),
  m_last_time_stamp (Seconds(0)),
  m_last_state (4),
  m_preamble (0.012544),
  m_battery_level (battery_capacity),
  m_cumulative_tx_conso (0),
  m_cumulative_rx_conso (0),
  m_cumulative_stb_conso (0),
  m_cumulative_sleep_conso (0)
{
}

EndDeviceLoraPhy::~EndDeviceLoraPhy ()
{
}

// Downlink sensitivity (from SX1272 datasheet)
// {SF7, SF8, SF9, SF10, SF11, SF12}
// These sensitivites are for a bandwidth of 125000 Hz

const double EndDeviceLoraPhy::sensitivity[6] =
{-124, -127, -130, -133, -135, -137};

// Standard Battery Capacity in mAh
const double EndDeviceLoraPhy::battery_capacity = 2400;

void
EndDeviceLoraPhy::SetSpreadingFactor (uint8_t sf)
{
  m_sf = sf;
}

uint8_t
EndDeviceLoraPhy::GetSpreadingFactor (void)
{
  return m_sf;
}

Time
EndDeviceLoraPhy::GetLastTimeStamp (void)
{
  return m_last_time_stamp;
}

int
EndDeviceLoraPhy::GetLastState (void)
{
  return m_last_state;
}

double
EndDeviceLoraPhy::GetBatteryLevel (void)
{
  return m_battery_level;
}

double
EndDeviceLoraPhy::GetTxConso (void)
{
  return m_cumulative_tx_conso;
}

double
EndDeviceLoraPhy::GetRxConso (void)
{
  return m_cumulative_rx_conso;
}

double
EndDeviceLoraPhy::GetStbConso (void)
{
  return m_cumulative_stb_conso;
}

double
EndDeviceLoraPhy::GetSleepConso (void)
{
  return m_cumulative_sleep_conso;
}

void
EndDeviceLoraPhy::Send (Ptr<Packet> packet, LoraTxParameters txParams,
                        double frequencyMHz, double txPowerDbm)
{
  NS_LOG_FUNCTION (this << packet << txParams << frequencyMHz << txPowerDbm);

  NS_LOG_INFO ("Current state: " << m_state);

  // We must be either in STANDBY or SLEEP mode to send a packet

  if (m_state != STANDBY && m_state != SLEEP)
    {
      NS_LOG_INFO ("Cannot send because device is currently not in STANDBY or SLEEP mode");
      return;
    }

  // We can send the packet: switch to the TX state
  SwitchToTx ();

  // Compute the duration of the transmission
  Time duration = GetOnAirTime (packet, txParams);

  // Compute the duration preamble, this will be used for the interference helper
  Time preamble = GetPreambleTime (txParams.sf, txParams.bandwidthHz, txParams.nPreamble);

  // Tag the packet with information about its Spreading Factor, the preamble and the sender ID
  LoraTag tag;
  packet->RemovePacketTag (tag);
  tag.SetSpreadingFactor (txParams.sf);
  tag.SetPreamble(preamble.ToDouble(Time::S));
  tag.SetSenderID (m_device->GetNode ()->GetId ());
  packet->AddPacketTag (tag);

  // Send the packet over the channel
  NS_LOG_INFO ("Sending the packet in the channel");
  m_channel->Send (this, packet, txPowerDbm, txParams, duration, frequencyMHz);

  // Schedule the switch back to STANDBY mode.
  // For reference see SX1272 datasheet, section 4.1.6

  if (m_state != DEAD)  {

  Simulator::Schedule (duration, &EndDeviceLoraPhy::SwitchToStandby, this);

  // Schedule the txFinished callback, if it was set
  // The call is scheduled just after the switch to standby in case the upper
  // layer wishes to change the state. This ensures that it will find a PHY in
  // STANDBY mode.
  if (!m_txFinishedCallback.IsNull ())
    {
      Simulator::Schedule (duration + NanoSeconds (10),
                           &EndDeviceLoraPhy::m_txFinishedCallback, this,
                           packet);

    }

  // Create the event to calculate the energy consumption of this transmission event

  Ptr<LoraEnergyConsumptionHelper::Event> Event;
  Event = m_conso.Add (1, duration, txPowerDbm, txParams.sf, frequencyMHz, m_device->GetNode ()->GetId ());
  Consumption (Event, m_device->GetNode ()->GetId (), 1);

  NS_LOG_FUNCTION (this << duration);

  // Call the trace source
  if (m_device)
    {
      m_startSending (packet, m_device->GetNode ()->GetId ());
    }
  else
    {
      m_startSending (packet, 0);
    }
  }
}

void
EndDeviceLoraPhy::StartReceive (Ptr<Packet> packet, double rxPowerDbm,
                                uint8_t sf, Time duration, double frequencyMHz)
{

  NS_LOG_FUNCTION (this << packet << rxPowerDbm << unsigned (sf) << duration <<
                   frequencyMHz);

  // Notify the LoraInterferenceHelper of the impinging signal, and remember
  // the event it creates. This will be used then to correctly handle the end
  // of reception event.
  //
  // We need to do this regardless of our state or frequency, since these could
  // change (and making the interference relevant) while the interference is
  // still incoming.

  Ptr<LoraInterferenceHelper::Event> event;
  event = m_interference.Add (duration, rxPowerDbm, sf, packet, frequencyMHz);

  // Switch on the current PHY state
  switch (m_state)
    {
    // In the SLEEP, TX and RX cases we cannot receive the packet: we only add
    // it to the list of interferers and do not schedule an EndReceive event for
    // it.
    case SLEEP:
      {
        NS_LOG_INFO ("Dropping packet because device is in SLEEP state");
        break;
      }
    case TX:
      {
        NS_LOG_INFO ("Dropping packet because device is in TX state");
        break;
      }
    case DEAD:
      {
        NS_LOG_INFO ("Dropping packet because end-device's battery is discharged");
        break;
      }
    case RX:
      {
        NS_LOG_INFO ("Dropping packet because device is already in RX state");
        break;
      }
    // If we are in STANDBY mode, we can potentially lock on the currently
    // incoming transmission
    case STANDBY:
      {
        // There are a series of properties the packet needs to respect in order
        // for us to be able to lock on it:
        // - It's on frequency we are listening on
        // - It uses the SF we are configured to look for
        // - Its receive power is above the device sensitivity for that SF

        // Flag to signal whether we can receive the packet or not
        bool canLockOnPacket = true;

        // Save needed sensitivity
        double sensitivity = EndDeviceLoraPhy::sensitivity[unsigned(sf)-7];

        // Check frequency
        //////////////////
        if (!IsOnFrequency (frequencyMHz))
          {
            NS_LOG_INFO ("Packet lost because it's on frequency " <<
                         frequencyMHz << " MHz and we are listening at " <<
                         m_frequency << " MHz");

            // Fire the trace source for this event.
            if (m_device)
              {
                m_wrongFrequency (packet, m_device->GetNode ()->GetId ());
              }
            else
              {
                m_wrongFrequency (packet, 0);
              }

            canLockOnPacket = false;
          }

        // Check Spreading Factor
        /////////////////////////
        if (sf != m_sf)
          {
            NS_LOG_INFO ("Packet lost because it's using SF" << unsigned(sf) <<
                         ", while we are listening for SF" << unsigned(m_sf));

            // Fire the trace source for this event.
            if (m_device)
              {
                m_wrongSf (packet, m_device->GetNode ()->GetId ());
              }
            else
              {
                m_wrongSf (packet, 0);
              }

            canLockOnPacket = false;
          }

        // Check Sensitivity
        ////////////////////
        if (rxPowerDbm < sensitivity)
          {
            NS_LOG_INFO ("Dropping packet reception of packet with sf = " <<
                         unsigned(sf) << " because under the sensitivity of " <<
                         sensitivity << " dBm");

            // Fire the trace source for this event.
            if (m_device)
              {
                m_underSensitivity (packet, m_device->GetNode ()->GetId ());
              }
            else
              {
                m_underSensitivity (packet, 0);
              }

            canLockOnPacket = false;
          }

        // Check if one of the above failed
        ///////////////////////////////////
        if (canLockOnPacket)
          {
            // Switch to RX state
            // EndReceive will handle the switch back to STANDBY state
            SwitchToRx ();

            // Create the event to calculate the energy consumption of this transmission event

            Ptr<LoraEnergyConsumptionHelper::Event> Event;
            Event = m_conso.Add (2, duration, rxPowerDbm, sf, frequencyMHz, m_device->GetNode ()->GetId ());

            // Schedule the end of the reception of the packet
            NS_LOG_INFO ("Scheduling reception of a packet. End in " <<
                         duration.GetSeconds () << " seconds");

            Simulator::Schedule (duration, &LoraPhy::EndReceive, this, packet,
                                 event);

            // Fire the beginning of reception trace source
            m_phyRxBeginTrace (packet);
          }
      }
    }
}

void
EndDeviceLoraPhy::EndReceive (Ptr<Packet> packet,
                              Ptr<LoraInterferenceHelper::Event> event)
{
  NS_LOG_FUNCTION (this << packet << event);

  // Fire the trace source
  m_phyRxEndTrace (packet);

  // Update the packet's LoraTag
  LoraTag tag;
  packet->RemovePacketTag (tag);
  uint32_t SenderID = tag.GetSenderID();
  packet->AddPacketTag (tag);


  // Call the LoraInterferenceHelper to determine whether there was destructive
  // interference on this event.
  bool packetDestroyed = m_interference.IsDestroyedByInterference (event);

  // Fire the trace source if packet was destroyed
  if (packetDestroyed)
    {
      NS_LOG_INFO ("Packet destroyed by interference");

      bool OnThePreamble = m_interference.GetOnThePreamble ();
      Time colstart = m_interference.GetColStart ();
      Time colend = m_interference.GetColEnd ();
	  uint8_t colsf = m_interference.GetSF ();

      // Update the packet's LoraTag
      LoraTag tag;
      packet->RemovePacketTag (tag);
      tag.SetDestroyedBy (packetDestroyed);
      packet->AddPacketTag (tag);

      NS_LOG_DEBUG ("Packet destroyed - collision Parameters: start time = " << colstart.GetSeconds() << " end time = " << colend.GetSeconds() << " on the preamble = " << OnThePreamble <<" Sender ID = " << SenderID << " SF = " << unsigned(colsf));

      if (m_device)
        {
    	  m_interferedPacket (packet, m_device->GetNode ()->GetId () , SenderID, colsf, colstart, colend, OnThePreamble);
        }
      else
        {
          m_interferedPacket (packet, 0, SenderID, colsf, colstart, colend, OnThePreamble);
        }

    }
  else
    {
      NS_LOG_INFO ("Packet received correctly");

      if (m_device)
        {
          m_successfullyReceivedPacket (packet, m_device->GetNode ()->GetId (), SenderID);
        }
      else
        {
          m_successfullyReceivedPacket (packet, 0, SenderID);
        }

      // If there is one, perform the callback to inform the upper layer
      if (!m_rxOkCallback.IsNull ())
        {
          m_rxOkCallback (packet);
        }

    }
  // Automatically switch to Standby in either case

  if (not IsDead ()) {
  SwitchToStandby ();
  }
}

void
EndDeviceLoraPhy::Consumption (Ptr<LoraEnergyConsumptionHelper::Event> event, uint32_t NodeId, int ConsoType)
{

double event_conso;
double Cumulative_Tx_Conso = GetTxConso();
double Cumulative_Rx_Conso = GetRxConso();
double Cumulative_Stb_Conso = GetStbConso();
double Cumulative_Sleep_Conso = GetSleepConso();

	  // Check the type of event in order to compute the corresponding energy consumption

switch(ConsoType) {

	  case 1 : event_conso  = m_conso.TxConso(event);
	  	  	   Cumulative_Tx_Conso = Cumulative_Tx_Conso + event_conso;
	  	  	   m_cumulative_tx_conso = Cumulative_Tx_Conso;
	           break;
	  case 2 : event_conso =  m_conso.RxConso (event);
	  	  	   Cumulative_Rx_Conso = Cumulative_Rx_Conso + event_conso;
	  	  	   m_cumulative_rx_conso = Cumulative_Rx_Conso;
	           break;
	  case 3 : event_conso =  m_conso.StbConso (event);
	  	  	   Cumulative_Stb_Conso = Cumulative_Stb_Conso + event_conso;
	  	  	   m_cumulative_stb_conso = Cumulative_Stb_Conso;
	           break;
	  case 4 : event_conso =  m_conso.SleepConso (event);
	  	  	   Cumulative_Sleep_Conso = Cumulative_Sleep_Conso + event_conso;
	  	  	   m_cumulative_sleep_conso = Cumulative_Sleep_Conso;
	           break;
	  default :
	           break;
	  }

double battery_level = GetBatteryLevel ();
battery_level = battery_level - event_conso;

m_battery_level = battery_level;

switch(ConsoType) {

	case 1 : m_consumption (NodeId, ConsoType, Cumulative_Tx_Conso, battery_level);
			 break;
	case 2 : m_consumption (NodeId, ConsoType, Cumulative_Rx_Conso, battery_level);
         	 break;
	case 3 : m_consumption (NodeId, ConsoType, Cumulative_Stb_Conso, battery_level);
         	 break;
	case 4 : m_consumption (NodeId, ConsoType, Cumulative_Sleep_Conso, battery_level);
         	 break;
	default :
			 break;
}


if (battery_level <= 0) {
	SwitchToDead();
	m_dead_device(NodeId, m_cumulative_tx_conso, m_cumulative_rx_conso, m_cumulative_stb_conso, m_cumulative_sleep_conso, Simulator::Now ());
	}

NS_LOG_FUNCTION (this << battery_level << event);

}

void
EndDeviceLoraPhy::StateDuration (Time time_stamp, int current_state)
{
	double rxnull = 0;
	uint8_t sfnull = 0;

	Time last_time_stamp = GetLastTimeStamp ();
    int last_state = GetLastState ();

    NS_LOG_FUNCTION (this << last_state << current_state);

	Time duration = time_stamp - last_time_stamp;

	if (last_state == 3 || last_state == 4)
	{
		Ptr<LoraEnergyConsumptionHelper::Event> Event;
		Event = m_conso.Add (last_state, duration, rxnull, sfnull, rxnull, m_device->GetNode ()->GetId ());
		Consumption (Event, m_device->GetNode ()->GetId (), last_state);
	}

	m_last_state = current_state;
	m_last_time_stamp = time_stamp;
}

bool
EndDeviceLoraPhy::IsDead (void)
{
  return m_state == DEAD;
}

bool
EndDeviceLoraPhy::IsTransmitting (void)
{
  return m_state == TX;
}

bool
EndDeviceLoraPhy::IsOnFrequency (double frequencyMHz)
{
  return m_frequency == frequencyMHz;
}

void
EndDeviceLoraPhy::SetFrequency (double frequencyMHz)
{
  m_frequency = frequencyMHz;
}

void
EndDeviceLoraPhy::SwitchToStandby (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_state == DEAD)
    {
      NS_LOG_INFO ("Won't switch to standby because the end-device is DEAD");
      return;
    }

  m_state = STANDBY;
  StateDuration (Simulator::Now (), 3);
  NS_LOG_FUNCTION (this << "STB" << Simulator::Now ().GetSeconds ());
}

void
EndDeviceLoraPhy::SwitchToRx (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_state == DEAD)
    {
      NS_LOG_INFO ("Won't switch to rx because the end-device is DEAD");
      return;
    }

    NS_ASSERT (m_state == STANDBY);

  m_state = RX;
  StateDuration (Simulator::Now (), 2);
  NS_LOG_FUNCTION (this << "RX" << Simulator::Now ().GetSeconds ());
}

void
EndDeviceLoraPhy::SwitchToTx (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_state == DEAD)
    {
      NS_LOG_INFO ("Won't switch to tx because the end-device is DEAD");
      return;
    }

  NS_ASSERT (m_state != RX);
  m_state = TX;
  StateDuration (Simulator::Now (), 1);
  NS_LOG_FUNCTION (this << "TX" << Simulator::Now ().GetSeconds ());

}

void
EndDeviceLoraPhy::SwitchToSleep (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_state == DEAD)
    {
      NS_LOG_INFO ("Won't switch to sleep because the end-device is DEAD");
      return;
    }

  m_state = SLEEP;
  StateDuration (Simulator::Now (), 4);

  NS_LOG_FUNCTION (this << "SLEEP" << Simulator::Now ().GetSeconds ());

}

void
EndDeviceLoraPhy::SwitchToDead (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_state = DEAD;

}

EndDeviceLoraPhy::State
EndDeviceLoraPhy::GetState (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  return m_state;
}
}
