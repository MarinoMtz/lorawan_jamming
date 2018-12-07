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
#include "ns3/jammer-lora-mac.h"
#include "ns3/attribute.h"

namespace ns3 {

class AppJammer : public Application {

public:

  AppJammer ();
  ~AppJammer ();

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
  Ptr<LoraMac> m_mac;

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

  Ptr<UniformRandomVariable> m_randomdelay;


};

} //namespace ns3

#endif /* APP_JAMMER */