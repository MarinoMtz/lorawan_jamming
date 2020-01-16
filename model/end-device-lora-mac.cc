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

#include "ns3/end-device-lora-mac.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <iterator>

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EndDeviceLoraMac");

NS_OBJECT_ENSURE_REGISTERED (EndDeviceLoraMac);

TypeId
EndDeviceLoraMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EndDeviceLoraMac")
    .SetParent<LoraMac> ()
    .SetGroupName ("lorawan")
    .AddTraceSource ("DataRate",
                     "Data Rate currently employed by this end device",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_dataRate),
                     "ns3::TracedValueCallback::uint8_t")
    .AddTraceSource ("TxPower",
                     "Transmission power currently employed by this end device",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_txPower),
                     "ns3::TracedValueCallback::Double")
    .AddTraceSource ("LastKnownLinkMargin",
                     "Last known demodulation margin in "
                     "communications between this end device "
                     "and a gateway",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_lastKnownLinkMargin),
                     "ns3::TracedValueCallback::Double")
    .AddTraceSource ("LastKnownGatewayCount",
                     "Last known number of gateways able to "
                     "listen to this end device",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_lastKnownGatewayCount),
                     "ns3::TracedValueCallback::Int")
	.AddTraceSource ("ResendPacket",
					 "Trace source indicating a packet re-transmission",
					MakeTraceSourceAccessor
						(&EndDeviceLoraMac::m_resendpacket))
    .AddTraceSource ("AggregatedDutyCycle",
                     "Aggregate duty cycle, in fraction form, "
                     "this end device must respect",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_aggregatedDutyCycle),
                     "ns3::TracedValueCallback::Double")
    .AddConstructor<EndDeviceLoraMac> ();
  return tid;
}

EndDeviceLoraMac::EndDeviceLoraMac () :
  m_dataRate (0),
  m_txPower (14),
  m_codingRate (1),                         	// LoraWAN default
  m_headerDisabled (0),                     	// LoraWAN default
  m_receiveDelay1 (Seconds (1)),            	// LoraWAN default
  m_receiveDelay2 (Seconds (2)),            	// LoraWAN default
  m_ackdifferentchannel(false),					// Bool to set if ACKs are sent in a diffent channel
  m_sendsecondreceivewindow(false),					// Bool to set if the second receive window is used
  m_FirstReceiveWindowDuration (Seconds (0.2)), // This will be set as the time necessary to detect a preamble at the corresponding sf
  m_SecondReceiveWindowDuration (Seconds (0.2)),// This will be set as the time necessary to detect a preamble at the corresponding sf
  m_firstReceiveWindowDataRate (0), 			// First rx window DR if it is sent on the same channel as the frame
  m_secondReceiveWindowDataRate (0),			// Second rx window DR
  m_acklength(1),								// ACK length
  m_closeWindow (EventId ()),               	// Initialize as the default eventId
  m_secondReceiveWindow (EventId ()),       	// Initialize as the default eventId
  m_firstReceiveWindowFrequency (868.1),		// Frequency of the second receive window
  m_secondReceiveWindowFrequency (868.1),		// Frequency of the fisrt receive window
  m_rx1DrOffset (0),                         	// LoraWAN default
  m_lastKnownLinkMargin (0),
  m_lastKnownGatewayCount (0),
  m_aggregatedDutyCycle (1),
  m_mType (LoraMacHeader::UNCONFIRMED_DATA_DOWN),
  m_sf (7),
  m_retransmission(false),
  m_rxnumber(1)
{
  NS_LOG_FUNCTION (this);

  // Initialize the random variable we'll use to decide which channel to
  // transmit on.
  m_uniformRV = CreateObject<UniformRandomVariable> ();
  m_exprandomdelay = CreateObject<ExponentialRandomVariable> ();

  // Void the two receiveWindow events
  m_closeWindow = EventId ();
  m_closeWindow.Cancel ();
  m_secondReceiveWindow = EventId ();
  m_secondReceiveWindow.Cancel ();
}

