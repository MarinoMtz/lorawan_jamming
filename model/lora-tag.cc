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

#include "ns3/lora-tag.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LoraTag);

TypeId
LoraTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoraTag")
    .SetParent<Tag> ()
    .SetGroupName ("lorawan")
    .AddConstructor<LoraTag> ();
  return tid;
}

TypeId
LoraTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

LoraTag::LoraTag () :
  m_sf (7),
  m_destroyedBy (0),
  m_receivePower (0),
  m_frequency (868.2),
  m_preamble (0.01254),
  m_senderid (0),
  m_jammer(0),
  m_pktid(0),
  m_gwid(0),
  m_ntx(0),
  m_retx(0)
{
}

LoraTag::~LoraTag ()
{
}

uint32_t
LoraTag::GetSerializedSize (void) const
{
  // Each datum about a SF is 1 byte + receivePower (the size of a double) +
  // frequency (the size of a double)

  return 4 + 3*sizeof(double) + 4*sizeof(uint32_t);
}

void
LoraTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_sf);
  i.WriteU8 (m_retx);
  i.WriteU8 (m_destroyedBy);
  i.WriteU8 (m_jammer);
  i.WriteDouble (m_receivePower);
  i.WriteDouble (m_frequency);
  i.WriteDouble (m_preamble);
  i.WriteU32 (m_senderid);
  i.WriteU32 (m_pktid);
  i.WriteU32 (m_gwid);
  i.WriteU32 (m_ntx);
}

void
LoraTag::Deserialize (TagBuffer i)
{
  m_sf = i.ReadU8 ();
  m_retx = i.ReadU8 ();
  m_destroyedBy = i.ReadU8 ();
  m_jammer = i.ReadU8 ();
  m_receivePower = i.ReadDouble ();
  m_frequency = i.ReadDouble ();
  m_preamble = i.ReadDouble ();
  m_senderid = i.ReadU32 ();
  m_pktid = i.ReadU32 ();
  m_gwid = i.ReadU32 ();
  m_ntx = i.ReadU32 ();
}

void
LoraTag::Print (std::ostream &os) const
{
  os << m_sf << " " << m_destroyedBy << " " << m_receivePower;
}

uint8_t
LoraTag::GetSpreadingFactor () const
{
  return m_sf;
}

uint8_t
LoraTag::GetDestroyedBy () const
{
  return m_destroyedBy;
}

double
LoraTag::GetReceivePower () const
{
  return m_receivePower;
}

void
LoraTag::SetDestroyedBy (uint8_t sf)
{
  m_destroyedBy = sf;
}

void
LoraTag::SetSpreadingFactor (uint8_t sf)
{
  m_sf = sf;
}

void
LoraTag::SetReceivePower (double receivePower)
{
  m_receivePower = receivePower;
}

void
LoraTag::SetFrequency (double frequency)
{
  m_frequency = frequency;
}

double
LoraTag::GetFrequency (void)
{
  return m_frequency;
}

double
LoraTag::GetPreamble (void)
{
  return m_preamble;
}

void
LoraTag::SetPreamble (double preamble)
{
  m_preamble = preamble;
}

uint32_t
LoraTag::GetSenderID (void)
{
  return m_senderid;
}

void
LoraTag::SetSenderID (uint32_t senderid)
{
	m_senderid = senderid;
}

uint8_t
LoraTag::GetJammer (void)
{
  return m_jammer;
}

void
LoraTag::SetJammer (uint8_t jammer)
{
  m_jammer = jammer;
}

uint32_t
LoraTag::GetPktID (void)
{
  return m_pktid;
}

void
LoraTag::SetPktID (uint32_t pktid)
{
  m_pktid = pktid;
}

uint32_t
LoraTag::GetGWID (void)
{
  return m_gwid;
}

void
LoraTag::SetGWid (uint32_t gwid)
{
	m_gwid = gwid;
}

uint32_t
LoraTag::Getntx (void)
{
  return m_ntx;
}

void
LoraTag::Setntx (uint32_t ntx)
{
	m_ntx = ntx;
}

uint8_t
LoraTag::GetRetx (void)
{

  return m_retx;
}

void
LoraTag::SetRetx (uint8_t retx)
{
	m_retx = retx;
}

} // namespace ns3
