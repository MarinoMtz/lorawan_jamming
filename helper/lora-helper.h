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

#ifndef LORA_HELPER_H
#define LORA_HELPER_H

#include "ns3/lora-phy-helper.h"
#include "ns3/lora-mac-helper.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/net-device.h"
#include "ns3/lora-net-device.h"

namespace ns3 {

/**
 * Helps to create LoraNetDevice objects
 *
 * This class can help create a large set of similar LoraNetDevice objects and
 * configure a large set of their attributes during creation.
 */
class LoraHelper
{
public:
  virtual ~LoraHelper ();

  LoraHelper ();

  /**
   * Install LoraNetDevices on a list of nodes
   *
   * \param phy the PHY helper to create PHY objects
   * \param mac the MAC helper to create MAC objects
   * \param c the set of nodes on which a lora device must be created
   * \returns a device container which contains all the devices created by this
   * method.
   */
  virtual NetDeviceContainer Install (const LoraPhyHelper &phyHelper,
                                      const LoraMacHelper &macHelper,
                                      NodeContainer c) const;

  /**
   * Install LoraNetDevice on a single node
   *
   * \param phy the PHY helper to create PHY objects
   * \param mac the MAC helper to create MAC objects
   * \param node the node on which a lora device must be created
   * \returns a device container which contains all the devices created by this
   * method.
   */
  virtual NetDeviceContainer Install (const LoraPhyHelper &phyHelper,
                                      const LoraMacHelper &macHelper,
                                      Ptr<Node> node) const;
};

} //namespace ns3

#endif /* LORA_HELPER_H */