EndDeviceLoraMac::~EndDeviceLoraMac ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
EndDeviceLoraMac::Send (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  bool sent;

  if (m_phy->GetObject<EndDeviceLoraPhy> ()->IsDead ()) {
	  NS_LOG_INFO ("Cannot send because end-device is DEAD");
	  //return;

	  sent = false;
	  return sent;
  }

  // Check that there are no scheduled receive windows.
  // We cannot send a packet if we are in the process of transmitting or waiting
  // for reception.

  if (!m_closeWindow.IsExpired () || !m_secondReceiveWindow.IsExpired ())
    {
      NS_LOG_WARN ("Attempting to send when there are receive windows" <<
                   " Transmission canceled");
      //return;

	  sent = false;
	  return sent;
    }

  // Check that payload length is below the allowed maximum
  if (packet->GetSize () > m_maxAppPayloadForDataRate.at (m_dataRate))
    {
      NS_LOG_WARN ("Attempting to send a packet larger than the maximum allowed"
                   << " size at this DataRate (DR" << unsigned(m_dataRate) <<
                   "). Transmission canceled.");
      //return;
	  sent = false;
	  return sent;

    }

  // Check that we can transmit according to the aggregate duty cycle timer
  //if (m_channelHelper.GetAggregatedWaitingTime () != Seconds (0))
    //{
      //NS_LOG_WARN ("Attempting to send, but the aggregate duty cycle won't allow it");
      //return;
	  //sent = false;
	  //return sent;
    //}

  // Pick a channel on which to transmit the packet
  Ptr<LogicalLoraChannel> txChannel = GetChannelForTx ();

  if (txChannel) // Proceed with transmission
    {

      ///////////////////////////////////////
      // Get information about this packet //
      ///////////////////////////////////////
	  uint8_t ntx;

	  LoraTag tag;
	  packet->RemovePacketTag (tag);
	  uint32_t ID = tag.GetPktID();
	  ntx = tag.Getntx();
	  packet->AddPacketTag (tag);

      /////////////////////////////////////////////////////////
      // Add headers, prepare TX parameters and send the packet
      /////////////////////////////////////////////////////////

      // Add the Lora Frame Header to the packet
      LoraFrameHeader frameHdr;
      ApplyNecessaryOptions (frameHdr);
      packet->AddHeader (frameHdr);
      NS_LOG_INFO ("Added frame header of size " << frameHdr.GetSerializedSize () <<
                   " bytes");

      // Add the Lora Mac header to the packet
      LoraMacHeader macHdr;
      ApplyNecessaryOptions (macHdr);
      packet->AddHeader (macHdr);
      NS_LOG_INFO ("Added MAC header of size " << macHdr.GetSerializedSize () <<
                   " bytes");

      // Craft LoraTxParameters object
      LoraTxParameters params;
      params.sf = GetSfFromDataRate (m_dataRate);
      params.headerDisabled = m_headerDisabled;
      params.codingRate = m_codingRate;
      params.bandwidthHz = GetBandwidthFromDataRate (m_dataRate);
      params.nPreamble = m_nPreambleSymbols;
      params.crcEnabled = 1;
      params.lowDataRateOptimizationEnabled = 0;


      // Compute the duration of the receive windows as the time necessary to detect a preamble

      m_FirstReceiveWindowDuration = m_phy->GetReceiveWindowTime (params,1);
      m_SecondReceiveWindowDuration = m_phy->GetReceiveWindowTime (params,1);

      // Wake up PHY layer and directly send the packet

      // Make sure we can transmit at the current power on this channel
      //NS_ASSERT (m_txPower <= m_channelHelper.GetTxPowerForChannel (txChannel));
      m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToStandby ();
      m_phy->Send (packet, params, txChannel->GetFrequency (), m_txPower);

      //NS_LOG_INFO ("MAC ---> Sending on Freq: " << txChannel->GetFrequency () << " MHz");


      ////////////////////////////////////////////////
      // Register packet transmission for duty cycle//
      ////////////////////////////////////////////////

      // Compute packet duration
      Time duration = m_phy->GetOnAirTime (packet, params);

      // Register the sent packet into the DutyCycleHelper
      m_channelHelper.AddEvent (duration, txChannel);

      //////////////////////////////
      // Prepare for the downlink //
      //////////////////////////////

      // Switch the PHY to the channel so that it will listen here for downlink
      // m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency (txChannel->GetFrequency ());

      // Instruct the PHY on the right Spreading Factor to listen for during the window

      // uint8_t replyDataRate = GetFirstReceiveWindowDataRate ();

      //NS_LOG_DEBUG ("m_dataRate: " << unsigned (m_dataRate) <<
      //              ", m_rx1DrOffset: " << unsigned (m_rx1DrOffset) <<
      //              ", replyDataRate: " << unsigned (replyDataRate));

      //m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor
        //(GetSfFromDataRate (replyDataRate));

      //NS_LOG_INFO ("--->> replyDataRate " << unsigned (replyDataRate));

	  sent = true;
	  if (ntx > 1) {m_resendpacket (packet,m_device->GetNode ()->GetId (),
			  txChannel->GetFrequency (),params.sf);}

      /////////////////////////////////////
      // Update the transmissions vector //
      /////////////////////////////////////

	  NS_LOG_INFO ("---->> Sending Pkt ID " << ID << " Ntx " << "" << unsigned(ntx)
			  << " Node ID " << m_device->GetNode ()->GetId ());


	  PacketTrack (ID, ntx, 0);

	  return sent;
    }

  else // Transmission cannot be performed
    {
      m_cannotSendBecauseDutyCycle (packet);

      sent = false;
	  return sent;
    }
}

