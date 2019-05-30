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

#ifndef SIMPLE_NETWORK_SERVER_H
#define SIMPLE_NETWORK_SERVER_H

#include "ns3/application.h"
#include "ns3/net-device.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/packet.h"
#include "ns3/lora-device-address.h"
#include "ns3/device-status.h"
#include "ns3/gateway-status.h"
#include "ns3/node-container.h"
#include <vector>
#include <algorithm>
#include <numeric>

using namespace std;

namespace ns3 {

/**
 * A SimpleNetworkServer is an application standing on top of a node equipped with
 * links that connect it with the gateways.
 *
 * This version of the NetworkServer isn't smart enough to handle MAC commands,
 * but can reply correctly with ACKs to confirmed uplink messages.
 */
class SimpleNetworkServer : public Application
{
public:

  static TypeId GetTypeId (void);

  SimpleNetworkServer();
  virtual ~SimpleNetworkServer();

  /**
   * Parse and take action on the commands contained on the frameHeader
   */
  void ParseCommands (LoraFrameHeader frameHeader);

  /**
   * Start the NS application
   */
  void StartApplication (void);

  /**
   * Stop the NS application
   */
  void StopApplication (void);

  void SetStopTime (Time stop);

  void SetParameters (uint32_t GW, uint32_t ED, uint32_t JM, int buffer_length, bool ewma_training, double target, double lambda, double UCL, double LCL);

  void SetInterArrival (void);

  /**
   * Inform the SimpleNetworkServer that these nodes are connected to the network
   * This method will create a DeviceStatus object for each new node, and add it to the list
   */
  void AddNodes (NodeContainer nodes);

  /**
   * Inform the SimpleNetworkServer that this node is connected to the network
   * This method will create a DeviceStatus object for the new node (if it doesn't already exist)
   */
  void AddNode (Ptr<Node> node);

  /**
   * Add this gateway to the list of gateways connected to this NS
   */
  void AddGateway (Ptr<Node> gateway, Ptr<NetDevice> netDevice);

  /**
   * Receive a packet from a gateway
   * \param packet the received packet
   */
  bool Receive (Ptr<NetDevice> device, Ptr<const Packet> packet,
                uint16_t protocol, const Address& address);

  /**
   * Send a packet through a gateway to an ED, using the first receive window
   */
  void SendOnFirstWindow (LoraDeviceAddress address);

  /**
   * Send a packet through a gateway to an ED, using the second receive window
   */
  void SendOnSecondWindow (LoraDeviceAddress address);

  /**
   * Check whether a reply to the device with a certain address already exists
   */
  bool HasReply (LoraDeviceAddress address);

  /**
   * Get the data rate that should be used when replying in the first receive window
   */
  uint8_t GetDataRateForReply (uint8_t receivedDataRate);

  /**
   * Get the best gateway that is available to reply to this device.
   *
   * This method assumes the gateway needs to be available at the time that
   * it is called.
   */
  Address GetGatewayForReply (LoraDeviceAddress deviceAddress, double frequency);

  void PacketCounter (uint32_t pkt_ID, uint32_t gw_ID, uint32_t ed_ID);

  void InterArrivalTime(uint32_t ed_ID, double arrival_time);


  void EWMA(vector<double> IAT, uint32_t ed_ID);

  Time m_stop_time;

  /**
   * Trace source that is fired when a packet arrives at the NS, it contains information about
   * the Packet ID, ED and GW, and the time stamp.
   */

  TracedCallback< vector<uint32_t>, vector<uint32_t>, vector<uint32_t>, vector<uint32_t> > m_packetrx;

  TracedCallback< vector<vector<double> >, vector<vector<double> >,vector<vector<double> >,vector<vector<double> >,vector<vector<double> > > m_arrivaltime;

  TracedCallback< vector<double>, vector<double> > m_ewma;

  uint32_t m_devices;
  uint32_t m_gateways;
  uint32_t m_jammers;

  //variables related to the EWMA
  int m_buffer_length;
  double m_target;
  double m_lambda;

  //bool variable

  bool m_ewma_learning = false;

  vector<vector<uint32_t> > m_devices_pktid;

  vector<uint32_t> m_devices_pktreceive;
  vector<uint32_t> m_devices_pktduplicate;
  vector<uint32_t> m_gateways_pktreceive;
  vector<uint32_t> m_gateways_pktduplicate;

  vector<vector<double> > m_devices_interarrivaltime;
  vector<vector<double> > m_devices_arrivaltime;
  vector<double> m_last_arrivaltime_known;
  vector<vector<double> > m_devices_ewma;

  vector<double> m_ucl;
  vector<double> m_lcl;

  // pre-defined ucl and lcl (after training process)

  double m_pre_ucl;
  double m_pre_lcl;

    // ucl and lcl for tracing purposes

  vector<vector<double> > m_devices_ucl;
  vector<vector<double> > m_devices_lcl;
  vector<vector<double> > m_devices_ewma_total;
  vector<vector<double> > m_devices_interarrivaltime_total;
  vector<vector<double> > m_devices_arrivaltime_total;

  bool m_interarrivaltime;


  bool  AlreadyReceived(vector<uint32_t> vec_pkt_ID, uint32_t pkt_ID);

protected:
  std::map<LoraDeviceAddress,DeviceStatus> m_deviceStatuses;

  std::map<Address,GatewayStatus> m_gatewayStatuses;
};

} /* namespace ns3 */

#endif /* SIMPLE_NETWORK_SERVER_H */
