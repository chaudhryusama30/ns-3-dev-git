/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Universita' degli Studi di Napoli "Federico II"
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
 * Author: Pasquale Imputato <p.imputato@gmail.com>
 * Author: Stefano Avallone <stefano.avallone@unina.it>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

// This simple example shows how to use TrafficControlHelper to install a 
// QueueDisc on a device.
//
// The default QueueDisc is a pfifo_fast with a capacity of 1000 packets (as in
// Linux). However, in this example, we install a RedQueueDisc with a capacity
// of 10000 packets.
//
// Network topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point
//
// The output will consist of all the traced changes in the length of the RED
// internal queue and in the length of the netdevice queue:
//
//    DevicePacketsInQueue 0 to 1
//    TcPacketsInQueue 7 to 8
//    TcPacketsInQueue 8 to 9
//    DevicePacketsInQueue 1 to 0
//    TcPacketsInQueue 9 to 8
//
// plus some statistics collected at the network layer (by the flow monitor)
// and the application layer. Finally, the number of packets dropped by the
// queuing discipline, the number of packets dropped by the netdevice and
// the number of packets requeued by the queuing discipline are reported.
//
// If the size of the DropTail queue of the netdevice were increased from 1
// to a large number (e.g. 1000), one would observe that the number of dropped
// packets goes to zero, but the latency grows in an uncontrolled manner. This
// is the so-called bufferbloat problem, and illustrates the importance of
// having a small device queue, so that the standing queues build in the traffic
// control layer where they can be managed by advanced queue discs rather than
// in the device layer.

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TrafficControlExample");

void
TcPacketsInQueueTrace (uint32_t oldValue, uint32_t newValue)
{
  std::cout << "TcPacketsInQueue " << oldValue << " to " << newValue << std::endl;
}

void
DevicePacketsInQueueTrace (uint32_t oldValue, uint32_t newValue)
{
  std::cout << "DevicePacketsInQueue " << oldValue << " to " << newValue << std::endl;
}

