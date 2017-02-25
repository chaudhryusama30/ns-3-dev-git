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

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/object-factory.h"
#include "ns3/drop-tail-queue.h"
#include "fifo-queue-disc.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FifoQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (FifoQueueDisc);

TypeId FifoQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FifoQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<FifoQueueDisc> ()
    .AddAttribute ("Mode",
                   "Whether to use bytes (see MaxBytes) or packets (see MaxPackets) as the maximum queue disc size metric.",
                   EnumValue (QUEUE_DISC_MODE_PACKETS),
                   MakeEnumAccessor (&FifoQueueDisc::SetMode,
                                     &FifoQueueDisc::GetMode),
                   MakeEnumChecker (QUEUE_DISC_MODE_BYTES, "QUEUE_DISC_MODE_BYTES",
                                    QUEUE_DISC_MODE_PACKETS, "QUEUE_DISC_MODE_PACKETS"))
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets accepted by this queue disc.",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&FifoQueueDisc::m_maxPackets),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxBytes",
                   "The maximum number of bytes accepted by this queue disc.",
                   UintegerValue (1000 * 65535),
                   MakeUintegerAccessor (&FifoQueueDisc::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

FifoQueueDisc::FifoQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

FifoQueueDisc::~FifoQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
FifoQueueDisc::SetMode (QueueDiscMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_mode = mode;
}

FifoQueueDisc::QueueDiscMode
FifoQueueDisc::GetMode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mode;
}

bool
FifoQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  // If Queue::Enqueue fails, QueueDisc::Drop is called by the internal queue
  // because QueueDisc::AddInternalQueue sets the drop callback
  return GetInternalQueue (0)->Enqueue (item);
}

Ptr<QueueDiscItem>
FifoQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  return StaticCast<QueueDiscItem> (GetInternalQueue (0)->Dequeue ());
}

Ptr<const QueueDiscItem>
FifoQueueDisc::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  return StaticCast<const QueueDiscItem> (GetInternalQueue (0)->Peek ());
}

bool
FifoQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("FifoQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () != 0)
    {
      NS_LOG_ERROR ("FifoQueueDisc needs no packet filter");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      // create a DropTail queue
      Ptr<Queue> queue;
      if (m_mode == QUEUE_DISC_MODE_PACKETS)
        {
          queue = CreateObjectWithAttributes<DropTailQueue> ("Mode", EnumValue (Queue::QUEUE_MODE_PACKETS),
                                                             "MaxPackets", UintegerValue (m_maxPackets));
        }
      else
        {
          queue = CreateObjectWithAttributes<DropTailQueue> ("Mode", EnumValue (Queue::QUEUE_MODE_BYTES),
                                                             "MaxPackets", UintegerValue (m_maxBytes));
        }
      AddInternalQueue (queue);
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("FifoQueueDisc needs 1 internal queue");
      return false;
    }

  if ((m_mode == FifoQueueDisc::QUEUE_DISC_MODE_PACKETS && GetInternalQueue (0)->GetMaxPackets () != m_maxPackets) ||
      (m_mode == FifoQueueDisc::QUEUE_DISC_MODE_BYTES && GetInternalQueue (0)->GetMaxBytes () != m_maxBytes))
    {
      NS_LOG_ERROR ("The size of the internal queue is different than the queue disc limit");
      return false;
    }

  return true;
}

void
FifoQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
}

} // namespace ns3