void
EndDeviceLoraMac::Receive (Ptr<Packet const> packet)
{
  NS_LOG_FUNCTION (this << packet);

  // Work on a copy of the packet
  Ptr<Packet> packetCopy = packet->Copy ();

  // Remove the Mac Header to get some information
  LoraMacHeader mHdr;
  packetCopy->RemoveHeader (mHdr);

  // Only keep analyzing the packet if it's downlink
  if (!mHdr.IsUplink ())
    {
      NS_LOG_INFO ("Found a downlink packet");

      // Remove the Frame Header
      LoraFrameHeader fHdr;
      fHdr.SetAsDownlink ();
      packetCopy->RemoveHeader (fHdr);

      // Determine whether this packet is for us
      bool messageForUs = (m_address == fHdr.GetAddress ());

      if (messageForUs)
        {
          NS_LOG_INFO ("The message is for us!");

          // If it exists, cancel the second receive window event
          Simulator::Cancel (m_secondReceiveWindow);

          // Parse the MAC commands
          ParseCommands (fHdr);
          // TODO Pass the packet up to the NetDevice
          // Call the trace source
          m_receivedPacket (packet);

    	  LoraTag tag;
    	  packetCopy->RemovePacketTag (tag);
    	  uint32_t ID = tag.GetPktID();
    	  packetCopy->AddPacketTag (tag);

    	  PacketTrack (ID, 0, 1);

		  NS_LOG_INFO ("---->> Packet ackited ID " << ID << " address " << m_address);
		  NS_LOG_INFO ("---->> Packet length " << unsigned (packet->GetSize()));
        }
      else
        {
          NS_LOG_DEBUG ("The message is intended for another recipient.");
        }
    }
}

void
EndDeviceLoraMac::ParseCommands (LoraFrameHeader frameHeader)
{
  NS_LOG_FUNCTION (this << frameHeader);

  std::list<Ptr<MacCommand> > commands = frameHeader.GetCommands ();
  std::list<Ptr<MacCommand> >::iterator it;
  for (it = commands.begin (); it != commands.end (); it++)
    {
      NS_LOG_DEBUG ("Iterating over the MAC commands");
      enum MacCommandType type = (*it)->GetCommandType ();
      switch (type)
        {
        case (LINK_CHECK_ANS):
          {
            NS_LOG_DEBUG ("Detected a LinkCheckAns command");

            // Cast the command
            Ptr<LinkCheckAns> linkCheckAns = (*it)->GetObject<LinkCheckAns> ();

            // Call the appropriate function to take action
            OnLinkCheckAns (linkCheckAns->GetMargin (), linkCheckAns->GetGwCnt ());

            break;
          }
        case (LINK_ADR_REQ):
          {
            NS_LOG_DEBUG ("Detected a LinkAdrReq command");

            // Cast the command
            Ptr<LinkAdrReq> linkAdrReq = (*it)->GetObject<LinkAdrReq> ();

            // Call the appropriate function to take action
            OnLinkAdrReq (linkAdrReq->GetDataRate (), linkAdrReq->GetTxPower (),
                          linkAdrReq->GetEnabledChannelsList (),
                          linkAdrReq->GetRepetitions ());

            break;
          }
        case (DUTY_CYCLE_REQ):
          {
            NS_LOG_DEBUG ("Detected a DutyCycleReq command");

            // Cast the command
            Ptr<DutyCycleReq> dutyCycleReq = (*it)->GetObject<DutyCycleReq> ();

            // Call the appropriate function to take action
            OnDutyCycleReq (dutyCycleReq->GetMaximumAllowedDutyCycle ());

            break;
          }
        case (RX_PARAM_SETUP_REQ):
          {
            NS_LOG_DEBUG ("Detected a RxParamSetupReq command");

            // Cast the command
            Ptr<RxParamSetupReq> rxParamSetupReq = (*it)->GetObject<RxParamSetupReq> ();

            // Call the appropriate function to take action
            OnRxParamSetupReq (rxParamSetupReq->GetRx1DrOffset (),
                               rxParamSetupReq->GetRx2DataRate (),
                               rxParamSetupReq->GetFrequency ());

            break;
          }
        case (DEV_STATUS_REQ):
          {
            NS_LOG_DEBUG ("Detected a DevStatusReq command");

            // Cast the command
            Ptr<DevStatusReq> devStatusReq = (*it)->GetObject<DevStatusReq> ();

            // Call the appropriate function to take action
            OnDevStatusReq ();

            break;
          }
        case (NEW_CHANNEL_REQ):
          {
            NS_LOG_DEBUG ("Detected a NewChannelReq command");

            // Cast the command
            Ptr<NewChannelReq> newChannelReq = (*it)->GetObject<NewChannelReq> ();

            // Call the appropriate function to take action
            OnNewChannelReq (newChannelReq->GetChannelIndex (), newChannelReq->GetFrequency (), newChannelReq->GetMinDataRate (), newChannelReq->GetMaxDataRate ());

            break;
          }
        case (RX_TIMING_SETUP_REQ):
          {
            break;
          }
        case (TX_PARAM_SETUP_REQ):
          {
            break;
          }
        case (DL_CHANNEL_REQ):
          {
            break;
          }
        default:
          {
            NS_LOG_ERROR ("CID not recognized");
            break;
          }
        }
    }

}

