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

#include "ns3/periodic-sender-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/periodic-sender.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PeriodicSenderHelper");

PeriodicSenderHelper::PeriodicSenderHelper ()
{
  m_factory.SetTypeId ("ns3::PeriodicSender");
  m_initialDelay = CreateObject<UniformRandomVariable> ();
  m_size = 10;
  m_exp = false;
  m_sf = 7;
  m_simtime = Seconds(0);
}

PeriodicSenderHelper::~PeriodicSenderHelper ()
{
}

void
PeriodicSenderHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
PeriodicSenderHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
PeriodicSenderHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
PeriodicSenderHelper::InstallPriv (Ptr<Node> node) const
{
  NS_LOG_FUNCTION (this << node);

  Ptr<PeriodicSender> app = m_factory.Create<PeriodicSender> ();

  Time interval;

  interval = m_period;

  app->SetInterval (interval);

  NS_LOG_DEBUG ("Created an application with interval = " << interval.GetMinutes () << " minutes");

  if (m_exp)
  {
	  app->SetInitialDelay (MilliSeconds (0));
  }
  	  else
  	  {
  		app->SetInitialDelay (Seconds (unsigned (m_initialDelay->GetValue (0, 100))));
	  }

  app->SetNode (node);
  node->AddApplication (app);
  app->SetPktSize (m_size);
  app->SetSpreadingFactor(m_sf);
  app->SetExp(m_exp);
  app->SetSimTime(m_simtime);

  return app;
}

void
PeriodicSenderHelper::SetSimTime (Time simtime)
{
//  NS_LOG_FUNCTION (this << interval);
	m_simtime = simtime;
}

void
PeriodicSenderHelper::SetPeriod (Time period)
{
  m_period = period;
}

void
PeriodicSenderHelper::SetPacketSize (uint16_t size)
{
  m_size = size;
}

void
PeriodicSenderHelper::SetExp (bool Exp)
{

  m_exp = Exp;
  NS_LOG_DEBUG ("IAT Exp " << m_exp);

}

void
PeriodicSenderHelper::SetSpreadingFactor (uint8_t sf)
{
  m_sf = sf;
}


} // namespace ns3
