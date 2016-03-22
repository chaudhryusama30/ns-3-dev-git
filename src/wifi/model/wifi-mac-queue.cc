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

#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/assert.h"
#include "wifi-mac-queue.h"
#include "qos-blocked-destinations.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (WifiMacQueue);

WifiMacQueueItem::WifiMacQueueItem (Ptr<const Packet> p, const WifiMacHeader & header)
  : QueueItem (p),
    m_header (header)
{
}

WifiMacQueueItem::~WifiMacQueueItem()
{
}

const WifiMacHeader&
WifiMacQueueItem::GetHeader (void) const
{
  return m_header;
}

Mac48Address
WifiMacQueueItem::GetAddress (enum WifiMacHeader::AddressType type) const
{
  if (type == WifiMacHeader::ADDR1)
    {
      return m_header.GetAddr1 ();
    }
  if (type == WifiMacHeader::ADDR2)
    {
      return m_header.GetAddr2 ();
    }
  if (type == WifiMacHeader::ADDR3)
    {
      return m_header.GetAddr3 ();
    }
  return 0;
}

Time
WifiMacQueueItem::GetTimeStamp (void) const
{
  return m_tstamp;
}

void
WifiMacQueueItem::SetTimeStamp (Time tstamp)
{
  m_tstamp = tstamp;
}

void
WifiMacQueueItem::Print (std::ostream& os) const
{
  os << m_tstamp << " " << m_header << " " << GetConstPacket ()
  ;
}


TypeId
WifiMacQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiMacQueue")
    .SetParent<Queue> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiMacQueue> ()
//     .AddAttribute ("MaxPacketNumber", "If a packet arrives when there are already this number of packets, it is dropped.",
//                    UintegerValue (400),
//                    MakeUintegerAccessor (&WifiMacQueue::m_maxSize),
//                    MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxDelay", "If a packet stays longer than this delay in the queue, it is dropped.",
                   TimeValue (MilliSeconds (500.0)),
                   MakeTimeAccessor (&WifiMacQueue::m_maxDelay),
                   MakeTimeChecker ())
  ;
  return tid;
}

WifiMacQueue::WifiMacQueue ()
{
}

WifiMacQueue::~WifiMacQueue ()
{
  Flush ();
}

void
WifiMacQueue::SetMaxDelay (Time delay)
{
  m_maxDelay = delay;
}

Time
WifiMacQueue::GetMaxDelay (void) const
{
  return m_maxDelay;
}

bool
WifiMacQueue::DoInsert (Ptr<QueueItem> item)
{
  NS_ASSERT (m_queue.size () == GetNPackets ());

  Ptr<WifiMacQueueItem> wmqi = StaticCast<WifiMacQueueItem> (item);
  wmqi->SetTimeStamp (Simulator::Now ());
  m_queue.insert (m_pos, wmqi);
  return true;
}

Ptr<QueueItem>
WifiMacQueue::DoExtract (void)
{
  NS_ASSERT (m_queue.size () == GetNPackets ());

  Ptr<WifiMacQueueItem> wmqi = *m_pos;
  m_pos = m_queue.erase (m_pos);
  return wmqi;
}

Ptr<const QueueItem>
WifiMacQueue::DoPeep (void) const
{
  NS_ASSERT (m_queue.size () == GetNPackets ());

  return *m_pos;
}

bool
WifiMacQueue::Enqueue (Ptr<QueueItem> item)
{
  NS_FATAL_ERROR ("WifiMacQueue forbids the use of the Enqueue method.");
}

bool
WifiMacQueue::PushBack (Ptr<WifiMacQueueItem> item)
{
  Cleanup ();
  m_pos = m_queue.end ();
  return Insert (item);
}

void
WifiMacQueue::Cleanup (void)
{
  if (m_queue.empty ())
    {
      return;
    }

  Time now = Simulator::Now ();
  for (m_pos = m_queue.begin (); m_pos != m_queue.end (); )
    {
      if ((*m_pos)->GetTimeStamp () + m_maxDelay > now)
        {
          m_pos++;
        }
      else
        {
          Extract ();
        }
    }
}

Ptr<QueueItem>
WifiMacQueue::Dequeue (void)
{
  NS_FATAL_ERROR ("WifiMacQueue forbids the use of the Dequeue method.");
}

Ptr<WifiMacQueueItem>
WifiMacQueue::PopFront (void)
{
  Cleanup ();
  m_pos = m_queue.begin ();
  return StaticCast<WifiMacQueueItem> (Extract ());
}

