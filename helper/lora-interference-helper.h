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

#ifndef LORA_INTERFERENCE_HELPER_H
#define LORA_INTERFERENCE_HELPER_H

#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/logical-lora-channel.h"
#include "ns3/lora-tag.h"
#include <list>

namespace ns3 {

/**
  * Helper for LoraPhy that manages interference calculations.
  *
  * This class keeps a list of signals that are impinging on the antenna of the
  * device, in order to compute which ones can be correctly received and which
  * ones are lost due to interference.
  */

class LoraInterferenceHelper
{
public:
  /**
   * A class representing a signal in time.
   *
   * Used in LoraInterferenceHelper to keep track of which signals overlap and
   * cause destructive interference.
   */

    enum Int_Model {
    	Pure_ALOHA,
		CE_PowerLevel,
		CE_CumulEnergy,
		Cochannel_Matrix,
    };

  class Event : public SimpleRefCount<LoraInterferenceHelper::Event>
  {

public:

    Event (Time duration, double rxPowerdBm, uint8_t spreadingFactor,
           Ptr<Packet> packet, double frequencyMHz);
    ~Event ();

    /**
     * Get the duration of the event.
     */
    Time GetDuration (void) const;

    /**
     * Get the starting time of the event.
     */
    Time GetStartTime (void) const;

    /**
     * Get the ending time of the event.
     */
    Time GetEndTime (void) const;

    /**
     * Get the power of the event.
     */
    double GetRxPowerdBm (void) const;

    /**
     * Get the spreading factor used by this signal.
     */
    uint8_t GetSpreadingFactor (void) const;

    /**
     * Get the packet this event was generated for.
     */
    Ptr<Packet> GetPacket (void) const;

    /**
     * Get the frequency this event was on.
     */
    double GetFrequency (void) const;

    /**
     * Print the current event in a human readable form.
     */
    void Print (std::ostream &stream) const;

private:

    /**
     * The time this signal begins (at the device).
     */
    Time m_startTime;

    /**
     * The time this signal ends (at the device).
     */
    Time m_endTime;

    /**
     * The spreading factor of this signal.
     */
    uint8_t m_sf;

    /**
     * The power of this event in dBm (at the device).
     */
    double m_rxPowerdBm;

    /**
     * The packet this event was generated for.
     */
    Ptr<Packet> m_packet;

    /**
     * The frequency this event was on.
     */
    double m_frequencyMHz;

  };

  static TypeId GetTypeId (void);

  LoraInterferenceHelper();
  virtual ~LoraInterferenceHelper();

  /**
   * Add an event to the InterferenceHelper
   *
   * \param duration the duration of the packet.
   * \param rxPower the received power in dBm.
   * \param spreadingFactor the spreading factor used by the transmission.
   * \param packet The packet carried by this transmission.
   * \param frequencyMHz The frequency this event was sent at.
   *
   * \return the newly created event
   */

  Ptr<LoraInterferenceHelper::Event> Add (Time duration, double rxPower,
                                          uint8_t spreadingFactor,
                                          Ptr<Packet> packet,
                                          double frequencyMHz);

  /**
   * Get a list of the interferers currently registered at this
   * InterferenceHelper.
   */
  std::list< Ptr< LoraInterferenceHelper::Event > > GetInterferers ();


  void SetInterferenceModel(Int_Model);

  Int_Model GetInterferenceModel(void);


  void SetDelta (double delta);

  uint8_t GetDelta (void);

  /**
   * Print the events that are saved in this helper in a human readable format.
   */
  void PrintEvents (std::ostream &stream);

  /**
   * Determine whether the event was destroyed by interference or not. This is
   * the method where the SNIR tables come into play and the computations
   * regarding power are performed.

   * \param event The event for which to check the outcome.
   * \return The sf of the packets that caused the loss, or 0 if there was no
   * loss.
   */
  bool IsDestroyedByInterference (Ptr<LoraInterferenceHelper::Event> event);

  bool Cochannel (Ptr<LoraInterferenceHelper::Event> event);

  bool Aloha (Ptr<LoraInterferenceHelper::Event> event);

  bool CumulEnergy (Ptr<LoraInterferenceHelper::Event> event);

  bool PowerLevel (Ptr<LoraInterferenceHelper::Event> event);

   /**
   * Compute the time duration in which two given events are overlapping.
   *
   * \param event1 The first event
   * \param event2 The second event
   *
   * \return The overlap time
   */
  Time GetOverlapTime (Ptr< LoraInterferenceHelper:: Event> event1,
                       Ptr<LoraInterferenceHelper:: Event> event2);


  bool OnThePreamble (Ptr< LoraInterferenceHelper:: Event> event1,
                       Ptr<LoraInterferenceHelper:: Event> event2);

  bool GetFirst (Ptr<LoraInterferenceHelper::Event> event1,
                 Ptr<LoraInterferenceHelper::Event> event2);


  /**
   * Delete all events in the LoraInterferenceHelper.
   */
  void ClearAllEvents (void);

  /**
   * Delete old events in this LoraInterferenceHelper.
   */
  void CleanOldEvents (void);

  bool GetOnThePreamble (void);

  Time GetColStart (void);

  Time GetColEnd (void);

  uint8_t GetSF (void);

  bool  GetCE (void);

  void CleanParameters(void);

  bool m_onthepreamble;
  bool m_preamble;
  bool m_ce;
  Time m_colstart;
  Time m_colend;
  uint8_t m_colsf;

  Int_Model m_intmodel;
  double m_delta;

private:

  /**
   * A list of the events this LoraInterferenceHelper is keeping track of.
   */
  std::list< Ptr< LoraInterferenceHelper::Event > > m_events;

  /**
   * The matrix containing information about how packets survive interference.
   */
  static const double collisionSnir[6][6];

  /**
   * The threshold after which an event is considered old and removed from the
   * list.
   */
  static Time oldEventThreshold;

};

/**
 * Allow easy logging of LoraInterferenceHelper Events
 */
std::ostream &operator << (std::ostream &os, const
                           LoraInterferenceHelper::Event &event);
}

#endif /* LORA_INTERFERENCE_HELPER_H */