void
EndDeviceLoraMac::ApplyNecessaryOptions (LoraFrameHeader& frameHeader)
{
  NS_LOG_FUNCTION_NOARGS ();

  frameHeader.SetAsUplink ();
  frameHeader.SetFPort (1); // TODO Use an appropriate frame port based on the application
  frameHeader.SetAddress (m_address);
  frameHeader.SetAdr (0); // TODO Set ADR if a member variable is true
  frameHeader.SetAdrAckReq (0); // TODO Set ADRACKREQ if a member variable is true
  frameHeader.SetAck (0); // TODO Set ACK if a member variable is true
  // FPending does not exist in uplink messages
  frameHeader.SetFCnt (0); // TODO Set this based on the counters

  // Add listed MAC commands
  for (const auto &command : m_macCommandList)
    {
      NS_LOG_INFO ("Applying a MAC Command of CID " <<
                   unsigned(MacCommand::GetCIDFromMacCommand
                              (command->GetCommandType ())));

      frameHeader.AddCommand (command);
    }

  m_macCommandList.clear ();
}

void
EndDeviceLoraMac::ApplyNecessaryOptions (LoraMacHeader& macHeader)
{
  NS_LOG_FUNCTION_NOARGS ();

  macHeader.SetMType (m_mType);
  macHeader.SetMajor (1);
}

void
EndDeviceLoraMac::SetMType (LoraMacHeader::MType mType)
{
  m_mType = mType;
}

void
EndDeviceLoraMac::TxFinished (Ptr<Packet const> packet)
{
  //NS_LOG_FUNCTION_NOARGS ();

  //Schedule the opening of the first receive window
  Simulator::Schedule (m_receiveDelay1,
                       &EndDeviceLoraMac::OpenFirstReceiveWindow, this);

  // Schedule the opening of the second receive window

  NS_LOG_INFO ("----> OPEN SECOND " << m_sendsecondreceivewindow);

  if (m_sendsecondreceivewindow)
  {
	  m_secondReceiveWindow = Simulator::Schedule (m_receiveDelay2,
	                                               &EndDeviceLoraMac::OpenSecondReceiveWindow,
	                                               this);
  }

  //Schedule the Check and resend function

  // Craft LoraTxParameters object
  LoraTxParameters paramsack;
  paramsack.sf = GetSfFromDataRate (m_firstReceiveWindowDataRate);
  paramsack.headerDisabled = m_headerDisabled;
  paramsack.codingRate = m_codingRate;
  paramsack.bandwidthHz = GetBandwidthFromDataRate (m_firstReceiveWindowDataRate);
  paramsack.nPreamble = m_nPreambleSymbols;
  paramsack.crcEnabled = 1;
  paramsack.lowDataRateOptimizationEnabled = 0;

  //Compute the length of the ACK shoud have + 8 (preamble) + 1 (MAC Header)

  Ptr<Packet> replyPacket = Create<Packet> (m_acklength+9);

  // Compute the time on air of the ACK to trigger the Check ackited function

  Time ackduration = m_phy->GetOnAirTime (replyPacket, paramsack);

  // Get the Packet information

  Ptr<Packet> PacketCopy = packet->Copy ();

  LoraTag tag;
  PacketCopy -> RemovePacketTag (tag);
  uint32_t ID = tag.GetPktID();
  uint8_t ntx = tag.Getntx();
  double mean = tag.GetMean();
  uint32_t size = PacketCopy->GetSize();


  // Schedule the Check and Resend function if according to the variable

  if (m_retransmission)
  {
	  Simulator::Schedule (m_receiveDelay2 + ackduration , &EndDeviceLoraMac::CheckAndResend, this, ID, ntx, size, mean);
  }


  // Switch the PHY to sleep
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToSleep ();
}

void
EndDeviceLoraMac::CheckAndResend (uint32_t ID, int ntx, uint32_t size, double mean)
{

	NS_LOG_INFO (" Inside the function to check and resend ");
	NS_LOG_INFO ("---->> Packet to be checked info " << ID <<" Nb transmissions "<< unsigned (ntx) << " size " << size);

	NS_LOG_INFO ("---->> Packet Ackited ? " << pktackited[ID-1]);


	bool pktack = PacketTrack (ID, 0, 2);

	//Scheduling next transmission

    m_app -> ConfirmedTraffic (ntx, ID, pktack);

}

bool
EndDeviceLoraMac::PacketTrack (uint32_t ID, int ntx, uint8_t type)
{

bool ackited = false;

switch (type)
{
	case 0:
		if (ntx == 1){AddPacketID(ID);}
		else {AddRetransmission(ID, ntx);}
	// Call the functionto add the packet to the pkttransmissions vector
		break;
	case 1:
		AddAckPacket (ID);
		break;
	case 2:
		ackited = CheckAckPacket (ID);
		break;
	default : break;
}

	//vector<uint32_t> pktackited;
	//vector<uint8_t> pkttransmissions;

	return ackited;
}

void
EndDeviceLoraMac::AddPacketID (uint32_t ID)
{

	std::vector<uint32_t>::iterator it = std::find(pkstsentids.begin(), pkstsentids.end(), ID);

	if (it != pkstsentids.end()){
		NS_LOG_INFO ("ID already inserted");
		return;
	}
	else{
		NS_LOG_INFO ("Adding Packet ID ");
		pkstsentids.push_back(ID);
		pktackited.push_back(0);
		pkttransmissions.push_back(1);

	}
	return;
}

