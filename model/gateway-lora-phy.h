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

#ifndef GATEWAY_LORA_PHY_H
#define GATEWAY_LORA_PHY_H

#include "ns3/object.h"
#include "ns3/net-device.h"
#include "ns3/nstime.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include "ns3/lora-phy.h"
#include "ns3/traced-value.h"
#include <list>

using namespace std;

namespace ns3 {

class LoraChannel;

/**
 * Class modeling a Lora SX1301 chip.
 *
 * This class models the behaviour of the chip employed in Lora gateways. These
 * chips are characterized by the presence of 8 receive paths, or parallel
 * receivers, which can be employed to listen to different channels
 * simultaneously. This characteristic of the chip is modeled using the
 * ReceivePath class, which describes a single parallel receiver. GatewayLoraPhy
 * essentially holds and manages a collection of these objects.
 */
class GatewayLoraPhy : public LoraPhy
{
public:

  static TypeId GetTypeId (void);

  GatewayLoraPhy();
  virtual ~GatewayLoraPhy();

  virtual void StartReceive (Ptr<Packet> packet, double rxPowerDbm, uint8_t sf,
                             Time duration, double frequencyMHz);

  virtual void EndReceive (Ptr<Packet> packet, Ptr<LoraInterferenceHelper::Event> event);

  virtual void Send (Ptr<Packet> packet, LoraTxParameters txParams,
                     double frequencyMHz, double txPowerDbm);

  virtual void TxFinished (Ptr<Packet> packet);

  bool IsTransmitting (void);

  void SetInterferenceModel (uint8_t inter);

  void SetDelta (double delta);

  virtual bool IsOnFrequency (double frequencyMHz);

  /**
   * Add a reception path, locked on a specific frequency.
   *
   * \param frequencyMHz The frequency on which to set this ReceptionPath.
   */
  void AddReceptionPath (double frequencyMHz);

  /**
   * Reset the list of reception paths.
   *
   * This method deletes all currently available ReceptionPath objects.
   */
  void ResetReceptionPaths (void);

  /**
   * A vector containing the sensitivities required to correctly decode
   * different spreading factors.
   */

  void Authpreamble (void);

  bool m_authpre;

  static const double sensitivity[6];

private:

  /**
   * This class represents a configurable reception path.
   *
   * ReceptionPaths are configured to listen on a certain frequency. Differently
   * from EndDeviceLoraPhys, these do not need to be configured to listen for a
   * certain SF. ReceptionPaths be either locked on an event or free.
   */
  class ReceptionPath : public SimpleRefCount<GatewayLoraPhy::ReceptionPath>
  {

public:

    /**
     * Constructor.
     *
     * \param frequencyMHz The frequency this path is set to listen on.
     */
    ReceptionPath (double frequencyMHz);

    ~ReceptionPath();

    /**
     * Getter for the operating frequency.
     *
     * \return The frequency this ReceivePath is configured to listen on.
     */
    double GetFrequency (void);

    /**
     * Setter for the frequency.
     *
     * \param frequencyMHz The frequency [MHz] this ReceptionPath will listen on.
     */
    void SetFrequency (double frequencyMHz);

    /**
     * Query whether this reception path is available to lock on a signal.
     *
     * \return True if its current state is free, false if it's currently locked.
     */
    bool IsAvailable (void);

    /**
     * Set this reception path as available.
     *
     * This function sets the m_available variable as true, and deletes the
     * LoraInterferenceHelper Event this ReceivePath was previously locked on.
     */
    void Free (void);

    /**
     * Set this reception path as not available and lock it on the
     * provided event.
     *
     * \param event The LoraInterferenceHelper Event to lock on.
     */
    void LockOnEvent (Ptr<LoraInterferenceHelper::Event> event);

    /**
     * Set the event this reception path is currently on.
     *
     * \param event the event to lock this ReceptionPath on.
     */
    void SetEvent (Ptr<LoraInterferenceHelper::Event> event);

    /**
     * Get the event this reception path is currently on.
     *
     * \returns 0 if no event is currently being received, a pointer to
     * the event otherwise.
     */
    Ptr<LoraInterferenceHelper::Event> GetEvent (void);

private:

    /**
     * The frequency this path is currently listening on, in MHz.
     */
    double m_frequencyMHz;

    /**
     * Whether this reception path is available to lock on a signal or not.
     */
    bool m_available;

    /**
     * The event this reception path is currently locked on.
     */
    Ptr< LoraInterferenceHelper::Event > m_event;
  };

  /**
   * A list containing the various parallel receivers that are managed by this
   * Gateway.
   */
  std::list<Ptr<ReceptionPath> > m_receptionPaths;

  /**
   * The number of occupied reception paths.
   */
  TracedValue<int> m_occupiedReceptionPaths;

  /**
   * Trace source that is fired when a packet cannot be received because all
   * available ReceivePath instances are busy.
   *
   * \see class CallBackTraceSource
   */

  TracedCallback<Ptr<const Packet>, uint32_t, uint32_t, double, uint8_t> m_noMoreDemodulators;

  bool m_isTransmitting; //!< Flag indicating whether a transmission is going on
};

} /* namespace ns3 */

#endif /* GATEWAY_LORA_PHY_H */
