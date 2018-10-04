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

#ifndef LORA_ENERGY_CONSUMPTION_HELPER_H
#define LORA_ENERGY_CONSUMPTION_HELPER_H

#include "ns3/nstime.h"
#include "string.h"
#include "ns3/simulator.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"
#include "ns3/traced-value.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/logical-lora-channel.h"
#include "ns3/lora-phy.h"

#include <tuple>
#include <list>

namespace ns3 {

/**
  * Helper for LoraPhy that manages energy consumption calculations.
  *
  */
class LoraEnergyConsumptionHelper
{
public:
  /**
   * A class representing the battery level in time.
   */

    class Event : public SimpleRefCount<LoraEnergyConsumptionHelper::Event>
  {

public:

    Event (int status, Time duration, double txPowerdBm, uint8_t spreadingFactor,
           double frequencyMHz, uint32_t node_id);
    ~Event ();


    int GetStatus (void) const;
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
    double GetTxPowerdBm (void) const;

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
     * Get NodeId.
     */

    double GetNodeId (void) const;

    /**
     * Print the current event in a human readable form.
     */
    void Print (std::ostream &stream) const;

private:

    /**
     * The status of the device
     */

    int m_status;

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
    double m_txPowerdBm;

    /**
     * The packet this event was generated for.
     */
    Ptr<Packet> m_packet;

    /**
     * The frequency this event was on.
     */
    double m_frequencyMHz;

    /**
     * The frequency this event was on.
     */
    double m_node_id;

};

  static TypeId GetTypeId (void);

  LoraEnergyConsumptionHelper();
  virtual ~LoraEnergyConsumptionHelper();

  /**
   * Add an event to the LoraEnergyConsumptionHelper
   *
   * \param duration the duration of the packet.
   * \param txPower the sent power in dBm.
   * \param spreadingFactor the spreading factor used by the transmission.
   * \param packet The packet carried by this transmission.
   * \param frequencyMHz The frequency this event was sent at.
   *
   * \return the newly created event
   */
  Ptr<LoraEnergyConsumptionHelper::Event> Add (int status, Time duration, double rxPower,
                                          uint8_t spreadingFactor,
                                          double frequencyMHz,
										  uint32_t node_id);

  /**
   * Print the events that are saved in this helper in a human readable format.
   */

  void PrintEvents (std::ostream &stream);

  /**
   * Compute the energy consumption due to transmissions.
   */

  double TxConso (Ptr<LoraEnergyConsumptionHelper::Event> event);

  /**
   * Compute the energy consumption due to reception.
   */

  double RxConso (Ptr<LoraEnergyConsumptionHelper::Event> event);

  /**
   * Compute the energy consumption due to a standby event
   */

  double StbConso (Ptr<LoraEnergyConsumptionHelper::Event> event);

  /**
   * Compute the energy consumption due to a sleep event
   */

  double SleepConso (Ptr<LoraEnergyConsumptionHelper::Event> event);

private:

  /**
   * Power consumption related to different states (tx,rx,standby,sleep).
   */
  static const double consotx[6];
  static const double consorx[6];
  static const double consostb;
  static const double consosleep;

};

/**
 * Allow easy logging of LoraEnergyConsumptionHelper Events
 */
std::ostream &operator << (std::ostream &os, const
                           LoraEnergyConsumptionHelper::Event &event);
}

#endif /* LORA_ENERGY_CONSUMPTION_HELPER_H */
