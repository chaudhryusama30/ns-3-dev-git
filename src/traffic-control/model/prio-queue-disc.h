/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
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
 * Authors:  Stefano Avallone <stavallo@unina.it>
 */

#ifndef PRIO_QUEUE_DISC_H
#define PRIO_QUEUE_DISC_H

#include "ns3/queue-disc.h"
#include <vector>

namespace ns3 {

/**
 * \ingroup traffic-control
 *
 * Linux pfifo_fast is the default priority queue enabled on Linux
 * systems. Packets are enqueued in three FIFO droptail queues according
 * to three priority bands based on the packet priority.
 *
 * The system behaves similar to three ns3::DropTail queues operating
 * together, in which packets from higher priority bands are always
 * dequeued before a packet from a lower priority band is dequeued.
 *
 * The queue disc capacity, i.e., the maximum number of packets that can
 * be enqueued in the queue disc, is set through the limit attribute, which
 * plays the same role as txqueuelen in Linux. If no internal queue is
 * provided, three DropTail queues having each a capacity equal to limit are
 * created by default. User is allowed to provide queues, but they must be
 * three, operate in packet mode and each have a capacity not less
 * than limit. No packet filter can be provided.
 */
class PrioQueueDisc : public QueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief PrioQueueDisc constructor
   *
   * Creates a queue with a depth of 1000 packets per band by default
   */
  PrioQueueDisc ();

  virtual ~PrioQueueDisc();

  /**
   * Set the band (class) assigned to packets with specified priority.
   *
   * \param prio the priority of packets.
   * \param band the band assigned to packets.
   */
  void SetBandForPriority (uint16_t prio, uint16_t band);

  /**
   * Get the band (class) assigned to packets with specified priority.
   *
   * \param prio the priority of packets.
   * \returns the band assigned to packets.
   */
  uint16_t GetBandForPriority (uint16_t prio) const;

private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void) const;
  virtual bool CheckConfig (void);
  virtual void InitializeParams (void);

  std::vector<uint16_t> m_prio2band;    //!< Priority to band mapping
};

} // namespace ns3

#endif /* PRIO_QUEUE_DISC_H */