void
EndDeviceLoraMac::AddAckPacket (uint32_t ID)
{
	std::vector<uint32_t>::iterator it = std::find(pkstsentids.begin(), pkstsentids.end(), ID);

	if (it != pkstsentids.end()){

		uint32_t index = std::distance(pkstsentids.begin(), it);
		pktackited [index] = 1;
		NS_LOG_INFO ("Adding ACK track");

	}
	return;
}

bool
EndDeviceLoraMac::CheckAckPacket (uint32_t ID)
{
	std::vector<uint32_t>::iterator it = std::find(pkstsentids.begin(), pkstsentids.end(), ID);

	if (it != pkstsentids.end()){

		uint32_t index = std::distance(pkstsentids.begin(), it);
		if (pktackited [index] == 1){
			return true;}
		else {return false;}
	}
	return false;
}

void
EndDeviceLoraMac::AddRetransmission (uint32_t ID, int ntx)
{
	std::vector<uint32_t>::iterator it = std::find(pkstsentids.begin(), pkstsentids.end(), ID);

	if (it != pkstsentids.end()){

		uint32_t index = std::distance(pkstsentids.begin(), it);
		pkttransmissions [index] = ntx;
		NS_LOG_INFO ("Adding re-transmission track");

	}
	return;
}


void
EndDeviceLoraMac::OpenFirstReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_phy->GetObject<EndDeviceLoraPhy> ()->GetState () == EndDeviceLoraPhy::DEAD
	  || m_phy->GetObject<EndDeviceLoraPhy> ()->GetState () == EndDeviceLoraPhy::TX
	  || m_phy->GetObject<EndDeviceLoraPhy> ()->GetState () == EndDeviceLoraPhy::RX)
    {
      NS_LOG_INFO ("Won't open first receive window because the end-device is DEAD");
      return;
    }

  // Switch the PHY to the channel so that it will listen here for downlink
  m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency (m_firstReceiveWindowFrequency);

  uint8_t replyDataRate = GetFirstReceiveWindowDataRate ();

  // Set the SF to listen the ACK
  m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor (GetSfFromDataRate (replyDataRate));


  // Set Phy in Standby mode

  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToStandby ();

  // Schedule return to sleep after "at least the time required by the end
  // device's radio transceiver to effectively detect a downlink preamble"
  // (LoraWAN specification)


  NS_LOG_INFO ("END - DEVICE Using parameters 1st rx: " << m_firstReceiveWindowFrequency << "Hz, DR"
                                    << unsigned(replyDataRate));

  m_closeWindow = Simulator::Schedule (m_FirstReceiveWindowDuration,
                                       &EndDeviceLoraMac::CloseFirstReceiveWindow, this);
}

void
EndDeviceLoraMac::CloseFirstReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<EndDeviceLoraPhy> phy = m_phy->GetObject<EndDeviceLoraPhy> ();

  // Check the Phy layer's state:
  // - RX -> We have received a preamble.
  // - STANDBY -> Nothing was detected.
  // We should never be in TX or SLEEP mode at this point
  switch (phy->GetState ())
    {
    case EndDeviceLoraPhy::TX:
    case EndDeviceLoraPhy::SLEEP:
    case EndDeviceLoraPhy::DEAD:
      //NS_ABORT_MSG ("PHY was in TX or SLEEP mode or DEAD when attempting to "
    		  //<< "close a receive window");
      break;
    case EndDeviceLoraPhy::RX:
      // PHY is receiving: let it finish. The Receive method will switch it back to SLEEP.
      break;
    case EndDeviceLoraPhy::STANDBY:
      // Turn PHY layer to SLEEP
      phy->SwitchToSleep ();
      break;
    }
}

void
EndDeviceLoraMac::OpenSecondReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();


  // Check for receiver status: if it's discharged, don't open this
  // window at all.

  NS_LOG_INFO ("Open Second window at "<< Simulator::Now ().GetSeconds ());

  if (m_phy->GetObject<EndDeviceLoraPhy> ()->GetState () == EndDeviceLoraPhy::DEAD
	  || m_phy->GetObject<EndDeviceLoraPhy> ()->GetState () == EndDeviceLoraPhy::TX
	  || m_phy->GetObject<EndDeviceLoraPhy> ()->GetState () == EndDeviceLoraPhy::RX)
    {
      NS_LOG_INFO ("Won't open second receive window because the end-device is DEAD");
      return;
    }

  // Set Phy in Standby mode
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToStandby ();

  uint8_t replyDataRate = GetSecondReceiveWindowDataRate ();

  // Switch to appropriate channel and data rate
  NS_LOG_INFO ("END - DEVICE Using parameters 2nd rx: " << m_secondReceiveWindowFrequency << "Hz, DR"
                                    << unsigned(replyDataRate));

  m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency
    (m_secondReceiveWindowFrequency);
  m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor
	(GetSfFromDataRate(replyDataRate));

  // Schedule return to sleep after "at least the time required by the end
  // device's radio transceiver to effectively detect a downlink preamble"
  // (LoraWAN specification)
  m_secondReceiveWindow = Simulator::Schedule (m_SecondReceiveWindowDuration,
                                       &EndDeviceLoraMac::CloseSecondReceiveWindow, this);
}

