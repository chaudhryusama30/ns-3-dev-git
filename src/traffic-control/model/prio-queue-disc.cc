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
#include "ns3/socket.h"
#include "prio-queue-disc.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PrioQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (PrioQueueDisc);

TypeId PrioQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PrioQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<PrioQueueDisc> ()
  ;
  return tid;
}

PrioQueueDisc::PrioQueueDisc ()
  : m_prio2band ({1, 2, 2, 2, 1, 2, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1})
{
  NS_LOG_FUNCTION (this);
}

PrioQueueDisc::~PrioQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
PrioQueueDisc::SetBandForPriority (uint16_t prio, uint16_t band)
{
  NS_LOG_FUNCTION (this << prio << band);

  NS_ASSERT_MSG (prio < 16, "Priority values must be less than 16");
  m_prio2band[prio] = band;
}

uint16_t
PrioQueueDisc::GetBandForPriority (uint16_t prio) const
{
  NS_LOG_FUNCTION (this << prio);

  NS_ASSERT_MSG (prio < 16, "Priority values must be less than 16");
  return m_prio2band[prio];
}

bool
PrioQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  uint8_t priority = 0;
  SocketPriorityTag priorityTag;
  if (item->GetPacket ()->PeekPacketTag (priorityTag))
    {
      priority = priorityTag.GetPriority ();
    }

  uint32_t band = m_prio2band[priority & 0x0f];

  bool retval = GetQueueDiscClass (band)->GetQueueDisc ()->Enqueue (item);

  // If Queue::Enqueue fails, QueueDisc::Drop is called by the child queue disc
  // because QueueDisc::AddQueueDiscClass sets the drop callback

  NS_LOG_LOGIC ("Number packets band " << band << ": " << GetQueueDiscClass (band)->GetQueueDisc ()->GetNPackets ());

  return retval;
}

Ptr<QueueDiscItem>
PrioQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item;

  for (uint32_t i = 0; i < GetNQueueDiscClasses (); i++)
    {
      if ((item = StaticCast<QueueDiscItem> (GetQueueDiscClass (i)->GetQueueDisc ()->Dequeue ())) != 0)
        {
          NS_LOG_LOGIC ("Popped from band " << i << ": " << item);
          NS_LOG_LOGIC ("Number packets band " << i << ": " << GetQueueDiscClass (i)->GetQueueDisc ()->GetNPackets ());
          return item;
        }
    }
  
  NS_LOG_LOGIC ("Queue empty");
  return item;
}

Ptr<const QueueDiscItem>
PrioQueueDisc::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  Ptr<const QueueDiscItem> item;

  for (uint32_t i = 0; i < GetNQueueDiscClasses (); i++)
    {
      if ((item = StaticCast<const QueueDiscItem> (GetQueueDiscClass (i)->GetQueueDisc ()->Peek ())) != 0)
        {
          NS_LOG_LOGIC ("Peeked from band " << i << ": " << item);
          NS_LOG_LOGIC ("Number packets band " << i << ": " << GetQueueDiscClass (i)->GetQueueDisc ()->GetNPackets ());
          return item;
        }
    }

  NS_LOG_LOGIC ("Queue empty");
  return item;
}

bool
PrioQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNInternalQueues () > 0)
    {
      NS_LOG_ERROR ("PrioQueueDisc cannot have internal queues");
      return false;
    }

  if (GetNPacketFilters () != 0)
    {
      NS_LOG_ERROR ("PrioQueueDisc currently does not use packet filters");
      return false;
    }

  if (GetNQueueDiscClasses () == 0)
    {
      // create 3 fifo queues
      ObjectFactory factory;
      factory.SetTypeId ("ns3::FifoQueueDisc");
      for (uint8_t i = 0; i < 2; i++)
        {
          Ptr<QueueDiscClass> qdclass = CreateObject<QueueDiscClass> ();
          qdclass->SetQueueDisc (factory.Create<QueueDisc> ());
          AddQueueDiscClass (qdclass);
        }
    }

  if (GetNQueueDiscClasses () < 2)
    {
      NS_LOG_ERROR ("PrioQueueDisc needs at least 2 classes");
      return false;
    }

  return true;
}

void
PrioQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
}

} // namespace ns3
