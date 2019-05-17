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

#include "ns3/app-jammer-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/periodic-sender.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AppJammerHelper");

AppJammerHelper::AppJammerHelper ()
{
  m_factory.SetTypeId ("ns3::AppJammer");
  m_initialDelay = CreateObject<UniformRandomVariable> ();
  m_size = 10;
}

AppJammerHelper::~AppJammerHelper ()
{
}

void
AppJammerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
AppJammerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
AppJammerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
AppJammerHelper::InstallPriv (Ptr<Node> node) const
{
  NS_LOG_FUNCTION (this << node);

  Ptr<AppJammer> app = m_factory.Create<AppJammer> ();

  Time interval;
  interval = m_period;
  double m_interval = interval.GetSeconds();
  app->SetInterval (interval);

  NS_LOG_DEBUG ("Created an application with interval = " <<interval.GetMinutes () << " minutes");

  app->SetInitialDelay (Seconds (unsigned (m_initialDelay->GetValue (0, 1))));

  //app->SetInitialDelay (MilliSeconds (0));

  //app->SetInitialDelay (Seconds (m_initialDelay->GetValue (0, 0.05)));

  app->SetNode (node);

  node->AddApplication (app);
  app->SetPktSize (m_size);

  return app;
}

void
AppJammerHelper::SetPeriod (Time period)
{
  m_period = period;
}

void
AppJammerHelper::SetPacketSize (uint16_t size)
{
  m_size = size;
}

} // namespace ns3