int
main (int argc, char *argv[])
{
  double simulationTime = 10; //seconds
  std::string transportProt = "Tcp";
  std::string socketType;

  CommandLine cmd;
  cmd.AddValue ("transportProt", "Transport protocol to use: Tcp, Udp", transportProt);
  cmd.Parse (argc, argv);

  if (transportProt.compare ("Tcp") == 0)
    {
      socketType = "ns3::TcpSocketFactory";
    }
  else
    {
      socketType = "ns3::UdpSocketFactory";
    }

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  pointToPoint.SetQueue ("ns3::DropTailQueue", "Mode", StringValue ("QUEUE_MODE_PACKETS"), "MaxPackets", UintegerValue (1));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  TrafficControlHelper tch;

//   uint16_t handle = tch.SetRootQueueDisc ("ns3::RedQueueDisc");
//   tch.AddInternalQueues (handle, 1, "ns3::DropTailQueue", "MaxPackets", UintegerValue (10000));

  uint16_t handle = tch.SetRootQueueDisc ("ns3::PrioQueueDisc");
  TrafficControlHelper::ClassIdList cid = tch.AddQueueDiscClasses (handle, 2, "ns3::QueueDiscClass");
  tch.AddChildQueueDisc (handle, cid[0], "ns3::FifoQueueDisc");
  tch.AddChildQueueDisc (handle, cid[1], "ns3::RedQueueDisc");
  
  QueueDiscContainer qdiscs = tch.Install (devices);

  Ptr<QueueDisc> q = qdiscs.Get (1);
  q->TraceConnectWithoutContext ("PacketsInQueue", MakeCallback (&TcPacketsInQueueTrace));
  // Alternatively:
  // Config::ConnectWithoutContext ("/NodeList/1/$ns3::TrafficControlLayer/RootQueueDiscList/0/PacketsInQueue",
  //                                MakeCallback (&TcPacketsInQueueTrace));

  Ptr<NetDevice> nd = devices.Get (1);
  Ptr<PointToPointNetDevice> ptpnd = DynamicCast<PointToPointNetDevice> (nd);
  Ptr<Queue> queue = ptpnd->GetQueue ();
  queue->TraceConnectWithoutContext ("PacketsInQueue", MakeCallback (&DevicePacketsInQueueTrace));

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  uint32_t payloadSize = 1448;
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));

  //Flows
  for (uint8_t i = 0; i < 2; i++)
    {
      uint16_t port = 7 + 10 * i;
      Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
      PacketSinkHelper packetSinkHelper (socketType, localAddress);
      ApplicationContainer sinkApp = packetSinkHelper.Install (nodes.Get (0));

      sinkApp.Start (Seconds (0.0));
      sinkApp.Stop (Seconds (simulationTime + 0.1));

      OnOffHelper onoff (socketType, Ipv4Address::GetAny ());
      onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      onoff.SetAttribute ("PacketSize", UintegerValue (payloadSize));
      onoff.SetAttribute ("DataRate", StringValue ("50Mbps")); //bit/s
      ApplicationContainer apps;

      InetSocketAddress rmt (interfaces.GetAddress (0), port);
      if (i == 1)
        {
          rmt.SetTos (Ipv4Header::DSCP_AF12 << 2);
        }
      AddressValue remoteAddress (rmt);

      onoff.SetAttribute ("Remote", remoteAddress);
      apps.Add (onoff.Install (nodes.Get (1)));
      apps.Start (Seconds (1.0));
      apps.Stop (Seconds (simulationTime + 0.1));
    }

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Simulator::Stop (Seconds (simulationTime + 5));
  Simulator::Run ();

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (uint8_t i = 1; i < 3; i++)
    {
      std::cout << std::endl << "*** Flow monitor statistics ***" << std::endl;
      std::cout << "  Tx Packets:   " << stats[i].txPackets << std::endl;
      std::cout << "  Tx Bytes:   " << stats[i].txBytes << std::endl;
      std::cout << "  Offered Load: " << stats[i].txBytes * 8.0 / (stats[i].timeLastTxPacket.GetSeconds () - stats[i].timeFirstTxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
      std::cout << "  Rx Packets:   " << stats[i].rxPackets << std::endl;
      std::cout << "  Rx Bytes:   " << stats[i].rxBytes << std::endl;
      uint32_t packetsDroppedByQueueDisc = 0;
      uint64_t bytesDroppedByQueueDisc = 0;
      if (stats[i].packetsDropped.size () > Ipv4FlowProbe::DROP_QUEUE_DISC)
        {
          packetsDroppedByQueueDisc = stats[i].packetsDropped[Ipv4FlowProbe::DROP_QUEUE_DISC];
          bytesDroppedByQueueDisc = stats[i].bytesDropped[Ipv4FlowProbe::DROP_QUEUE_DISC];
        }
      std::cout << "  Packets Dropped by Queue Disc:   " << packetsDroppedByQueueDisc << std::endl;
      std::cout << "  Bytes Dropped by Queue Disc:   " << bytesDroppedByQueueDisc << std::endl;
      uint32_t packetsDroppedByNetDevice = 0;
      uint64_t bytesDroppedByNetDevice = 0;
      if (stats[i].packetsDropped.size () > Ipv4FlowProbe::DROP_QUEUE)
        {
          packetsDroppedByNetDevice = stats[i].packetsDropped[Ipv4FlowProbe::DROP_QUEUE];
          bytesDroppedByNetDevice = stats[i].bytesDropped[Ipv4FlowProbe::DROP_QUEUE];
        }
      std::cout << "  Packets Dropped by NetDevice:   " << packetsDroppedByNetDevice << std::endl;
      std::cout << "  Bytes Dropped by NetDevice:   " << bytesDroppedByNetDevice << std::endl;
      std::cout << "  Throughput: " << stats[i].rxBytes * 8.0 / (stats[i].timeLastRxPacket.GetSeconds () - stats[i].timeFirstRxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
      std::cout << "  Mean delay:   " << stats[i].delaySum.GetSeconds () / stats[i].rxPackets << std::endl;
      std::cout << "  Mean jitter:   " << stats[i].jitterSum.GetSeconds () / (stats[i].rxPackets - 1) << std::endl;
    }

  Simulator::Destroy ();

  std::cout << std::endl << "*** TC Layer statistics ***" << std::endl;
  std::cout << "  Packets dropped by the TC layer: " << q->GetTotalDroppedPackets () << std::endl;
  std::cout << "  Bytes dropped by the TC layer: " << q->GetTotalDroppedBytes () << std::endl;
  std::cout << "  Packets dropped by the netdevice: " << queue->GetTotalDroppedPackets () << std::endl;
  std::cout << "  Packets requeued by the TC layer: " << q->GetTotalRequeuedPackets () << std::endl;

  return 0;
}
