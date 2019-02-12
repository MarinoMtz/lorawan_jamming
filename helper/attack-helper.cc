/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Padova
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
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 */

#include "ns3/attack-helper.h"
#include "ns3/lora-net-device.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AttackHelper");

AttackHelper::AttackHelper ()
{
}

void
AttackHelper::SetRandomDataRate (NodeContainer Jammers)
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NodeContainer::Iterator j = Jammers.Begin (); j != Jammers.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<JammerLoraMac> mac = loraNetDevice->GetMac ()->GetObject<JammerLoraMac> ();
      NS_ASSERT (mac != 0);
      mac->SetRandomDataRate ();
    }
}

void
AttackHelper::SetRandomSF (NodeContainer Jammers)
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NodeContainer::Iterator j = Jammers.Begin (); j != Jammers.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<JammerLoraPhy> phy = loraNetDevice->GetPhy ()->GetObject<JammerLoraPhy> ();
      NS_ASSERT (phy != 0);
      phy->SetRandomSF ();
    }
}

void
AttackHelper::SetAllSF (NodeContainer Jammers)
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NodeContainer::Iterator j = Jammers.Begin (); j != Jammers.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<JammerLoraPhy> phy = loraNetDevice->GetPhy ()->GetObject<JammerLoraPhy> ();
      NS_ASSERT (phy != 0);
      phy->SetAllSF ();
    }

}

void
AttackHelper::SetType (NodeContainer Jammers, double type)
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NodeContainer::Iterator j = Jammers.Begin (); j != Jammers.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<JammerLoraMac> mac = loraNetDevice->GetMac ()->GetObject<JammerLoraMac> ();
      NS_ASSERT (mac != 0);
      mac->SetType (type);

      Ptr<JammerLoraPhy> phy = loraNetDevice->GetPhy ()->GetObject<JammerLoraPhy> ();
      NS_ASSERT (phy != 0);
      phy->SetType (type);
    }
}


void
AttackHelper::SetTxPower (NodeContainer Jammers, double txpower)
{
	NS_LOG_FUNCTION_NOARGS ();

	for (NodeContainer::Iterator j = Jammers.Begin (); j != Jammers.End (); ++j)
	    {
	      Ptr<Node> object = *j;
	      Ptr<NetDevice> netDevice = object->GetDevice (0);
	      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
	      NS_ASSERT (loraNetDevice != 0);
	      Ptr<JammerLoraMac> mac = loraNetDevice->GetMac ()->GetObject<JammerLoraMac> ();
	      NS_ASSERT (mac != 0);
	      mac->SetTxPower (txpower);

	      Ptr<JammerLoraPhy> phy = loraNetDevice->GetPhy ()->GetObject<JammerLoraPhy> ();
	      NS_ASSERT (phy != 0);
	      phy->SetTxPower (txpower);
	    }
}

void
AttackHelper::ConfigureForEuRegionJm (NodeContainer Jammers) const
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NodeContainer::Iterator j = Jammers.Begin (); j != Jammers.End (); ++j)
	  {

      	  Ptr<Node> object = *j;
      	  Ptr<NetDevice> netDevice = object->GetDevice (0);
      	  Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      	  NS_ASSERT (loraNetDevice != 0);
      	  Ptr<JammerLoraMac> mac = loraNetDevice->GetMac ()->GetObject<JammerLoraMac> ();

      	  mac->SetSfForDataRate (std::vector<uint8_t> {12,11,10,9,8,7,7});
      	  mac->SetBandwidthForDataRate (std::vector<double> {125000,125000,125000,125000,125000,125000,250000});
      	  mac->SetMaxAppPayloadForDataRate (std::vector<uint32_t> {59,59,59,123,230,230,230,230});

      	  /////////////////////////////////////////////////////
      	  // TxPower -> Transmission power in dBm conversion //
      	  /////////////////////////////////////////////////////
      	  mac->SetTxDbmForTxPower (std::vector<double> {16, 14, 12, 10, 8, 6, 4, 2});
      	  //  jmMac->SetTxDbmForTxPower (std::vector<double> {100, 100, 100, 100, 100, 100, 100, 100});

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
      	  mac->SetReplyDataRateMatrix (matrix);

      	  /////////////////////
      	  // Preamble length //
      	  /////////////////////
      	  mac->SetNPreambleSymbols (8);

	    }
}

