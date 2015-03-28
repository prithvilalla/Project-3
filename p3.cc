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
#include "ns3/netanim-module.h"

#include <iostream>
#include <string>
#include <cmath>
#include <vector>

using namespace ns3;
using namespace std;

int main (int argc, char *argv[])
{
  
  uint32_t numNodes = 50;
  string dataRate = "1";
  uint32_t i;
  uint32_t port = 100;
  double startTime = 0.0;
  double stopTime = 2.5;
  uint32_t packetSize = 512;
  double txPower = 10000;
  
  CommandLine cmd;
  cmd.AddValue("numNodes","Number of nodes",numNodes);
  cmd.AddValue("dataRate","Data Rate in Mbps",dataRate);
  cmd.AddValue("txPower","Transmission Power in mW",txPower);
  cmd.Parse(argc,argv);
  
  dataRate = dataRate + "Mbps";
  txPower = 10*log10(txPower);
  
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue(dataRate));
  Config::SetDefault ("ns3::OnOffApplication::OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
  Config::SetDefault ("ns3::OnOffApplication::OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (packetSize));
  
  
  NodeContainer nodes;
  nodes.Create(numNodes);
  
  WifiHelper wifi;
  
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.Set("TxPowerStart",DoubleValue(txPower));
  wifiPhy.Set("TxPowerEnd",DoubleValue(txPower));
  
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
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
  
  RngSeedManager::SetSeed(123456);
  Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable>();
  U->SetAttribute ("Stream", IntegerValue (123));
  
  ApplicationContainer sourceApps;
  ApplicationContainer sinkApps;
  
  vector <uint32_t> j;
  for(i = 0;i<numNodes;i++)
    j.push_back(i);
  
  while(!j.empty())
  {
    U->SetAttribute ("Min", DoubleValue (1.0));
    U->SetAttribute ("Max", DoubleValue (j.size()-1));
    uint32_t peer = U->GetInteger();
    
    OnOffHelper source1 ("ns3::UdpSocketFactory",InetSocketAddress(interfaces.GetAddress(j[peer]),port+j[peer]));
    sourceApps.Add(source1.Install(NodeContainer(nodes.Get(j[0]))));
    
    PacketSinkHelper sink1("ns3::UdpSocketFactory",InetSocketAddress(interfaces.GetAddress(j[0]),port+j[0]));
    sinkApps.Add(sink1.Install(nodes.Get(j[0])));
    
    OnOffHelper source2 ("ns3::UdpSocketFactory",InetSocketAddress(interfaces.GetAddress(j[0]),port+j[0]));
    sourceApps.Add(source2.Install(NodeContainer(nodes.Get(j[peer]))));
    
    PacketSinkHelper sink2("ns3::UdpSocketFactory",InetSocketAddress(interfaces.GetAddress(j[peer]),port+j[peer]));
    sinkApps.Add(sink2.Install(nodes.Get(j[peer])));
    
    cout<<"Peer1 = "<<j[0]<<" Peer2 "<<j[peer]<<endl;
    
    j.erase(j.begin());
    j.erase(j.begin()+peer-1);
  }
  
  sourceApps.Start(Seconds(startTime));
  sourceApps.Stop(Seconds(stopTime));
  sinkApps.Start(Seconds(startTime));
  sinkApps.Stop(Seconds(stopTime));
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  
  AnimationInterface anim ("wireless-animation.xml");
  
  Simulator::Stop(Seconds(stopTime));
  Simulator::Run();
  Simulator::Destroy();
  
  double goodput;
  i = 0;
  cout<<"---------------------------------------------------"<<endl;
  for(ApplicationContainer::Iterator k = sinkApps.Begin();k != sinkApps.End();k++)
  {
    Ptr<PacketSink> temp = DynamicCast<PacketSink>(*k);
    double bytesRcvd = temp->GetTotalRx();
    goodput = bytesRcvd*8/1024/(stopTime-startTime);
    cout<<"Sink "<<" goodput "<<goodput<<" kbps"<<endl;
    i++;
  }
  return 0;
  
}
