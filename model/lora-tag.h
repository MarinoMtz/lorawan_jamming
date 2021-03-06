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

#ifndef LORA_TAG_H
#define LORA_TAG_H

#include "ns3/tag.h"

namespace ns3 {

/**
 * Tag used to save various data about a packet, like its Spreading Factor and
 * data about interference.
 */
class LoraTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * Create a LoraTag with a given spreading factor and collision.
   *
   * \param sf The Spreading Factor.
   * \param destroyedBy The SF this tag's packet was destroyed by.
   */
  LoraTag ();

  virtual ~LoraTag ();

  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize () const;
  virtual void Print (std::ostream &os) const;

  /**
   * Read which Spreading Factor this packet was transmitted with.
   *
   * \return This tag's packet's SF.
   */
  uint8_t GetSpreadingFactor () const;

  /**
   * Read which Spreading Factor this packet was destroyed by.
   *
   * \return The SF this packet was destroyed by.
   */
  uint8_t GetDestroyedBy () const;

  /**
   * Read the power this packet arrived with.
   *
   * \return This tag's packet received power.
   */
  double GetReceivePower () const;

  /**
   * Set which Spreading Factor this packet was transmitted with.
   *
   * \param sf The Spreading Factor.
   */
  void SetSpreadingFactor (uint8_t sf);

  /**
   * Set which Spreading Factor this packet was destroyed by.
   *
   * \param sf The Spreading Factor.
   */
  void SetDestroyedBy (uint8_t sf);

  /**
   * Set the power this packet was received with.
   *
   * \param receivePower The power, in dBm.
   */
  void SetReceivePower (double receivePower);

  /**
   * Set the frequency of the packet.
   *
   * This value works in two ways:
   * - It is used by the GW to signal to the NS the frequency of the uplink
       packet
   * - It is used by the NS to signal to the GW the freqeuncy of a downlink
       packet
   */
  void SetFrequency (double frequency);

  /**
   * Get the frequency of the packet.
   */
  double GetFrequency (void);

  double GetPreamble(void);

  void SetPreamble (double Preamble);
  /**
   * Set the data rate for this packet.
   *
   * \param dataRate The data rate.
   */

  uint32_t GetSenderID (void);
  void SetSenderID (uint32_t senderid);
  uint8_t GetJammer (void);
  void  SetJammer (uint8_t jammer);
  uint32_t GetPktID (void);
  void  SetPktID (uint32_t pktid);
  uint32_t GetGWID (void);
  void SetGWid (uint32_t gwid);

  uint32_t Getntx (void);
  void Setntx (uint32_t ntx);

  uint8_t GetRetx (void);
  void SetRetx (uint8_t retx);

private:
  uint8_t m_sf; //!< The Spreading Factor used by the packet.
  uint8_t m_destroyedBy; //!< The Spreading Factor that destroyed the packet.
  double m_receivePower; //!< The reception power of this packet.
                      //!packet.
  double m_frequency; //!< The frequency of this packet
  double m_preamble; //!< The preamble time of this packet
  uint32_t m_senderid;
  uint8_t m_jammer;
  uint32_t m_pktid;
  uint32_t m_gwid;

  uint32_t m_ntx;

  uint8_t m_retx; // Variable indicating if this packet is needed to be ackited

};
} // namespace ns3
#endif