void
AttackHelper::ConfigureBand (NodeContainer Jammers,
							 double firstFrequency,
							 double lastFrequency,
							 double dutyCycle,
							 double maxTxPowerDbm,
							 double frequency,
							 uint8_t minDataRate,
							 uint8_t maxDataRate)
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NodeContainer::Iterator j = Jammers.Begin (); j != Jammers.End (); ++j)
	  {
      	  Ptr<Node> object = *j;
      	  Ptr<NetDevice> netDevice = object->GetDevice (0);
      	  Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      	  NS_ASSERT (loraNetDevice != 0);
      	  Ptr<JammerLoraMac> mac = loraNetDevice->GetMac ()->GetObject<JammerLoraMac> ();

      	  LogicalLoraChannelHelper channelHelper;
      	  channelHelper.AddSubBand (firstFrequency, lastFrequency, dutyCycle, maxTxPowerDbm);
      	  Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (frequency, minDataRate, maxDataRate);
      	  channelHelper.AddChannel (lc1);
      	  mac->SetLogicalLoraChannelHelper (channelHelper);
	  }
}

void
AttackHelper::ConfigureBand (NodeContainer Jammers, double DutyCycle)
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NodeContainer::Iterator j = Jammers.Begin (); j != Jammers.End (); ++j)
	  {

      	  Ptr<Node> object = *j;
      	  Ptr<NetDevice> netDevice = object->GetDevice (0);
      	  Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      	  NS_ASSERT (loraNetDevice != 0);
      	  Ptr<JammerLoraMac> mac = loraNetDevice->GetMac ()->GetObject<JammerLoraMac> ();

      	  //////////////
      	  // SubBands //
      	  //////////////

      	  LogicalLoraChannelHelper channelHelper;
      	  channelHelper.AddSubBand (868, 868.6, DutyCycle, 14);
      	  channelHelper.AddSubBand (868.7, 869.2, DutyCycle, 14);
      	  channelHelper.AddSubBand (869.4, 869.65, DutyCycle, 27);

      	  //////////////////////
      	  // Default channels //
      	  //////////////////////
      	  Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (868.1, 0, 5);
      	  Ptr<LogicalLoraChannel> lc2 = CreateObject<LogicalLoraChannel> (868.3, 0, 5);
      	  Ptr<LogicalLoraChannel> lc3 = CreateObject<LogicalLoraChannel> (868.5, 0, 5);
      	  channelHelper.AddChannel (lc1);
      	  channelHelper.AddChannel (lc2);
      	  channelHelper.AddChannel (lc3);
      	  mac->SetLogicalLoraChannelHelper (channelHelper);

	  }
}

void
AttackHelper::SetFrequency(NodeContainer Jammers,double Frequency)
{
	for (NodeContainer::Iterator j = Jammers.Begin (); j != Jammers.End (); ++j)
	    {
	      Ptr<Node> object = *j;
	      Ptr<NetDevice> netDevice = object->GetDevice (0);
	      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
	      NS_ASSERT (loraNetDevice != 0);

	      Ptr<JammerLoraPhy> phy = loraNetDevice->GetPhy ()->GetObject<JammerLoraPhy> ();
	      NS_ASSERT (phy != 0);
	      phy->SetFrequency (Frequency);

	    }
}

void
AttackHelper::SetPacketSize (NodeContainer Jammers,uint16_t PacketSize)
{
	for (NodeContainer::Iterator j = Jammers.Begin (); j != Jammers.End (); ++j)
	    {
	      Ptr<Node> object = *j;
	      Ptr<NetDevice> netDevice = object->GetDevice (0);
	      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
	      NS_ASSERT (loraNetDevice != 0);

	      Ptr<JammerLoraPhy> phy = loraNetDevice->GetPhy ()->GetObject<JammerLoraPhy> ();
	      NS_ASSERT (phy != 0);
	      phy->SetPacketSize (PacketSize);

	    }
}
}