Ptr<const QueueItem>
WifiMacQueue::Peek (void) const
{
  NS_FATAL_ERROR ("WifiMacQueue forbids the use of the Peek method.");
}

Ptr<const WifiMacQueueItem>
WifiMacQueue::PeekFront (void)
{
  Cleanup ();
  m_pos = m_queue.begin ();
  return StaticCast<const WifiMacQueueItem> (Peep ());
}

Ptr<WifiMacQueueItem>
WifiMacQueue::DequeueByTidAndAddress (uint8_t tid, WifiMacHeader::AddressType type, Mac48Address dest)
{
  Cleanup ();
  for (m_pos = m_queue.begin (); m_pos != m_queue.end (); ++m_pos)
    {
      if ((*m_pos)->GetHeader ().IsQosData () && (*m_pos)->GetAddress (type) == dest &&
            (*m_pos)->GetHeader ().GetQosTid () == tid)
        {
          return StaticCast<WifiMacQueueItem> (Extract ());
        }
    }

  return 0;
}

Ptr<const WifiMacQueueItem>
WifiMacQueue::PeekByTidAndAddress (uint8_t tid, WifiMacHeader::AddressType type, Mac48Address dest)
{
  Cleanup ();
  for (m_pos = m_queue.begin (); m_pos != m_queue.end (); ++m_pos)
    {
      if ((*m_pos)->GetHeader ().IsQosData () && (*m_pos)->GetAddress (type) == dest &&
            (*m_pos)->GetHeader ().GetQosTid () == tid)
        {
          return StaticCast<const WifiMacQueueItem> (Peep ());
        }
    }

  return 0;
}

bool
WifiMacQueue::Remove (Ptr<const WifiMacQueueItem> item)
{
  m_pos = m_queue.begin ();
  for (; m_pos != m_queue.end (); m_pos++)
    {
      if (*m_pos == item)
        {
          Extract ();
          return true;
        }
    }
  return false;
}

bool
WifiMacQueue::RemovePacket (Ptr<const Packet> p)
{
  m_pos = m_queue.begin ();
  for (; m_pos != m_queue.end (); m_pos++)
    {
      if ((*m_pos)->GetConstPacket () == p)
        {
          Extract ();
          return true;
        }
    }
  return false;
}

bool
WifiMacQueue::PushFront (Ptr<WifiMacQueueItem> item)
{
  Cleanup ();
  m_pos = m_queue.begin ();
  return Insert (item);
}

uint32_t
WifiMacQueue::GetNPacketsByTidAndAddress (uint8_t tid, WifiMacHeader::AddressType type,
                                          Mac48Address addr)
{
  Cleanup ();
  uint32_t nPackets = 0;
  for (m_pos = m_queue.begin (); m_pos != m_queue.end (); m_pos++)
    {
      if ((*m_pos)->GetAddress (type) == addr && (*m_pos)->GetHeader ().IsQosData () && (*m_pos)->GetHeader ().GetQosTid () == tid)
        {
          nPackets++;
        }
    }
  return nPackets;
}

Ptr<WifiMacQueueItem>
WifiMacQueue::DequeueFirstAvailable (const QosBlockedDestinations *blockedPackets)
{
  Cleanup ();
  for (m_pos = m_queue.begin (); m_pos != m_queue.end (); m_pos++)
    {
      if (!(*m_pos)->GetHeader ().IsQosData ()
          || !blockedPackets->IsBlocked ((*m_pos)->GetHeader ().GetAddr1 (), (*m_pos)->GetHeader ().GetQosTid ()))
        {
          return StaticCast<WifiMacQueueItem> (Extract ());
        }
    }
  return 0;
}

Ptr<const WifiMacQueueItem>
WifiMacQueue::PeekFirstAvailable (const QosBlockedDestinations *blockedPackets)
{
  Cleanup ();
  for (m_pos = m_queue.begin (); m_pos != m_queue.end (); m_pos++)
    {
      if (!(*m_pos)->GetHeader ().IsQosData ()
          || !blockedPackets->IsBlocked ((*m_pos)->GetHeader ().GetAddr1 (), (*m_pos)->GetHeader ().GetQosTid ()))
        {
          return StaticCast<const WifiMacQueueItem> (Peep ());
        }
    }
  return 0;
}

void
WifiMacQueue::Flush (void)
{
  while (!IsEmpty ())
    {
      PopFront ();
    }
}

} //namespace ns3
