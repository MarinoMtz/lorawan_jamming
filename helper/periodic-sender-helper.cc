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

  double m_interval = interval.GetSeconds();

  app->SetInitialDelay (Seconds (m_initialDelay->GetValue (0, 36000)));

  //app->SetInitialDelay (Seconds (0));

  app->SetNode (node);

  node->AddApplication (app);

  app->SetPktSize (m_size);

  return app;
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
} // namespace ns3
