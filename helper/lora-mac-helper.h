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

#ifndef LORA_MAC_HELPER_H
#define LORA_MAC_HELPER_H

#include "ns3/net-device.h"
#include "ns3/lora-channel.h"
#include "ns3/lora-phy.h"
#include "ns3/lora-mac.h"
#include "ns3/lora-device-address-generator.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/jammer-lora-mac.h"
#include "ns3/gateway-lora-mac.h"
#include "ns3/node-container.h"

namespace ns3 {

class LoraMacHelper
{
public:

  /**
   * Define the kind of device. Can be either GW (Gateway) or ED (End Device).
   */
  enum DeviceType
  {
    GW,
    ED,
	JM
  };

  /**
   * Define the operational region.
   */
  enum Regions
  {
    EU,
    US,
    China,
    EU433MHz,
    Australia,
    CN,
    AS923MHz,
    SouthKorea
  };

  /**
   * Create a mac helper without any parameter set. The user must set
   * them all to be able to call Install later.
   */
  LoraMacHelper ();

  /**
   * Set an attribute of the underlying MAC object.
   *
   * \param name the name of the attribute to set.
   * \param v the value of the attribute.
   */
  void Set (std::string name, const AttributeValue &v);

  /**
   * Set the address generator to use for creation of these nodes.
   */
  void SetAddressGenerator (Ptr<LoraDeviceAddressGenerator> addrGen);

  /**
   * Set the kind of MAC this helper will create.
   *
   * \param dt the device type (either gateway or end device).
   */
  void SetDeviceType (enum DeviceType dt);

  /**
   * Set the region in which the device is to operate.
   */
  void SetRegion (enum Regions region);

  void SetTxPower (double txpower);

  /**
   * Create the LoRaMac instance and connect it to a device
   *
   * \param node the node on which we wish to create a wifi MAC.
   * \param device the device within which this MAC will be created.
   * \returns a newly-created LoraMac object.
   */
  Ptr<LoraMac> Create (Ptr<Node> node, Ptr<NetDevice> device) const;

  /**
   * Set up the end device's data rates
   * This function assumes we are using the following convention:
   * SF7 -> DR5
   * SF8 -> DR4
   * SF9 -> DR3
   * SF10 -> DR2
   * SF11 -> DR1
   * SF12 -> DR0
   */
  static void SetSpreadingFactorsUp (NodeContainer endDevices,
                                     NodeContainer gateways,
                                     Ptr<LoraChannel> channel);

  void SetSpreadingFactors (NodeContainer endDevices,
		  	  	  	  	  	uint8_t sf);

  void SetMType (NodeContainer endDevices,LoraMacHeader::MType mType);

  void SetACKParams (bool two_rx, double ackfrequency, int ack_sf, int acklength);

  // Set Retransmission params

  void SetRRX (bool retransmission, uint32_t rxnumber);


private:

  /**
   * Perform region-specific configurations for the 868 MHz EU band.
   */
  void ConfigureForEuRegion (Ptr<EndDeviceLoraMac> edMac) const;
  /**
   * Perform region-specific configurations for the 868 MHz EU band.
   */
  void ConfigureForEuRegion (Ptr<GatewayLoraMac> gwMac) const;

   /**
   * Apply configurations that are common both for the GatewayLoraMac and the
   * EndDeviceLoraMac classes.
   */

  void ApplyCommonEuConfigurations (Ptr<LoraMac> loraMac) const;

  ObjectFactory m_mac;
  Ptr<LoraDeviceAddressGenerator> m_addrGen; //!< Pointer to the address generator to use
  enum DeviceType m_deviceType; //!< The kind of device to install
  enum Regions m_region; //!< The region in which the device will operate

  //ACK Parameters

  bool m_two_rx;
  double m_ReceiveWindowFrequency;
  int m_ack_sf;
  int m_acklength;


  // Retransmission params
  bool m_retransmission;
  uint32_t m_rxnumber;



};

} //namespace ns3

#endif /* LORA_PHY_HELPER_H */
