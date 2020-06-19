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

#ifndef NETWORK_SERVER_HELPER_H
#define NETWORK_SERVER_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/simple-network-server.h"
#include <stdint.h>
#include <string>

namespace ns3 {

/**
 * This class can install Network Server applications on multiple nodes at once.
 */
class NetworkServerHelper
{
public:
  NetworkServerHelper ();

  ~NetworkServerHelper ();

  void SetAttribute (std::string name, const AttributeValue &value);

  ApplicationContainer Install (NodeContainer c);

  ApplicationContainer Install (Ptr<Node> node);

  /**
   * Set which gateways will need to be connected to this NS.
   */
  void SetGateways (NodeContainer gateways);

  /**
   * Set which end devices will be managed by this NS.
   */
  void SetEndDevices (NodeContainer endDevices);

  void SetJammers (uint32_t jammers);

  Ptr<SimpleNetworkServer> m_ns = 0;

  void SetNS (Ptr<SimpleNetworkServer> app);

  Ptr<SimpleNetworkServer> GetNS (void);

  void SetStopTime (Time stop);

  Time m_stop_time = Seconds(0);

  void SetBuffer (int buffer_length);

  void  SetInterArrival (void);

  bool m_interarrivaltime = false;

  // Parameters EWMA

  void  SetEWMA (bool ewma, double target, double lambda, double ucl, double lcl);

  int m_buffer_length = 0;
  double m_target = 0;
  double m_lambda = 0;
  bool m_ewma = true;
  double m_ucl = 0;
  double m_lcl = 0;

  void SetACKParams (bool two_rx, double ackfrequency, int ack_sf, bool acksamesf, int acklength);

  // Variables related to ACK

  bool m_acksamesf;
  bool m_two_rx;
  double m_ackfrequency;
  int m_ack_sf;
  int m_acklength;

private:
  Ptr<Application> InstallPriv (Ptr<Node> node);

  ObjectFactory m_factory;

  NodeContainer m_gateways;   //!< Set of gateways to connect to this NS

  NodeContainer m_endDevices;   //!< Set of endDevices to connect to this NS

  uint32_t m_jammers;   //!< Set of endDevices to connect to this NS

  PointToPointHelper p2pHelper; //!< Helper to create PointToPoint links
};

} // namespace ns3

#endif /* NETWORK_SERVER_HELPER_H */