void
EndDeviceLoraMac::CloseSecondReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_LOG_INFO ("Closing Second RX Window at "<<Simulator::Now ().GetSeconds ());

  Ptr<EndDeviceLoraPhy> phy = m_phy->GetObject<EndDeviceLoraPhy> ();

  // NS_ASSERT (phy->m_state != EndDeviceLoraPhy::TX &&
  // phy->m_state != EndDeviceLoraPhy::SLEEP);

  // Check the Phy layer's state:
  // - RX -> We have received a preamble.
  // - STANDBY -> Nothing was detected.
  switch (phy->GetState ())
    {
    case EndDeviceLoraPhy::TX:
    case EndDeviceLoraPhy::SLEEP:
    case EndDeviceLoraPhy::DEAD:
      //NS_ABORT_MSG ("PHY was in TX or SLEEP mode or DEAD when attempting to "
      //	  << "close a receive window");
      break;
    case EndDeviceLoraPhy::RX:
      // PHY is receiving: let it finish. The Receive method will switch it back to SLEEP.
      break;
    case EndDeviceLoraPhy::STANDBY:
      // Turn PHY layer to SLEEP
      phy->SwitchToSleep ();
      break;
    }

}

Ptr<LogicalLoraChannel>
EndDeviceLoraMac::GetChannelForTx (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Pick a random channel to transmit on
  std::vector<Ptr<LogicalLoraChannel> > logicalChannels;
  logicalChannels = m_channelHelper.GetChannelList (); // Use a separate list to do the shuffle
  logicalChannels = Shuffle (logicalChannels);

  // Try every channel
  std::vector<Ptr<LogicalLoraChannel> >::iterator it;
  for (it = logicalChannels.begin (); it != logicalChannels.end (); ++it)
    {
      // Pointer to the current channel
      Ptr<LogicalLoraChannel> logicalChannel = *it;
      double frequency = logicalChannel->GetFrequency ();

      //NS_LOG_DEBUG ("Frequency of the current channel: " << frequency);

      // Verify that we can send the packet
      Time waitingTime = m_channelHelper.GetWaitingTime (logicalChannel);

      //NS_LOG_DEBUG ("Waiting time for current channel = " <<
                    //waitingTime.GetSeconds ());

      if (logicalChannel->IsEnabledForUplink()){

      // Send immediately if we can
      //if (waitingTime == Seconds (0))
      //  {
          return *it;
      //  }
      //else
      //  {
      //    NS_LOG_DEBUG ("Packet cannot be immediately transmitted on " <<
      //                  "the current channel because of duty cycle limitations");
      //  }
      }
    }
  return 0; // In this case, no suitable channel was found
}

std::vector<Ptr<LogicalLoraChannel> >
EndDeviceLoraMac::Shuffle (std::vector<Ptr<LogicalLoraChannel> > vector)
{
  NS_LOG_FUNCTION_NOARGS ();

  int size = vector.size ();

  for (int i = 0; i < size; ++i)
    {
      uint16_t random = std::floor (m_uniformRV->GetValue (0, size));
      Ptr<LogicalLoraChannel> temp = vector.at (random);
      vector.at (random) = vector.at (i);
      vector.at (i) = temp;
    }

  return vector;
}

/////////////////////////
// Setters and Getters //
/////////////////////////

void
EndDeviceLoraMac::SetApp (Ptr<PeriodicSender> app)
{
	m_app = app;
}

void
EndDeviceLoraMac::SetDataRate (uint8_t dataRate)
{
  NS_LOG_FUNCTION (this << unsigned (dataRate));

  m_dataRate = dataRate;
  SetSpreadingFactor (GetSfFromDataRate (m_dataRate));

}

void
EndDeviceLoraMac::SetSpreadingFactor (uint8_t sf)
{
  NS_LOG_FUNCTION (this << unsigned (sf));

  m_sf = sf;

}

void
EndDeviceLoraMac::SetTxPower (double txPower)
{
  NS_LOG_FUNCTION (this << unsigned (txPower));

  m_txPower = txPower;
}

uint8_t
EndDeviceLoraMac::GetSpreadingFactor (void)
{
  NS_LOG_FUNCTION (this);

  return m_sf;
}

uint8_t
EndDeviceLoraMac::GetDataRate (void)
{
  NS_LOG_FUNCTION (this);

  return m_dataRate;
}

void
EndDeviceLoraMac::SetDeviceAddress (LoraDeviceAddress address)
{
  NS_LOG_FUNCTION (this << address);

  m_address = address;
}

LoraDeviceAddress
EndDeviceLoraMac::GetDeviceAddress (void)
{
  NS_LOG_FUNCTION (this);

  return m_address;
}

void
EndDeviceLoraMac::OnLinkCheckAns (uint8_t margin, uint8_t gwCnt)
{
  NS_LOG_FUNCTION (this << unsigned(margin) << unsigned(gwCnt));

  m_lastKnownLinkMargin = margin;
  m_lastKnownGatewayCount = gwCnt;
}

