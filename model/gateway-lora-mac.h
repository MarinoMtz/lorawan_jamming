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

#ifndef GATEWAY_LORA_MAC_H
#define GATEWAY_LORA_MAC_H

#include "ns3/lora-mac.h"
#include "ns3/lora-tag.h"

namespace ns3 {

class GatewayLoraMac : public LoraMac
{
public:

  static TypeId GetTypeId (void);

  GatewayLoraMac();
  virtual ~GatewayLoraMac();

  // Implementation of the LoraMac interface
  virtual bool Send (Ptr<Packet> packet);

  // Implementation of the LoraMac interface
  bool IsTransmitting (void);

  // Implementation of the LoraMac interface
  virtual void Receive (Ptr<Packet const> packet);

  // Implementation of the LoraMac interface
  virtual void TxFinished (Ptr<Packet const> packet);

  /**
   * Return the next time at which we will be able to transmit.
   *
   * \return The next transmission time.
   */
  Time GetWaitingTime (double frequency);
private:

protected:

};

} /* namespace ns3 */

#endif /* GATEWAY_LORA_MAC_H */
