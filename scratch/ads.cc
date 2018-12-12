/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Lucas Pacheco <lucas.pacheco@itec.ufpa.br>
 * 
 * Subscribe to Pewdiepie
 */

#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/applications-module.h"
#include "ns3/animation-interface.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
 #include "ns3/packet-sink.h"
 #include "ns3/packet-sink-helper.h"
 #include "ns3/on-off-helper.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/config-store.h"
#include "ns3/flow-monitor.h"
#include "ns3/core-module.h"
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/lte-module.h"
//#include "ns3/gtk-config-store.h"

using namespace ns3;

// Set logComponent
NS_LOG_COMPONENT_DEFINE ("ADSAssignment");

Ptr<PacketSink> sink;                         /* Pointer to the packet sink application */
uint64_t lastTotalRx = 0;                     /* The value of the last total received bytes */

// Handover Notifiers
void NotifyConnectionEstablishedUe(std::string context,
    uint64_t imsi,
    uint16_t cellid,
    uint16_t rnti)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
        << " " << context << " UE IMSI " << imsi
        << ": connected to CellId " << cellid << " with RNTI " << rnti);
}
void NotifyHandoverStartUe(std::string context,
    uint64_t imsi,
    uint16_t cellid,
    uint16_t rnti,
    uint16_t targetCellId)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
        << " " << context << " UE IMSI " << imsi
        << ": previously connected to CellId " << cellid << " with RNTI "
        << rnti << ", doing handover to CellId " << targetCellId);
}
void NotifyHandoverEndOkUe(std::string context,
    uint64_t imsi,
    uint16_t cellid,
    uint16_t rnti)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
        << " " << context << " UE IMSI " << imsi
        << ": successful handover to CellId " << cellid << " with RNTI "
        << rnti);
}
void NotifyConnectionEstablishedEnb(std::string context,
    uint64_t imsi,
    uint16_t cellid,
    uint16_t rnti)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
        << " " << context << " eNB CellId " << cellid
        << ": successful connection of UE with IMSI " << imsi << " RNTI "
        << rnti);
}
void NotifyHandoverStartEnb(std::string context,
    uint64_t imsi,
    uint16_t cellid,
    uint16_t rnti,
    uint16_t targetCellId)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
        << " " << context << " eNB CellId " << cellid
        << ": start handover of UE with IMSI " << imsi << " RNTI "
        << rnti << " to CellId " << targetCellId);
}
void NotifyHandoverEndOkEnb(std::string context,
    uint64_t imsi,
    uint16_t cellid,
    uint16_t rnti)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
        << " " << context << " eNB CellId " << cellid
        << ": completed handover of UE with IMSI " << imsi << " RNTI "
        << rnti);
}

void
CalculateThroughput ()
{
  std::ofstream outfile("throughput.out",  std::ofstream::app);

  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5;     /* Convert Application RX Packets to MBits. */
  outfile << now.GetSeconds () << " s : \t" << cur << " Mbit/s" << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);

  outfile.flush();
  outfile.close();
}


