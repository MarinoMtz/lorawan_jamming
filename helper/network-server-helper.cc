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

#include "ns3/network-server-helper.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NetworkServerHelper");

NetworkServerHelper::NetworkServerHelper ()
{
  m_factory.SetTypeId ("ns3::SimpleNetworkServer");
  p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2pHelper.SetChannelAttribute ("Delay", StringValue ("2ms"));
}

NetworkServerHelper::~NetworkServerHelper ()
{
}

void
NetworkServerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
NetworkServerHelper::SetGateways (NodeContainer gateways)
{
  m_gateways = gateways;
}

void
NetworkServerHelper::SetJammers (NodeContainer jammers)
{
  m_jammers = jammers;
}

void
NetworkServerHelper::SetEndDevices (NodeContainer endDevices)
{
  m_endDevices = endDevices;
}



ApplicationContainer
NetworkServerHelper::Install (Ptr<Node> node)
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
NetworkServerHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

void
NetworkServerHelper::SetNS (Ptr<SimpleNetworkServer> app)
{
  m_ns = app;
}

Ptr<SimpleNetworkServer>
NetworkServerHelper::GetNS (void)
{
  return m_ns;
}

Ptr<Application>
NetworkServerHelper::InstallPriv (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);

  Ptr<SimpleNetworkServer> app = m_factory.Create<SimpleNetworkServer> ();

  app->SetNode (node);
  node->AddApplication (app);

  // Cycle on each gateway
  for (NodeContainer::Iterator i = m_gateways.Begin ();
       i != m_gateways.End ();
       i++)
    {
      // Add the connections with the gateway
      // Create a PointToPoint link between gateway and NS
      NetDeviceContainer container = p2pHelper.Install (node, *i);

      // Add the gateway to the NS list
      app->AddGateway (*i, container.Get (0));
    }

  // Link the SimpleNetworkServer to its NetDevices
  for (uint32_t i = 0; i < node->GetNDevices (); i++)
    {
      Ptr<NetDevice> currentNetDevice = node->GetDevice (i);
      currentNetDevice->SetReceiveCallback (MakeCallback
                                              (&SimpleNetworkServer::Receive,
                                              app));
    }

  // Add the end devices

  SetNS(app);
  app->SetParameters(m_gateways.GetN(), m_endDevices.GetN(), m_jammers.GetN(),m_buffer_length);
  app->AddNodes (m_endDevices);
  app->SetStopTime(m_stop_time);

  app->SetACKParams (m_differentchannel, m_secondreceivewindow,
		  m_ackfrequency, m_ackdatarate, m_acklength);

  if (m_ewma) {app->SetEWMA(m_ewma, m_target, m_lambda, m_ucl, m_lcl);}
  if (m_interarrivaltime) {app->SetInterArrival();}

  return app;
  }

  void
  NetworkServerHelper::SetStopTime (Time Stop)
  {
	  m_stop_time = Stop;
  }

  void
  NetworkServerHelper::SetBuffer (int buffer_length)
  {
	  m_buffer_length = buffer_length;
  }

  void
  NetworkServerHelper::SetInterArrival ()
  {
	  m_interarrivaltime = true;
  }

  void
  NetworkServerHelper::SetEWMA (bool ewma, double target, double lambda, double ucl, double lcl)
  {
	  m_target = target;
	  m_lambda = lambda;
	  m_ewma = ewma;
	  m_ucl = ucl;
	  m_lcl = lcl;
  }

  void
  NetworkServerHelper::SetACKParams (bool differentchannel, bool secondreceivewindow, double ackfrequency, int ackdatarate, int acklength)
  {
	  m_differentchannel = differentchannel;
	  m_secondreceivewindow = secondreceivewindow;
	  m_ackfrequency = ackfrequency;
	  m_ackdatarate = ackdatarate;
	  m_acklength = acklength;
  }





} // namespace ns3
