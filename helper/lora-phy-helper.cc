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

#include "ns3/lora-phy-helper.h"
#include "ns3/log.h"
#include "ns3/sub-band.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoraPhyHelper");

LoraPhyHelper::LoraPhyHelper ()
{
}

void
LoraPhyHelper::SetChannel (Ptr<LoraChannel> channel)
{
  m_channel = channel;
}

void
LoraPhyHelper::SetDeviceType (enum DeviceType dt)
{

  NS_LOG_FUNCTION (this << dt);
  switch (dt)
    {
    case GW:
      m_phy.SetTypeId ("ns3::GatewayLoraPhy");
      break;
    case ED:
      m_phy.SetTypeId ("ns3::EndDeviceLoraPhy");
      break;
    case JM:
      m_phy.SetTypeId ("ns3::JammerLoraPhy");
      break;
    }
}

void
LoraPhyHelper::Set (std::string name, const AttributeValue &v)
{
  m_phy.Set (name, v);
}

Ptr<LoraPhy>
LoraPhyHelper::Create (Ptr<Node> node, Ptr<NetDevice> device) const
{
  NS_LOG_FUNCTION (this << node << device);

  // Create the PHY and set its channel
  Ptr<LoraPhy> phy = m_phy.Create<LoraPhy> ();
  phy->SetChannel (m_channel);

  // Configuration is different based on the kind of device we have to create
  std::string typeId = m_phy.GetTypeId ().GetName ();
  if (typeId == "ns3::GatewayLoraPhy")
    {
      // Inform the channel of the presence of this PHY
      m_channel->Add (phy);

      // For now, assume that the PHY will listen to the default EU channels
      // with this ReceivePath configuration:
      // 3 ReceivePaths on 868.1
      // 3 ReceivePaths on 868.3
      // 2 ReceivePaths on 868.5

      // We expect that MacHelper instances will overwrite this setting if the
      // device will operate in a different region
      std::vector<double> frequencies;
      frequencies.push_back (868.1);
      // frequencies.push_back (868.3);
      // frequencies.push_back (868.5);

      std::vector<double>::iterator it = frequencies.begin ();

      int receptionPaths = 1;
      int maxReceptionPaths = 1;
      while (receptionPaths < maxReceptionPaths)
        {
          if (it == frequencies.end ())
            it = frequencies.begin ();
          phy->GetObject<GatewayLoraPhy> ()->AddReceptionPath (*it);
          ++it;
          receptionPaths++;
        }

    }

  if (typeId == "ns3::EndDeviceLoraPhy")
    {
      // The line below can be commented to speed up uplink-only simulations.
      // This implies that the LoraChannel instance will only know about
      // Gateways, and it will not lose time delivering packets and interference
      // information to devices which will never listen.

      m_channel->Add (phy);
    }

  if (typeId == "ns3::JammerLoraPhy")
    {
      // The line below can be commented to speed up uplink-only simulations.
      // This implies that the LoraChannel instance will only know about
      // Gateways, and it will not lose time delivering packets and interference
      // information to devices which will never listen.

      // m_channel->Add (phy);
    }


  // Link the PHY to its net device
  phy->SetDevice (device);

  return phy;
}
}