int
main (int argc, char *argv[])
{
  uint16_t numberOfUeNodes = 1; // number of mobile devices
  uint16_t numberOfEnbNodes = 4; // number of ENBs
  bool nodeMobilitySpeed = true; // gotta go fast
  uint16_t nodeSpeed = 20; // node's velocity
  double simTime = 150; // simulation time
  double distance = 1000.0; // distance between cells
  double interPacketInterval = 1000; // time between  packets

  uint32_t payloadSize = 1472;                       /* Transport layer payload size in bytes. */
  std::string dataRate = "100Mbps";                  /* Application layer datarate. */
  std::string tcpVariant = "TcpNewReno";             /* TCP variant type. */
  std::string phyRate = "HtMcs7";                    /* Physical layer bitrate. */

  uint16_t seed = 1000; // random seed
  std::string handoverAlg = "a2a4"; //handover algorithm

  uint16_t select = 0; // scheduler selection

  // Parse command line arguments
  CommandLine cmd;
  cmd.AddValue("seed", "Simulation Random Seed", seed);
  cmd.AddValue("handoverAlg", "Handover Algorithm Used", handoverAlg);
  cmd.AddValue("select","Select the scheduler type",select);
  cmd.Parse (argc, argv);

  // scheduler definition
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
   if (select == 0)
     {
     } 
   else if (select == 1)
     {
      lteHelper->SetSchedulerType("ns3::TdMtFfMacScheduler");
     }
   else if (select == 2)
     {
       lteHelper->SetSchedulerType("ns3::TtaFfMacScheduler");
     }
   else if (select == 3)
     {
       lteHelper->SetSchedulerType("ns3::TdBetFfMacScheduler");
     }
   else if (select == 4)
     {
       lteHelper->SetSchedulerType("ns3::TdTbfqFfMacScheduler");
     }
   else if (select == 5)
     {
	lteHelper->SetSchedulerType("ns3::PssFfMacScheduler");
     }
   else
     {
       NS_LOG_ERROR ("Invalid scheduler type.  Values must be [0-5], where 0=RR 1=MT ;2=TTA;3=BET;4=TBFQ;5=PSS");
     }

  // path loss model
  lteHelper->SetAttribute("PathlossModel",
       StringValue("ns3::FriisPropagationLossModel"));

  // set frequency bands
  Config::SetDefault ("ns3::LteUeNetDevice::DlEarfcn", UintegerValue (100));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (100));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (18100));
  
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (50));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (50));

  // Handover algorithm
  if (handoverAlg == "noop")
       lteHelper->SetHandoverAlgorithmType("ns3::NoOpHandoverAlgorithm");
  else if (handoverAlg == "a3") {
       lteHelper->SetHandoverAlgorithmType("ns3::A3RsrpHandoverAlgorithm");
       lteHelper->SetHandoverAlgorithmAttribute("Hysteresis", DoubleValue(3.0));
       lteHelper->SetHandoverAlgorithmAttribute("TimeToTrigger",
           TimeValue(MilliSeconds(256)));
  }

  else if (handoverAlg == "a2a4") {
      lteHelper->SetHandoverAlgorithmType("ns3::A2A4RsrqHandoverAlgorithm");
      lteHelper->SetHandoverAlgorithmAttribute("ServingCellThreshold",
          UintegerValue(30));
      lteHelper->SetHandoverAlgorithmAttribute("NeighbourCellOffset",
          UintegerValue(2));
  }

  // define EPC
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfEnbNodes);
  ueNodes.Create(numberOfUeNodes);

  // Install Mobility Model
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfEnbNodes; i++)
    {
      enbPositionAlloc->Add (Vector(distance * i, 0, 30));
    }
  for (uint16_t i = 0; i < numberOfUeNodes; i++)
    {
      uePositionAlloc->Add (Vector(distance * i, 0, 1));
    }
  MobilityHelper mobility;

  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(enbPositionAlloc);
  mobility.Install(enbNodes);

  mobility.SetPositionAllocator(uePositionAlloc);

  if (nodeMobilitySpeed){
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(ueNodes);
    for (NodeContainer::Iterator iter= ueNodes.Begin(); iter!=ueNodes.End(); ++iter){
      Ptr<Node> tmp_node = (*iter);
      tmp_node->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(nodeSpeed, 0, 0));
    }
  }
  else {
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(ueNodes);
  }

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  lteHelper->Attach(ueLteDevs);
  /* Install TCP Receiver on the access point */
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer sinkApp = sinkHelper.Install (ueNodes.Get(0));
  sink = StaticCast<PacketSink> (sinkApp.Get (0));

  /* Install TCP/UDP Transmitter on the station */
  OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (ueIpIface.GetAddress (0), 9)));
  server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  ApplicationContainer serverApp = server.Install (remoteHost);

  /* Start Applications */
  sinkApp.Start (Seconds (0.0));
  serverApp.Start (Seconds (1.0));
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      ++ulPort;
      ++otherPort;
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
      serverApps.Add (packetSinkHelper.Install (ueNodes.Get(u)));

      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

      UdpClientHelper client (ueIpIface.GetAddress (u), otherPort);
      client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      client.SetAttribute ("MaxPackets", UintegerValue(1000000));

      clientApps.Add (dlClient.Install (remoteHost));
      clientApps.Add (ulClient.Install (ueNodes.Get(u)));
      if (u+1 < ueNodes.GetN ())
        {
          clientApps.Add (client.Install (ueNodes.Get(u+1)));
        }
      else
        {
          clientApps.Add (client.Install (ueNodes.Get(0)));
        }
    }
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));

  lteHelper->EnableTraces ();
  lteHelper->AddX2Interface (enbNodes);
  //Uncomment to enable PCAP tracing
  p2ph.EnablePcapAll("lena-epc-first");

  AnimationInterface anim ("wireless-animation.xml"); // Mandatory

  for (uint32_t i = 0; i < ueNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (ueNodes.Get (i), "UE"); // Optional
      anim.UpdateNodeColor (ueNodes.Get (i), 255, 0, 0); // Optional
    }
  for (uint32_t i = 0; i < enbNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (enbNodes.Get (i), "ENB"); // Optional
      anim.UpdateNodeColor (enbNodes.Get (i), 0, 255, 0); // Optional
    }
  Ptr<FlowMonitor> flowmonitor;
  FlowMonitorHelper flowhelper;
  flowmonitor=flowhelper.InstallAll();

  /*--------------NOTIFICAÇÕES DE HANDOVER E SINAL-------------------------*/
   Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                  MakeCallback(&NotifyConnectionEstablishedEnb));
  Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
      MakeCallback(&NotifyConnectionEstablishedUe));
   Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                  MakeCallback(&NotifyHandoverStartEnb));
  Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
      MakeCallback(&NotifyHandoverStartUe));
   Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                  MakeCallback(&NotifyHandoverEndOkEnb));
  Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
        MakeCallback(&NotifyHandoverEndOkUe));

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  flowmonitor->SerializeToXmlFile("ads.xml",false,false);



  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy();
  return 0;

}
