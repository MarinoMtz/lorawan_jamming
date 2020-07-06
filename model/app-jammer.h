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

#ifndef APP_JAMMER_H
#define APP_JAMMER_H

#include "ns3/application.h"
#include "ns3/nstime.h"
#include "ns3/lora-mac.h"
#include "ns3/jammer-lora-mac.h"
#include "ns3/attribute.h"
#include <vector>
#include <numeric>

using namespace std;

namespace ns3 {

class AppJammer : public Application {

public:

  AppJammer ();
  ~AppJammer ();

  static TypeId GetTypeId (void);


  void SetSimTime (Time simtime);


  Time GetSimTime (void) const;

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
   * Set packet size
   */
  void SetPktSize  (uint16_t size);

  /**
   * Set and get duty-cyle
   */

  void SetDC  (double dutycycle);

  double GetDC (void);

  /**
   * Set bool to Exponential distribution of arrival time
   */

  void SetExp (bool Exp);

  bool GetExp (void);

  /**
   * Set bool to Random SF at each transmission
   */

  void SetRanSF (bool ransf); // Set a specific SF

  void SetSF (void); // Set a random frequency if ransf is set to 1

  void SetRanChannel (bool RanCh);

  bool GetRanSF (void);

  /**
   * Set and Get the SF that will be used for this tx
   */

  void SetSpreadingFactor (uint8_t sf);

  uint8_t GetSpreadingFactor ();

  void SetFrequency (double Freq);

  void SetLambda (double lambda);

  double GetLambda (void);

  void SetLength (double m_length);

  /**
   * Send a packet using the LoraNetDevice's Send method
   */
  void SendPacket (void);

  void SendPacketMac (uint8_t sf, double Freq);

  /**
   * Start the application by scheduling the first SendPacket event
   */
  void StartApplication (void);

  void SetChannel (void);


  /**
   * Stop the application
   */
  void StopApplication (void);


private:

  Time m_simtime;

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

  /**
   * Duty Cycle
   */

  double m_dutycycle;

  /**
   * Exponential distribution of inter-arrival time
   */

  bool m_exp;
  bool m_ransf;
  bool m_ranch;
  uint8_t m_sf;

  Ptr<UniformRandomVariable> m_randomsf;
  Ptr<ExponentialRandomVariable> m_exprandomdelay;

  vector<double> send_times;
  double cumultime;
  double sent = 0;

  double m_lambda;
  double m_frequency;

};

} //namespace ns3

#endif /* APP_JAMMER */
