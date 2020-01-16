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

#ifndef ATTACK_HELPER_H
#define ATTACK_HELPER_H

#include "ns3/net-device.h"
#include "ns3/lora-channel.h"
#include "ns3/lora-phy.h"
#include "ns3/lora-mac.h"
#include "ns3/lora-device-address-generator.h"
#include "ns3/jammer-lora-mac.h"
#include "ns3/jammer-lora-phy.h"
#include "ns3/gateway-lora-mac.h"
#include "ns3/node-container.h"
namespace ns3 {

class AttackHelper
{

public:

	AttackHelper ();

	static void SetTxPower (NodeContainer Jammers,
							double txpower);

	void ConfigureForEuRegionJm (NodeContainer Jammers) const;

	void ConfigureBand (NodeContainer Jammers,
						double firstFrequency,
						double lastFrequency,
						double dutyCycle,
						double maxTxPowerDbm,
						double frequency,
						uint8_t minDataRate,
						uint8_t maxDataRate);

	void ConfigureBand (NodeContainer Jammers, double DutyCicle);

	void SetType (NodeContainer Jammers, double type);

	void SetFrequency(NodeContainer Jammers,double Frequency);

	void SetPacketSize (NodeContainer Jammers,uint16_t PacketSize);


};

} //namespace ns3

#endif /* ATTACK_HELPER_H */
