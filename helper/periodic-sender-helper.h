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

#ifndef PERIODIC_SENDER_HELPER_H
#define PERIODIC_SENDER_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/periodic-sender.h"
#include <stdint.h>
#include <string>

namespace ns3 {

/**
 * This class can be used to install PeriodicSender applications on a wide
 * range of nodes.
 */

class PeriodicSenderHelper
{
public:
  PeriodicSenderHelper ();

  ~PeriodicSenderHelper ();

  void SetAttribute (std::string name, const AttributeValue &value);

  ApplicationContainer Install (NodeContainer c) const;

  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Set the period to be used by the applications created by this helper.
   *
   * A value of Seconds (0) results in randomly generated periods according to
   * the model contained in the TR 45.820 document.
   *
   * \param period The period to set
   */
  void SetSimTime (Time simtime);

  void SetPeriod (Time period);

  void SetPacketSize (uint16_t size);

  void SetExp (bool Exp);

  void SetRetransmissions (bool retrans, uint32_t rxnumber, double percentage_rtx);

  void SetSpreadingFactor (uint8_t sf);

  void SetType (uint8_t);

  Ptr<PeriodicSender> GetApp (void);

private:

  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_factory;

  Ptr<UniformRandomVariable> m_initialDelay;

  Time m_period; //!< The period with which the application will be set to send
                 // messages
  uint16_t m_size;

  bool m_exp;

  bool m_retransmissions;

  uint32_t m_rxnumber;

  double m_percentage_rtx;

  uint8_t m_sf;

  Time m_simtime;

};

} // namespace ns3

#endif /* PERIODIC_SENDER_HELPER_H */
