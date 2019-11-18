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

#ifndef PERIODIC_SENDER_H
#define PERIODIC_SENDER_H

#include "ns3/application.h"
#include "ns3/nstime.h"
#include "ns3/lora-mac.h"
#include "ns3/attribute.h"

using namespace std;
namespace ns3 {

class PeriodicSender : public Application {
public:

  PeriodicSender ();
  ~PeriodicSender ();

  static TypeId GetTypeId (void);

  /**
   * Set the sending interval
   * \param interval the interval between two packet sendings
   */

  void SetSimTime (Time simtime);

  Time GetSimTime (void) const;

  void IncreasePacketID (void);

  void DecreasePacketID (void);

  uint32_t GetPacketID (void);

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
  void UnconfirmedTraffic (void);

  void ConfirmedTraffic (uint8_t ntx, uint32_t ID, bool confirmed);


  void SendPacketMacUnconfirmed ();

  void SendPacketMacConfirmed (uint32_t ID, uint8_t ntx);

  /**
   * Start the application by scheduling the first SendPacket event
   */
  void StartApplication (void);

  /**
   * Stop the application
   */
  void StopApplication (void);


  void SetPktSize  (uint16_t size);

  void SetSpreadingFactor  (uint8_t sf);

  uint8_t GetSpreadingFactor (void);

  void SetExp (bool Exp);

  bool GetExp (void);

  double GetNextTxTime (void);

  void SetRetransmissions (bool retrans, uint8_t rxnumber);

  bool GetRetransmissions (void);


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

  Ptr<PeriodicSender> m_ap;

  /**
   * The size of the packets this application sends
   */
  uint16_t m_pktSize;

  uint32_t m_pktID;

  bool m_exp;
  bool m_retransmissions;

  bool m_ransf;
  uint8_t m_sf;

  double m_mean;

  Ptr<UniformRandomVariable> m_randomdelay;
  Ptr<ExponentialRandomVariable> m_exprandomdelay;
  vector<double> send_times;

  /**
   * Duty Cycle
   */

  double m_dutycycle;
  Time m_simtime;
  double cumultime;

  vector<uint32_t> sendtries;

  TracedCallback< uint8_t > m_sendpacket;

  uint8_t m_rxnumber;

};

} //namespace ns3

#endif /* SENDER_APPLICATION */
