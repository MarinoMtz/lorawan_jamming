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

#include "ns3/lora-mac-helper.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/lora-net-device.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoraMacHelper");

LoraMacHelper::LoraMacHelper () :
  m_region (LoraMacHelper::EU)
{
}

void
LoraMacHelper::Set (std::string name, const AttributeValue &v)
{
  m_mac.Set (name, v);
}

void
LoraMacHelper::SetDeviceType (enum DeviceType dt)
{
  NS_LOG_FUNCTION (this << dt);
  switch (dt)
    {
    case GW:
      m_mac.SetTypeId ("ns3::GatewayLoraMac");
      break;
    case ED:
      m_mac.SetTypeId ("ns3::EndDeviceLoraMac");
      break;
    case JM:
      m_mac.SetTypeId ("ns3::JammerLoraMac");
      break;
    }
  m_deviceType = dt;
}

void
LoraMacHelper::SetAddressGenerator (Ptr<LoraDeviceAddressGenerator> addrGen)
{
  NS_LOG_FUNCTION (this);

  m_addrGen = addrGen;
}

void
LoraMacHelper::SetRegion (enum LoraMacHelper::Regions region)
{
  m_region = region;
}

Ptr<LoraMac>
LoraMacHelper::Create (Ptr<Node> node, Ptr<NetDevice> device) const
{
  Ptr<LoraMac> mac = m_mac.Create<LoraMac> ();
  mac->SetDevice (device);

  // If we are operating on an end device, add an address to it
  if (m_deviceType == ED && m_addrGen != 0)
    {
      mac->GetObject<EndDeviceLoraMac> ()->SetDeviceAddress
        (m_addrGen->NextAddress ());
    }

  // Add a basic list of channels based on the region where the device is
  // operating
  if (m_deviceType == ED)
    {
      Ptr<EndDeviceLoraMac> edMac = mac->GetObject<EndDeviceLoraMac> ();
      switch (m_region)
        {
        case LoraMacHelper::EU:
          {
            ConfigureForEuRegion (edMac);
            break;
          }
        default:
          {
            NS_LOG_ERROR ("This region isn't supported yet!");
            break;
          }
        }
    }

  if (m_deviceType == GW)
    {
      Ptr<GatewayLoraMac> gwMac = mac->GetObject<GatewayLoraMac> ();
      switch (m_region)
        {
        case LoraMacHelper::EU:
          {
            ConfigureForEuRegion (gwMac);
            break;
          }
        default:
          {
            NS_LOG_ERROR ("This region isn't supported yet!");
            break;
          }
        }
    }

  return mac;
}

void
LoraMacHelper::ConfigureForEuRegion (Ptr<EndDeviceLoraMac> edMac) const
{
	//NS_LOG_FUNCTION_NOARGS ();

	ApplyCommonEuConfigurations (edMac);

	/////////////////////////////////////////////////////
	// TxPower -> Transmission power in dBm conversion //
	/////////////////////////////////////////////////////
	edMac->SetTxDbmForTxPower (std::vector<double> {16, 14, 12, 10, 8, 6, 4, 2});

	////////////////////////////////////////////////////////////
	// Matrix to know which DataRate the GW will respond with //
	////////////////////////////////////////////////////////////
	LoraMac::ReplyDataRateMatrix matrix = {{{{0,0,0,0,0,0}},
                                          {{1,0,0,0,0,0}},
                                          {{2,1,0,0,0,0}},
                                          {{3,2,1,0,0,0}},
                                          {{4,3,2,1,0,0}},
                                          {{5,4,3,2,1,0}},
                                          {{6,5,4,3,2,1}},
                                          {{7,6,5,4,3,2}}}};
	edMac->SetReplyDataRateMatrix (matrix);

	/////////////////////
	// Preamble length //
	/////////////////////
	edMac->SetNPreambleSymbols (8);

	////////////////////
	// ACK Parameters //
	////////////////////

	edMac-> SetACKParams (m_two_rx, m_ReceiveWindowFrequency, m_ack_sf, m_acklength);

	////////////////////////////////
	// Retransmissions Parameters //
	////////////////////////////////

	edMac-> SetRRX (m_retransmission, m_rxnumber);

}