void
EndDeviceLoraMac::OnLinkAdrReq (uint8_t dataRate, uint8_t txPower,
                                std::list<int> enabledChannels, int repetitions)
{
  NS_LOG_FUNCTION (this << unsigned (dataRate) << unsigned (txPower) <<
                   repetitions);

  // Three bools for three requirements before setting things up
  bool channelMaskOk = true;
  bool dataRateOk = true;
  bool txPowerOk = true;

  // Check the channel mask
  /////////////////////////
  // Check whether all specified channels exist on this device
  auto channelList = m_channelHelper.GetChannelList ();
  int channelListSize = channelList.size ();

  for (auto it = enabledChannels.begin (); it != enabledChannels.end (); it++)
    {
      if ((*it) > channelListSize)
        {
          channelMaskOk = false;
          break;
        }
    }

  // Check the dataRate
  /////////////////////
  // We need to know we can use it at all
  // To assess this, we try and convert it to a SF/BW combination and check if
  // those values are valid. Since GetSfFromDataRate and
  // GetBandwidthFromDataRate return 0 if the dataRate is not recognized, we
  // can check against this.
  uint8_t sf = GetSfFromDataRate (dataRate);
  double bw = GetBandwidthFromDataRate (dataRate);
  NS_LOG_DEBUG ("SF: " << unsigned (sf) << ", BW: " << bw);
  if (sf == 0 || bw == 0)
    {
      dataRateOk = false;
    }

  // We need to know we can use it in at least one of the enabled channels
  // Cycle through available channels, stop when at least one is enabled for the
  // specified dataRate.
  if (dataRateOk && channelMaskOk) // If false, skip the check
    {
      bool foundAvailableChannel = false;
      for (auto it = enabledChannels.begin (); it != enabledChannels.end (); it++)
        {
          NS_LOG_DEBUG ("MinDR: " << unsigned (channelList.at (*it)->GetMinimumDataRate ()));
          NS_LOG_DEBUG ("MaxDR: " << unsigned (channelList.at (*it)->GetMaximumDataRate ()));
          if (channelList.at (*it)->GetMinimumDataRate () <= dataRate &&
              channelList.at (*it)->GetMaximumDataRate () >= dataRate)
            {
              foundAvailableChannel = true;
              break;
            }
        }

      if (!foundAvailableChannel)
        {
          dataRateOk = false;
        }
    }

  // Check the txPower
  ////////////////////
  // Check whether we can use this transmission power
  if (GetDbmForTxPower (txPower) == 0)
    {
      txPowerOk = false;
    }

  NS_LOG_DEBUG ("Finished checking. " <<
                "ChannelMaskOk: " << channelMaskOk << ", " <<
                "DataRateOk: " << dataRateOk << ", " <<
                "txPowerOk: " << txPowerOk);

  // If all checks are successful, set parameters up
  //////////////////////////////////////////////////
  if (channelMaskOk && dataRateOk && txPowerOk)
    {
      // Cycle over all channels in the list
      for (uint32_t i = 0; i < m_channelHelper.GetChannelList ().size (); i++)
        {
          if (std::find (enabledChannels.begin (), enabledChannels.end (), i) != enabledChannels.end ())
            {
              m_channelHelper.GetChannelList ().at (i)->SetEnabledForUplink ();
            }
          else
            {
              m_channelHelper.GetChannelList ().at (i)->DisableForUplink ();
            }
        }

      // Set the data rate
      m_dataRate = dataRate;

      // Set the transmission power
      m_txPower = GetDbmForTxPower (txPower);
    }

  // Craft a LinkAdrAns MAC command as a response
  ///////////////////////////////////////////////
  m_macCommandList.push_back (CreateObject<LinkAdrAns> (txPowerOk, dataRateOk,
                                                        channelMaskOk));
}

void
EndDeviceLoraMac::OnDutyCycleReq (double dutyCycle)
{
  NS_LOG_FUNCTION (this << dutyCycle);

  // Make sure we get a value that makes sense
  NS_ASSERT (0 <= dutyCycle && dutyCycle < 1);

  // Set the new duty cycle value
  m_aggregatedDutyCycle = dutyCycle;

  // Craft a DutyCycleAns as response
  NS_LOG_INFO ("Adding DutyCycleAns reply");
  m_macCommandList.push_back (CreateObject<DutyCycleAns> ());
}

void
EndDeviceLoraMac::OnRxParamSetupReq (uint8_t rx1DrOffset, uint8_t rx2DataRate, double frequency)
{
  NS_LOG_FUNCTION (this << unsigned (rx1DrOffset) << unsigned (rx2DataRate) <<
                   frequency);

  bool offsetOk = true;
  bool dataRateOk = true;

  // Check that the desired offset is valid
  if ( !(0 <= rx1DrOffset && rx1DrOffset <= 5))
    {
      offsetOk = false;
    }

  // Check that the desired data rate is valid
  if (GetSfFromDataRate (rx2DataRate) == 0 ||
      GetBandwidthFromDataRate (rx2DataRate) == 0)
    {
      dataRateOk = false;
    }

  // For now, don't check for validity of frequency
  if (offsetOk && dataRateOk)
    {
      //m_secondReceiveWindowDataRate = rx2DataRate;
      m_rx1DrOffset = rx1DrOffset;
      m_secondReceiveWindowFrequency = frequency;
    }

  // Craft a RxParamSetupAns as response
  NS_LOG_INFO ("Adding RxParamSetupAns reply");
  m_macCommandList.push_back (CreateObject<RxParamSetupAns> (offsetOk,
                                                             dataRateOk, true));
}

