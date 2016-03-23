/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_MAC_QUEUE_H
#define WIFI_MAC_QUEUE_H

#include <list>
#include <utility>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/queue.h"
#include "wifi-mac-header.h"

namespace ns3 {
class QosBlockedDestinations;

/**
 * \ingroup wifi
 *
 * WifiMacQueueItem is a subclass of QueueItem which stores (const) packets
 * along with their Wifi MAC headers and the time when they were enqueued.
 */
class WifiMacQueueItem : public QueueItem {
public:
  /**
   * \brief Create a Wifi MAC queue item containing a packet and a Wifi MAC header.
   * \param p the const packet included in the created item.
   * \param header the Wifi Mac header included in the created item.
   */
  WifiMacQueueItem (Ptr<const Packet> p, const WifiMacHeader & header);

  virtual ~WifiMacQueueItem ();

  /**
   * \brief Get the header stored in this item
   * \return the header stored in this item.
   */
  const WifiMacHeader & GetHeader (void) const;

  /**
   * \brief Return the requested address present in the header
   * \param type the type of the address to return
   * \return the address
   */
  Mac48Address GetAddress (enum WifiMacHeader::AddressType type) const;

  /**
   * \brief Get the timestamp included in this item
   * \return the timestamp included in this item.
   */
  Time GetTimeStamp (void) const;

  /**
   * \brief Set the timestamp to store in this item
   * \param tstamp the timestamp to store in this item.
   */
  void SetTimeStamp (Time tstamp);

  /**
   * \brief Get the id of the event scheduled to remove this item from the queue
   * \return the id of the event scheduled to remove this item from the queue.
   */
  EventId GetRemoveEvent (void) const;

  /**
   * \brief Set the id of the event scheduled to remove this item from the queue
   * \param eid the id of the event scheduled to remove this item from the queue.
   */
  void SetRemoveEvent (EventId eid);

  /**
   * \brief Print the item contents.
   * \param os output stream in which the data should be printed.
   */
  virtual void Print (std::ostream &os) const;

private:
  /**
   * \brief Default constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  WifiMacQueueItem ();
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  WifiMacQueueItem (const WifiMacQueueItem &);
  /**
   * \brief Assignment operator
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  WifiMacQueueItem &operator = (const WifiMacQueueItem &);

  WifiMacHeader m_header;   //!< Wifi MAC header associated with the packet
  Time m_tstamp;            //!< timestamp when the packet arrived at the queue
  EventId m_removeEvent;    //!< id of the event scheduled to remove this item from the queue
};

/**
 * \ingroup wifi
 *
 * This queue implements the timeout procedure described in
 * (Section 9.19.2.6 "Retransmit procedures" paragraph 6; IEEE 802.11-2012).
 *
 * When a packet is received by the MAC, to be sent to the PHY,
 * it is queued in the internal queue after being tagged by the
 * current time.
 *
 * When a packet is dequeued, the queue checks its timestamp
 * to verify whether or not it should be dropped. If
 * dot11EDCATableMSDULifetime has elapsed, it is dropped.
 * Otherwise, it is returned to the caller.
 */
class WifiMacQueue : public Queue
{
public:
  static TypeId GetTypeId (void);
  WifiMacQueue ();
  ~WifiMacQueue ();

  /**
   * Set the maximum delay before the packet is discarded.
   *
   * \param delay the maximum delay
   */
  void SetMaxDelay (Time delay);
  /**
   * Return the maximum delay before the packet is discarded.
   *
   * \return the maximum delay
   */
  Time GetMaxDelay (void) const;

  /**
   * Do NOT use this method, use PushBack instead.
   */
  virtual bool Enqueue (Ptr<QueueItem> item);
  /**
   * Do NOT use this method, use PopFront instead.
   */
  virtual Ptr<QueueItem> Dequeue (void);
  /**
   * Do NOT use this method, use PeekFront instead.
   */
  virtual Ptr<const QueueItem> Peek (void) const;