void
LoraMacHelper::ConfigureForEuRegion (Ptr<GatewayLoraMac> gwMac) const
{
  //NS_LOG_FUNCTION_NOARGS ();

  ///////////////////////////////
  // ReceivePath configuration //
  ///////////////////////////////
  Ptr<GatewayLoraPhy> gwPhy = gwMac->GetDevice ()->
    GetObject<LoraNetDevice> ()->GetPhy ()->GetObject<GatewayLoraPhy> ();

  ApplyCommonEuConfigurations (gwMac);

  if (gwPhy) // If cast is successful, there's a GatewayLoraPhy
    {
      NS_LOG_DEBUG ("Resetting reception paths");
      gwPhy->ResetReceptionPaths ();

      std::vector<double> frequencies_downlink;
      frequencies_downlink.push_back (868.1);
      frequencies_downlink.push_back (868.2);
      frequencies_downlink.push_back (868.3);

      std::vector<double>::iterator it = frequencies_downlink.begin ();

      int receptionPaths = 0;
      int maxReceptionPaths = 8;
      while (receptionPaths < maxReceptionPaths)
        {
          if (it == frequencies_downlink.end ())
            it = frequencies_downlink.begin ();
          gwPhy->GetObject<GatewayLoraPhy> ()->AddReceptionPath (*it);
          ++it;
          receptionPaths++;
        }


      std::vector<double> frequencies_uplink;
      frequencies_uplink.push_back (869.525);

      std::vector<double>::iterator itt = frequencies_uplink.begin ();

      int upreceptionPaths = 0;
      int maxupReceptionPaths = 1;
      while (upreceptionPaths < maxupReceptionPaths)
       {
          if (itt == frequencies_uplink.end ())
            itt = frequencies_uplink.begin ();
          gwPhy->GetObject<GatewayLoraPhy> ()->AddReceptionPath (*itt);
          ++itt;
          upreceptionPaths++;
        }
    }
}

void
LoraMacHelper::ApplyCommonEuConfigurations (Ptr<LoraMac> loraMac) const
{
  //NS_LOG_FUNCTION_NOARGS ();

  //////////////
  // SubBands //
  //////////////

  LogicalLoraChannelHelper channelHelper;
  channelHelper.AddSubBand (868, 868.6, 1, 14);
  // channelHelper.AddSubBand (868.7, 869.2, 0.001, 14);
  channelHelper.AddSubBand (869.4, 869.65, 1, 27);

  //////////////////////
  // Default channels //
  //////////////////////
  Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (868.1, 0, 5);
  channelHelper.AddChannel (lc1);

  if (m_multi_channel){
	  Ptr<LogicalLoraChannel> lc2 = CreateObject<LogicalLoraChannel> (868.2, 0, 5);
	  Ptr<LogicalLoraChannel> lc3 = CreateObject<LogicalLoraChannel> (868.3, 0, 5);
	  channelHelper.AddChannel (lc2);
	  channelHelper.AddChannel (lc3);
  }


  //////////////////////
  // Downlink channels  //
  //////////////////////

  Ptr<LogicalLoraChannel> lc4 = CreateObject<LogicalLoraChannel> (869.525, 0, 5);
  lc4->DisableForUplink();
  channelHelper.AddChannel (lc4);

  loraMac->SetLogicalLoraChannelHelper (channelHelper);

  ///////////////////////////////////////////////
  // DataRate -> SF, DataRate -> Bandwidth     //
  // and DataRate -> MaxAppPayload conversions //
  ///////////////////////////////////////////////

  loraMac->SetSfForDataRate (std::vector<uint8_t> {12,11,10,9,8,7,7});
  loraMac->SetDataRateForSF (std::vector<uint8_t> {0,1,2,3,4,5,5});

  loraMac->SetBandwidthForDataRate (std::vector<double>
                                    {125000,125000,125000,125000,125000,125000,250000});
  loraMac->SetMaxAppPayloadForDataRate (std::vector<uint32_t>
                                        {59,59,59,123,230,230,230,230});

}

