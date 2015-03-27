/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-helper.h"
#include "ns3/olsr-helper.h"

using namespace ns3;
using namespace std;

int main (int argc, char *argv[])
{
  RngSeedManager::SetSeed(121277);
  Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable>();
  U->SetAttribute ("Stream", IntegerValue (111));
  //U->SetAttribute ("Min", DoubleValue (0.0));
  //U->SetAttribute ("Max", DoubleValue (0.1));
  
  uint32_t numNodes = 20;
  uint32_t i;
  uint32_t port1 = 1;
  uint32_t port2 = 5000;;
  double startTime = 0.0;
  double stopTime = 10.0;
  
  CommandLine cmd;
  cmd.AddValue("numNodes","Number of nodes",numNodes);
  cmd.Parse(argc,argv);
  
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("1Mbps"));
  
  NodeContainer nodes;
  nodes.Create(numNodes);
  
  WifiHelper wifi;
  
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.Set ("RxGain", DoubleValue (0) );
  
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (-80));
  wifiPhy.SetChannel (wifiChannel.Create ());
  
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  
  wifiMac.SetType ("ns3::AdhocWifiMac");
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue ("DsssRate1Mbps"),
                                "ControlMode",StringValue ("DsssRate1Mbps"));
  
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);
  
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"),
                                 "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);
  
  InternetStackHelper internet;
  AodvHelper aodv;
  internet.SetRoutingHelper(aodv);
  internet.Install (nodes);
  
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);
  
  ApplicationContainer sourceApps1;
  ApplicationContainer sourceApps2;
  ApplicationContainer sinkApps1;
  ApplicationContainer sinkApps2;
  
  for(i = 0;i < numNodes/2;i++)
  {
    OnOffHelper source1 ("ns3::UdpSocketFactory",InetSocketAddress(interfaces.GetAddress(i+(numNodes/2)),port2));
    source1.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
    source1.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=0.]"));
    //source1.SetAttribute ("DataRate", DataRateValue (dataRate));
    sourceApps1.Add(source1.Install(NodeContainer(nodes.Get(i))));
    
    PacketSinkHelper sink1("ns3::UdpSocketFactory",InetSocketAddress(interfaces.GetAddress(i),port1));
    sinkApps1.Add(sink1.Install(nodes.Get(i)));
    
    OnOffHelper source2 ("ns3::UdpSocketFactory",InetSocketAddress(interfaces.GetAddress(i),port1));
    source2.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
    source2.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=0.]"));
    //source2.SetAttribute ("DataRate", DataRateValue (dataRate));
    sourceApps2.Add(source2.Install(NodeContainer(nodes.Get(i+(numNodes/2)))));
    
    PacketSinkHelper sink2("ns3::UdpSocketFactory",InetSocketAddress(interfaces.GetAddress(i+(numNodes/2)),port2));
    sinkApps2.Add(sink2.Install(nodes.Get(i+(numNodes/2))));
    
  }
  
  sourceApps1.Start(Seconds(startTime));
  sourceApps1.Stop(Seconds(stopTime));
  sourceApps2.Start(Seconds(startTime));
  sourceApps2.Stop(Seconds(stopTime));
    
  sinkApps1.Start(Seconds(startTime));
  sinkApps1.Stop(Seconds(stopTime));
  sinkApps2.Start(Seconds(startTime));
  sinkApps2.Stop(Seconds(stopTime));
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  
  Simulator::Stop(Seconds(stopTime));
  Simulator::Run();
  Simulator::Destroy();
  
  double goodput;
  i = 0;
  cout<<"---------------------------------------------------"<<endl;
  for(ApplicationContainer::Iterator k = sinkApps1.Begin();k != sinkApps1.End();k++)
  {
    Ptr<PacketSink> temp = DynamicCast<PacketSink>(*k);
    double bytesRcvd = temp->GetTotalRx();
    goodput = bytesRcvd*8/1024/(stopTime-startTime);
    cout<<"Sink "<<i<<" goodput "<<goodput<<" kbps"<<endl;
    i++;
  }
  for(ApplicationContainer::Iterator k = sinkApps2.Begin();k != sinkApps2.End();k++)
  {
    Ptr<PacketSink> temp = DynamicCast<PacketSink>(*k);
    double bytesRcvd = temp->GetTotalRx();
    goodput = bytesRcvd*8/1024/(stopTime-startTime);
    cout<<"Sink "<<i<<" goodput "<<goodput<<" kbps"<<endl;
    i++;
  }
  return 0;
  
}