  /**
   * Enqueue the given item at the <i>end</i> of the queue.
   *
   * \param item the WifiMacQueueItem object to be enqueued at the end
   * \return true if the operation was successful; false otherwise
   */
  virtual bool PushBack (Ptr<WifiMacQueueItem> item);
  /**
   * Dequeue the item in the front of the queue.
   *
   * \return the WifiMacQueueItem object
   */
  virtual Ptr<WifiMacQueueItem> PopFront (void);
  /**
   * Peek the item in the front of the queue. The packet is not removed.
   *
   * \return the WifiMacQueueItem object
   */
  virtual Ptr<const WifiMacQueueItem> PeekFront (void);
  /**
   * Enqueue the given item at the <i>front</i> of the queue.
   * Note that an assert will fail in DoInsert if you attempt to enqueue
   * an item of type other than WifiMacQueueItem.
   *
   * \param item the WifiMacQueueItem object to be enqueued at the front
   *
   * \return true if the operation was successful; false otherwise
   */
  bool PushFront (Ptr<WifiMacQueueItem> item);
  /**
   * Search and return, if it is present in this queue, the first item having
   * the address indicated by <i>type</i> equal to <i>addr</i>, and tid
   * equal to <i>tid</i>. This method removes the item from this queue.
   * It is typically used by ns3::EdcaTxopN in order to perform correct MSDU
   * aggregation (A-MSDU).
   *
   * \param tid the given TID
   * \param type the given address type
   * \param addr the given destination
   *
   * \return the item
   */
  Ptr<WifiMacQueueItem> DequeueByTidAndAddress (uint8_t tid,
                                                WifiMacHeader::AddressType type,
                                                Mac48Address addr);
  /**
   * Search and return, if it is present in this queue, the first item having
   * the address indicated by <i>type</i> equal to <i>addr</i>, and tid
   * equal to <i>tid</i>. This method does not remove the item from this queue.
   * It is typically used by ns3::EdcaTxopN in order to perform correct MSDU
   * aggregation (A-MSDU).
   *
   * \param tid the given TID
   * \param type the given address type
   * \param addr the given destination
   *
   * \return the item
   */
  Ptr<const WifiMacQueueItem> PeekByTidAndAddress (uint8_t tid,
                                             WifiMacHeader::AddressType type,
                                             Mac48Address addr);
  /**
   * If exists, remove <i>item</i> from this queue and return true. Otherwise, it
   * takes no effects and return false. Deletion of the packet is
   * performed in linear time (O(n)).
   *
   * \param item the item to be removed
   *
   * \return true if the item was removed, false otherwise
   */
  bool Remove (Ptr<const WifiMacQueueItem> item);
  /**
   * If exists, remove the item storing the given <i>packet</i> from this queue
   * and return true. Otherwise, it takes no effects and return false. Deletion
   * of the packet is performed in linear time (O(n)).
   *
   * \param packet the packet to be removed
   *
   * \return true if the packet was removed, false otherwise
   */
  bool RemovePacket (Ptr<const Packet> p);
  /**
   * Return the number of QoS packets having tid equal to <i>tid</i> and the
   * address specified by <i>type</i> equal to <i>addr</i>.
   *
   * \param tid the given TID
   * \param type the given address type
   * \param addr the given destination
   *
   * \return the number of QoS packets
   */
  uint32_t GetNPacketsByTidAndAddress (uint8_t tid,
                                       WifiMacHeader::AddressType type,
                                       Mac48Address addr);
  /**
   * Return the item storing the first packet available for transmission. A packet
   * could not be available if it is a QoS packet with a tid and an address1 field
   * equal to <i>tid</i> and <i>addr</i> respectively that index a pending agreement
   * in the BlockAckManager object. Such packet must not be transmitted until
   * reception of an ADDBA response frame from the station addressed by <i>addr</i>.
   * This method removes the item from queue.
   *
   * \param blockedPackets
   *
   * \return the item
   */
  Ptr<WifiMacQueueItem> DequeueFirstAvailable (const QosBlockedDestinations *blockedPackets);
  /**
   * Return the item storing the first packet available for transmission. The packet
   * is not removed from queue.
   *
   * \param blockedPackets
   *
   * \return the item
   */
  Ptr<const WifiMacQueueItem> PeekFirstAvailable (const QosBlockedDestinations *blockedPackets);

  /**
   * Flush the queue.
   */
  void Flush (void);

protected:
  /**
   * typedef for packet queue.
   */
  typedef std::list<Ptr<WifiMacQueueItem> > PacketQueue;
  /**
   * typedef for packet queue const iterator.
   */
  typedef std::list<Ptr<WifiMacQueueItem> >::const_iterator PacketQueueCI;
  /**
   * typedef for packet queue iterator.
   */
  typedef std::list<Ptr<WifiMacQueueItem> >::iterator PacketQueueI;

private:
  virtual bool DoInsert (Ptr<QueueItem> item);
  virtual Ptr<QueueItem> DoExtract (void);
  virtual Ptr<const QueueItem> DoPeep (void) const;

  PacketQueue m_queue; //!< Packet queue
  PacketQueueI m_pos;  //!< Iterator pointing to the position in which operations are to be performed
  Time m_maxDelay;     //!< Time to live for packets in the queue
};

} //namespace ns3

#endif /* WIFI_MAC_QUEUE_H */