void
LoraMacHelper::SetSpreadingFactorsUp (NodeContainer endDevices, NodeContainer gateways, Ptr<LoraChannel> channel)
{
  //NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("MAC Helper --> Set Spreading Factors Up");

  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      NS_ASSERT (mac != 0);

      // Try computing the distance from each gateway and find the best one
      Ptr<Node> bestGateway = gateways.Get (0);
      Ptr<MobilityModel> bestGatewayPosition = bestGateway->GetObject<MobilityModel> ();

      // Assume devices transmit at 14 dBm
      double highestRxPower = channel->GetRxPower (14, position, bestGatewayPosition);

      for (NodeContainer::Iterator currentGw = gateways.Begin () + 1;
           currentGw != gateways.End (); ++currentGw)
        {
          // Compute the power received from the current gateway
          Ptr<Node> curr = *currentGw;
          Ptr<MobilityModel> currPosition = curr->GetObject<MobilityModel> ();
          double currentRxPower = channel->GetRxPower (14, position, currPosition);    // dBm
          NS_LOG_DEBUG ("Rx Power [dBm] = " << currentRxPower);

          if (currentRxPower > highestRxPower)
            {
              bestGateway = curr;
              bestGatewayPosition = curr->GetObject<MobilityModel> ();
              highestRxPower = currentRxPower;
            }
        }

      // NS_LOG_DEBUG ("Rx Power: " << highestRxPower);
      double rxPower = highestRxPower;

      NS_LOG_DEBUG ("Rx Power [dBm] = " << rxPower);

      // Get the Gw sensitivity
      Ptr<NetDevice> gatewayNetDevice = bestGateway->GetDevice (0);
      Ptr<LoraNetDevice> gatewayLoraNetDevice = gatewayNetDevice->GetObject<LoraNetDevice> ();
      Ptr<GatewayLoraPhy> gatewayPhy = gatewayLoraNetDevice->GetPhy ()->GetObject<GatewayLoraPhy> ();
      const double *gwSensitivity = gatewayPhy->sensitivity;

      if(rxPower > *gwSensitivity)
        {
          mac->SetDataRate (5);
          NS_LOG_DEBUG ("MAC Helper --> Setting Data Rate = " << 5);
        }
      else if (rxPower > *(gwSensitivity+1))
        {
          mac->SetDataRate (4);
          NS_LOG_DEBUG ("MAC Helper --> Setting Data Rate = " << 4);
        }
      else if (rxPower > *(gwSensitivity+2))
        {
          mac->SetDataRate (3);
          NS_LOG_DEBUG ("MAC Helper --> Setting Data Rate = " << 3);
        }
      else if (rxPower > *(gwSensitivity+3))
        {
          mac->SetDataRate (2);
          NS_LOG_DEBUG ("MAC Helper --> Setting Data Rate = " << 2);
        }
      else if (rxPower > *(gwSensitivity+4))
        {
          mac->SetDataRate (1);
          NS_LOG_DEBUG ("MAC Helper --> Setting Data Rate = " << 1);
        }
      else if (rxPower > *(gwSensitivity+5))
        {
          mac->SetDataRate (0);
          NS_LOG_DEBUG ("MAC Helper --> Setting Data Rate = " << 0);
        }
      else // Device is out of range. Assign SF12.
        {
          mac->SetDataRate (0);
          NS_LOG_DEBUG ("MAC Helper --> Setting Data Rate = " << 0);
        }
    }
}


void
LoraMacHelper::SetSpreadingFactors (NodeContainer endDevices, uint8_t sf)
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      NS_ASSERT (mac != 0);

      switch (sf) {
          case 7: mac->SetDataRate (5);
          	  	  break;
          case 8: mac->SetDataRate (4);
          	  	  break;
          case 9: mac->SetDataRate (3);
          	  	  break;
          case 10: mac->SetDataRate (2);
          	  	  break;
          case 11:  mac->SetDataRate (1);
          	  	  break;
          case 12: mac->SetDataRate (0);
          	  	  break;
          default: mac->SetDataRate (5);

              break;
      }
    }
}


void
LoraMacHelper::SetMType (NodeContainer endDevices, LoraMacHeader::MType mType)
{

	  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
	    {
	      Ptr<Node> object = *j;
	      Ptr<NetDevice> netDevice = object->GetDevice (0);
	      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
	      NS_ASSERT (loraNetDevice != 0);
	      Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
	      NS_ASSERT (mac != 0);
	      mac->SetMType (mType);
	    }
}

void
LoraMacHelper::SetACKParams (bool two_rx, double ackfrequency, int ack_sf, int acklength)
{
	m_two_rx = two_rx;
	m_ReceiveWindowFrequency = ackfrequency;
	m_ack_sf = ack_sf;
	m_acklength = acklength;
}

void
LoraMacHelper::SetRRX (bool retransmission, uint32_t rxnumber)
{
	m_retransmission = retransmission;
	m_rxnumber = rxnumber;
	//NS_LOG_FUNCTION ("maxtx "  << rxnumber );

}

void
LoraMacHelper::SetMulti (bool multi_channel)
{
	m_multi_channel = multi_channel;
	//NS_LOG_FUNCTION ("maxtx "  << rxnumber );

}

}
