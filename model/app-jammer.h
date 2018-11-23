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

#ifndef APP_JAMMER_H
#define APP_JAMMER_H

#include "ns3/application.h"
#include "ns3/nstime.h"
#include "ns3/lora-mac.h"
#include "ns3/jammer-lora-phy.h"
#include "ns3/jammer-lora-mac.h"
#include "ns3/attribute.h"

namespace ns3 {

class AppJammer : public Application {

public:

	  class Event : public SimpleRefCount<AppJammer::Event>
	  {

	public:

	  Event (Ptr<Packet> packet, double rxPowerDbm,
		        uint8_t sf, Time duration, double frequencyMHz, double Type);
	  ~Event ();

	private:

	  /**
	    * The packet this event was generated for.
	    */
	  Ptr<Packet> m_packet;

	  /**
	   * The power of this event in dBm (at the device).
	   */
	  double m_rxPowerdBm;

	  /**
	   * The spreading factor of this signal.
	   */
	  uint8_t m_sf;

	  /**
	   * Duration of the packet.
	   */
	  Time m_duration;

	  /**
	   * The frequency this event was on.
	   */
	  double m_frequencyMHz;

	  };

  AppJammer ();
  ~AppJammer ();

  Ptr<AppJammer::Event> Add (Ptr<Packet> packet,
		  	  	  	  	  	 double rxPowerDbm,
							 uint8_t sf,
							 Time duration,
							 double frequencyMHz,
							 double Type);

  static TypeId GetTypeId (void);

  /**
   * Set the sending interval
   * \param interval the interval between two packet sendings
   */
  void SetInterval (Time interval);

  /**
   * Get the sending inteval
   * \returns the interval between two packet sends
   */
  Time GetInterval (void) const;

  /**
   * Set the initial delay of this application
   */
  void SetInitialDelay (Time delay);

  /**
   * Send a packet using the LoraNetDevice's Send method
   */
  void SendPacket (void);

  /**
   * Start the application by scheduling the first SendPacket event
   */
  void StartApplication (void);

  /**
   * Stop the application
   */
  void StopApplication (void);

  /**
   * Set packet size
   */
  void SetPktSize  (uint16_t size);

  /**
   * Set Jammer type
   */

  void SelectType (Ptr<Packet> packet, double rxPowerDbm,
  		  	  	uint8_t sf, Time duration, double frequencyMHz, double Type);

  void JammerI (Ptr<Packet> packet, double rxPowerDbm,
  		  	  	uint8_t sf, Time duration, double frequencyMHz);


private:

  /**
   * The interval between to consecutive send events
   */
  Time m_interval;

  /**
   * The initial delay of this application
   */
  Time m_initialDelay;

  /**
   * The sending event scheduled as next
   */
  EventId m_sendEvent;

  /**
   * The MAC layer of this node
   */
  Ptr<JammerLoraMac> m_mac;

  /**
   * The PHY layer of this node
   */
  Ptr<LoraPhy> m_phy;

  /**
   * The size of the packets this application sends
   */
  uint16_t m_pktSize;

  /**
   * Jammer type
   */
  uint8_t m_jammer_type;


};

} //namespace ns3

#endif /* APP_JAMMER */
