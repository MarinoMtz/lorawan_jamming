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
  m_dutycycle = 0.01;
  m_exp = false;
  m_ransf = false;
  m_sf = 7;
  m_simtime = Seconds(0);
  m_lambda = 0;
  m_ranch=false;
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
  //double m_interval = interval.GetSeconds();
  app->SetInterval (interval);
  app->SetNode (node);
  node->AddApplication (app);
  app->SetPktSize (m_size);
  app->SetDC (m_dutycycle);
  app->SetExp (m_exp);
  app->SetRanSF (m_ransf);
  app->SetSpreadingFactor (m_sf);
  app->SetSimTime (m_simtime);
  app->SetLambda(m_lambda);
  app->SetFrequency(m_frequency);
  app->SetRanChannel (m_ranch);

  if (m_exp)
  {
	  app->SetInitialDelay (MilliSeconds (0));
  }
  	  else
  	  {
  		app->SetInitialDelay (Seconds (unsigned (m_initialDelay->GetValue (0, 1))));
	  }

  //
  //app->SetInitialDelay (Seconds (m_initialDelay->GetValue (0, 0.05)));

  return app;
}

void
AppJammerHelper::SetSimTime (Time simtime)
{
//  NS_LOG_FUNCTION (this << interval);
	m_simtime = simtime;
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

void
AppJammerHelper::SetDC (double dutycycle)
{
  m_dutycycle = dutycycle;
}

void
AppJammerHelper::SetExp (bool Exp)
{

  m_exp = Exp;
  NS_LOG_DEBUG ("IAT Exp " << m_exp);

}

void
AppJammerHelper::SetRanSF (bool RanSF)
{

  m_ransf = RanSF;
  NS_LOG_DEBUG ("Random SF " << m_ransf);

}

void
AppJammerHelper::SetRanChannel (bool RanCh)
{

  m_ranch = RanCh;
  NS_LOG_DEBUG ("Random Channel " << m_ranch);

}

void
AppJammerHelper::SetSpreadingFactor (uint8_t sf)
{
  m_sf = sf;
}

void
AppJammerHelper::SetFrequency (double Freq)
{
  m_frequency = Freq;
}

void
AppJammerHelper::SetLambda (double lambda)
{

  m_lambda = lambda;
  NS_LOG_DEBUG ("Lambda " << m_lambda);

}

} // namespace ns3