void
EndDeviceLoraMac::OnDevStatusReq (void)
{
  NS_LOG_FUNCTION (this);

  uint8_t battery = 10; // XXX Fake battery level
  uint8_t margin = 10; // XXX Fake margin

  // Craft a RxParamSetupAns as response
  NS_LOG_INFO ("Adding DevStatusAns reply");
  m_macCommandList.push_back (CreateObject<DevStatusAns> (battery, margin));
}

void
EndDeviceLoraMac::OnNewChannelReq (uint8_t chIndex, double frequency, uint8_t minDataRate, uint8_t maxDataRate)
{
  NS_LOG_FUNCTION (this);

  bool dataRateRangeOk = true; // XXX Check whether the new data rate range is ok
  bool channelFrequencyOk = true; // XXX Check whether the frequency is ok

  // TODO Return false if one of the checks above failed
  // TODO Create new channel in the LogicalLoraChannelHelper

  SetLogicalChannel (chIndex, frequency, minDataRate, maxDataRate);

  NS_LOG_INFO ("Adding NewChannelAns reply");
  m_macCommandList.push_back (CreateObject<NewChannelAns> (dataRateRangeOk,
                                                           channelFrequencyOk));
}

void
EndDeviceLoraMac::AddLogicalChannel (double frequency)
{
  NS_LOG_FUNCTION (this << frequency);

  m_channelHelper.AddChannel (frequency);
}

void
EndDeviceLoraMac::AddLogicalChannel (Ptr<LogicalLoraChannel> logicalChannel)
{
  NS_LOG_FUNCTION (this << logicalChannel);

  m_channelHelper.AddChannel (logicalChannel);
}

void
EndDeviceLoraMac::SetLogicalChannel (uint8_t chIndex, double frequency,
                                     uint8_t minDataRate, uint8_t maxDataRate)
{
  NS_LOG_FUNCTION (this << unsigned (chIndex) << frequency <<
                   unsigned (minDataRate) << unsigned(maxDataRate));

  m_channelHelper.SetChannel (chIndex, CreateObject<LogicalLoraChannel>
                                (frequency, minDataRate, maxDataRate));
}

void
EndDeviceLoraMac::AddSubBand (double startFrequency, double endFrequency, double dutyCycle, double maxTxPowerDbm)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_channelHelper.AddSubBand (startFrequency, endFrequency, dutyCycle, maxTxPowerDbm);
}

uint8_t
EndDeviceLoraMac::GetFirstReceiveWindowDataRate (void)
{

  if (not m_ackdifferentchannel){
	  m_firstReceiveWindowDataRate = m_dataRate;
  }

  //NS_LOG_INFO ("First receive window DR - "<< unsigned (m_firstReceiveWindowDataRate));
  return m_replyDataRateMatrix.at (m_firstReceiveWindowDataRate).at (m_rx1DrOffset);
}

uint8_t
EndDeviceLoraMac::GetSecondReceiveWindowDataRate (void)
{

  if (not m_ackdifferentchannel){
	  m_secondReceiveWindowDataRate = m_dataRate;
  }

  //NS_LOG_INFO ("Second receive window DR - "<< unsigned (m_secondReceiveWindowDataRate));
  return m_replyDataRateMatrix.at (m_secondReceiveWindowDataRate).at (m_rx1DrOffset);

}

void
EndDeviceLoraMac::SetACKParams (bool differentchannel, bool secondreceivewindow, double ackfrequency, int ackdatarate, int acklength)
{

  NS_LOG_FUNCTION (this << secondreceivewindow);

  if (differentchannel)
  {
	  m_firstReceiveWindowFrequency = ackfrequency;
	  m_secondReceiveWindowFrequency = ackfrequency;

	  m_secondReceiveWindowDataRate = ackdatarate;
	  m_firstReceiveWindowDataRate = ackdatarate;
  }

  m_sendsecondreceivewindow = secondreceivewindow;
  m_ackdifferentchannel = differentchannel;

  m_acklength = acklength;

}

void
EndDeviceLoraMac::SetRRX (bool retransmission, uint32_t rxnumber)
{
	NS_LOG_FUNCTION (this << retransmission << " No " << rxnumber);
	m_retransmission = retransmission;
	m_rxnumber = rxnumber;

}

double
EndDeviceLoraMac::GetSecondReceiveWindowFrequency (void)
{
  return m_secondReceiveWindowFrequency;

}

uint8_t
EndDeviceLoraMac::GetAckPacketLength (void)
{
	return m_acklength;
}

double
EndDeviceLoraMac::GetAggregatedDutyCycle (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  return m_aggregatedDutyCycle;
}
}
