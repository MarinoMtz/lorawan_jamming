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

#include "ns3/forwarder.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Forwarder");

NS_OBJECT_ENSURE_REGISTERED (Forwarder);

TypeId
Forwarder::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Forwarder")
    .SetParent<Application> ()
    .AddConstructor<Forwarder> ()
    .SetGroupName ("lorawan");
  return tid;
}

Forwarder::Forwarder ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

Forwarder::~Forwarder ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
Forwarder::SetPointToPointNetDevice (Ptr<PointToPointNetDevice>
                                     pointToPointNetDevice)
{
  NS_LOG_FUNCTION (this << pointToPointNetDevice);

  m_pointToPointNetDevice = pointToPointNetDevice;
}

void
Forwarder::SetLoraNetDevice (Ptr<LoraNetDevice> loraNetDevice)
{
  NS_LOG_FUNCTION (this << loraNetDevice);

  m_loraNetDevice = loraNetDevice;
}

bool
Forwarder::ReceiveFromLora (Ptr<NetDevice> loraNetDevice, Ptr<const Packet>
                            packet, uint16_t protocol, const Address& sender)
{
  NS_LOG_FUNCTION (this << packet << protocol << sender);

  Ptr<Packet> packetCopy = packet->Copy ();

  m_pointToPointNetDevice->Send (packetCopy,
                                 m_pointToPointNetDevice->GetBroadcast (),
                                 0x800);

  return true;
}

bool
Forwarder::ReceiveFromPointToPoint (Ptr<NetDevice> pointToPointNetDevice,
                                    Ptr<const Packet> packet, uint16_t protocol,
                                    const Address& sender)
{
  NS_LOG_FUNCTION (this << packet << protocol << sender);

  Ptr<Packet> packetCopy = packet->Copy ();

  m_loraNetDevice->Send (packetCopy);

  return true;
}

void
Forwarder::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  // TODO Make sure we are connected to both needed devices
}

void
Forwarder::StopApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  // TODO Get rid of callbacks
}

}
