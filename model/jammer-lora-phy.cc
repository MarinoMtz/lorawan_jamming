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
#include "ns3/jammer-lora-phy.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/lora-tag.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("JammerLoraPhy");

NS_OBJECT_ENSURE_REGISTERED (JammerLoraPhy);

TypeId
JammerLoraPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::JammerLoraPhy")
    .SetParent<LoraPhy> ()
    .SetGroupName ("lorawan")
    .AddConstructor<JammerLoraPhy> ()
    .AddTraceSource ("LostPacketBecauseWrongFrequency",
                     "Trace source indicating a packet "
                     "could not be correctly decoded because"
                     "the ED was listening on a different frequency",
                     MakeTraceSourceAccessor (&JammerLoraPhy::m_wrongFrequency),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("LostPacketBecauseWrongSpreadingFactor",
                     "Trace source indicating a packet "
                     "could not be correctly decoded because"
                     "the ED was listening for a different Spreading Factor",
                     MakeTraceSourceAccessor (&JammerLoraPhy::m_wrongSf),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("EndDeviceState",
                     "The current state of the device",
                     MakeTraceSourceAccessor (&JammerLoraPhy::m_state),
                     "ns3::TracedValueCallback::JammerLoraPhy::State");
  return tid;
}

// Initialize the device with some common settings.
// These will then be changed by helpers.
JammerLoraPhy::JammerLoraPhy () :

  m_state (STANDBY),
  m_frequency (868.3),
  m_txpower (14),
  m_sf (12),
  m_preamble (0.012544),
  m_jamType (0),
  m_packetsize (0),
  m_randomsf(false),
  m_allsf(false)
{
  m_uniformRV = CreateObject<UniformRandomVariable> ();
}

JammerLoraPhy::~JammerLoraPhy ()
{
}

// Downlink sensitivity (from SX1272 datasheet)
// {SF7, SF8, SF9, SF10, SF11, SF12}
// These sensitivites are for a bandwidth of 125000 Hz

const double JammerLoraPhy::sensitivity[6] =
{-124, -127, -130, -133, -135, -137};

/**
 * An uniform random variable, used to select the SF when it is not fixed.
 */

void
JammerLoraPhy::SetSpreadingFactor (uint8_t sf)
{
  m_sf = sf;
}

void
JammerLoraPhy::SetRandomSpreadingFactor ()
{
  uint8_t random = std::floor (m_uniformRV->GetValue (7, 12));
  m_sf = random;
}


void
JammerLoraPhy::SetRandomSF ()
{
  m_randomsf = true;
}

void
JammerLoraPhy::SetAllSF ()
{
	m_allsf = true;
}

uint8_t
JammerLoraPhy::GetSpreadingFactor (void)
{
  return m_sf;
}

void
JammerLoraPhy::SetType (double type)
{
  m_jamType = type;
  NS_LOG_INFO ("Jammer Type: " << type);
}

void
JammerLoraPhy::Send (Ptr<Packet> packet, LoraTxParameters txParams,
                        double frequencyMHz, double txPowerDbm)
{
  NS_LOG_FUNCTION (this << packet << txParams << frequencyMHz << txPowerDbm);

  NS_LOG_INFO ("Current state: " << m_state);

  // We must be either in STANDBY mode to send a packet

  if (m_state != STANDBY)
    {
      NS_LOG_INFO ("Cannot send because device is currently not in STANDBY mode");
      return;
    }

  if (m_allsf)
  {
	for (uint8_t j = 7; j != 13; ++j)
	    {
		  txParams.sf = j;

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
		  tag.SetJammer (uint8_t (1));
		  packet->AddPacketTag (tag);

		  // Send the packet over the channel
		  NS_LOG_INFO ("Sending the packet in the channel");

		  m_channel->Send (this, packet, txPowerDbm, txParams, duration, frequencyMHz);
		  Simulator::Schedule (duration, &JammerLoraPhy::SwitchToStandby, this);

		  // Schedule the txFinished callback, if it was set
		  // The call is scheduled just after the switch to standby in case the upper
		  // layer wishes to change the state. This ensures that it will find a PHY in
		  // STANDBY mode.
		  if (!m_txFinishedCallback.IsNull ())
		    {
		      Simulator::Schedule (duration + NanoSeconds (10),
		                           &JammerLoraPhy::m_txFinishedCallback, this,
		                           packet);
		    }

		  NS_LOG_FUNCTION (this << duration);

		  // Call the trace source
		  if (m_device)
		    {
		      m_startSending (packet, m_device->GetNode ()->GetId (), frequencyMHz, txParams.sf);
		    }
		  else
		    {
		      m_startSending (packet, 0, 0, 0);
		    }
	    }
  }

  else if (m_randomsf)
  {
	SetRandomSpreadingFactor ();
	txParams.sf = GetSpreadingFactor ();
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
	tag.SetJammer (uint8_t (1));
	packet->AddPacketTag (tag);

	// Send the packet over the channel
	NS_LOG_INFO ("Sending the packet in the channel");

	m_channel->Send (this, packet, txPowerDbm, txParams, duration, frequencyMHz);
	Simulator::Schedule (duration, &JammerLoraPhy::SwitchToStandby, this);

	// Schedule the txFinished callback, if it was set
	// The call is scheduled just after the switch to standby in case the upper
	// layer wishes to change the state. This ensures that it will find a PHY in
	// STANDBY mode.

	if (!m_txFinishedCallback.IsNull ())
	   {
	   Simulator::Schedule (duration + NanoSeconds (10), &JammerLoraPhy::m_txFinishedCallback, this, packet);
	   }

	   NS_LOG_FUNCTION (this << duration);

	  // Call the trace source
	if (m_device)
	   {
		m_startSending (packet, m_device->GetNode ()->GetId (), frequencyMHz, txParams.sf);
	   }
	   else {
		   m_startSending (packet, 0, 0, 0);
	   }
   }
}

void
JammerLoraPhy::DirectSend (Ptr<Packet> packet, LoraTxParameters txParams,
                        double frequencyMHz, double txPowerDbm)
{
	NS_LOG_FUNCTION (this << packet << txParams << frequencyMHz << txPowerDbm);

	NS_LOG_INFO ("Current state: " << m_state);

	// We must be either in STANDBY mode to send a packet

	if (m_state != STANDBY)
	   {
	     NS_LOG_INFO ("Cannot send because device is currently not in STANDBY mode");
	     return;
	   }

	//SwitchToTx ();

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
	tag.SetJammer (uint8_t (1));
	packet->AddPacketTag (tag);

	// Send the packet over the channel
	NS_LOG_INFO ("Sending the packet in the channel");
	m_channel->Send (this, packet, txPowerDbm, txParams, duration, frequencyMHz);
	Simulator::Schedule (duration, &JammerLoraPhy::SwitchToStandby, this);

	// Schedule the txFinished callback, if it was set
	// The call is scheduled just after the switch to standby in case the upper
	// layer wishes to change the state. This ensures that it will find a PHY in
	// STANDBY mode.

	if (!m_txFinishedCallback.IsNull ())
		{
		Simulator::Schedule (duration + NanoSeconds (10), &JammerLoraPhy::m_txFinishedCallback, this, packet);
		}

	NS_LOG_FUNCTION (this << duration);

	// Call the trace source
	if (m_device)
	{
		m_startSending (packet, m_device->GetNode ()->GetId (), frequencyMHz, txParams.sf);
	} else {
		m_startSending (packet, 0, 0, 0);
	}
}


void
JammerLoraPhy::StartReceive (Ptr<Packet> packet, double rxPowerDbm,
                                uint8_t sf, Time duration, double frequencyMHz)
{
  LoraTxParameters txParams;
  Time jamduration = GetCAD (txParams);

  NS_LOG_FUNCTION (this << packet << rxPowerDbm << unsigned (sf) << duration << frequencyMHz);
  NS_LOG_INFO ("Jammer receive");


  // Update the packet's LoraTag
  LoraTag tag;
  packet->RemovePacketTag (tag);
  uint32_t SenderID = tag.GetSenderID();
  packet->AddPacketTag (tag);


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
    // In TX and RX cases we cannot receive the packet: we only add
    // it to the list of interferers and do not schedule an EndReceive event for
    // it.
    case TX:
      {
        NS_LOG_INFO ("Dropping packet because device is in TX state");
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
        double sensitivity = JammerLoraPhy::sensitivity[unsigned(sf)-7];


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
                m_underSensitivity (packet, m_device->GetNode ()->GetId (), SenderID, frequencyMHz, sf);
              }
            else
              {
                m_underSensitivity (packet, 0, SenderID, frequencyMHz, sf);
              }

            canLockOnPacket = false;
          }

        else if (m_jamType == 1)
        	{
        		Ptr<Packet> jampacket;
        		jampacket = Create<Packet>(m_packetsize);
        		Simulator::Schedule (jamduration, &JammerLoraPhy::DirectSend, this, jampacket, txParams, frequencyMHz, m_txpower);

        		//DirectSend (jampacket, txParams, frequencyMHz, m_txpower);
        	}

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

        else if (m_jamType == 2)
        	{
    			Ptr<Packet> jampacket;
    			jampacket = Create<Packet>(m_packetsize);
        		Simulator::Schedule (jamduration, &JammerLoraPhy::DirectSend, this, jampacket, txParams, frequencyMHz, m_txpower);
    			//DirectSend (jampacket, txParams, frequencyMHz, m_txpower);
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

        // Check if one of the above failed
        ///////////////////////////////////
        if (canLockOnPacket)
          {
            // Switch to RX state
            // EndReceive will handle the switch back to STANDBY state
            SwitchToRx ();

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
JammerLoraPhy::EndReceive (Ptr<Packet> packet,
                              Ptr<LoraInterferenceHelper::Event> event)
{
  NS_LOG_FUNCTION (this << packet << event);

  SwitchToStandby ();
}


bool
JammerLoraPhy::IsTransmitting (void)
{
  return m_state == TX;
}

bool
JammerLoraPhy::IsOnFrequency (double frequencyMHz)
{
  return m_frequency == frequencyMHz;
}

void
JammerLoraPhy::SetFrequency (double frequencyMHz)
{
  m_frequency = frequencyMHz;
}

void
JammerLoraPhy::SetTxPower (double TxPower)
{
  m_txpower = TxPower;
}

void
JammerLoraPhy::SetPacketSize (uint16_t PacketSize)
{
  m_packetsize = PacketSize;
}

void
JammerLoraPhy::SwitchToStandby (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_state = STANDBY;
  NS_LOG_FUNCTION (this << "STB" << Simulator::Now ().GetSeconds ());
}

void
JammerLoraPhy::SwitchToRx (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_ASSERT (m_state == STANDBY);

  m_state = RX;
  NS_LOG_FUNCTION (this << "RX" << Simulator::Now ().GetSeconds ());
}

void
JammerLoraPhy::SwitchToTx (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_ASSERT (m_state != RX);
  m_state = TX;
  NS_LOG_FUNCTION (this << "TX" << Simulator::Now ().GetSeconds ());

}

JammerLoraPhy::State
JammerLoraPhy::GetState (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  return m_state;
}
}
