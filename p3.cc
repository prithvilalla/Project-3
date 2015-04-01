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
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/netanim-module.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <cmath>
#include <vector>
#include <fstream>

using namespace ns3;
using namespace std;

#define channel_capacity 11
#define duty_cycle 0.5

uint32_t total = 0;

void trace(Ptr<const Packet> dataPacket)
{
  total++;
}

int main (int argc, char *argv[])
{

  Time::SetResolution(Time::US);
  
  uint32_t numNodes = 50;
  uint32_t i;
  uint32_t port = 100;
  double startTime = 0.0;
  double stopTime = 5.0;
  uint32_t packetSize = 512;
  double txPower = 250;
  string routingProtocol = "olsr";
  double trafficIntensity = 0.9;
  uint32_t seed = 123;
  
  CommandLine cmd;
  cmd.AddValue("numNodes","Number of nodes",numNodes);
  cmd.AddValue("txPower","Transmission Power in mW",txPower);
  cmd.AddValue("routingProtocol","The Routing Protocol",routingProtocol);
  cmd.AddValue("trafficIntensity","The Traffic Intensity",trafficIntensity);
  cmd.AddValue("seed","Seed",seed);
  cmd.Parse(argc,argv);
  
  double rate = trafficIntensity * channel_capacity / numNodes / duty_cycle;
  stringstream ss;
  ss<<rate;
  string dataRate = ss.str() + "Mbps";
  
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue(dataRate));
  Config::SetDefault ("ns3::OnOffApplication::OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"));
  Config::SetDefault ("ns3::OnOffApplication::OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"));
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (packetSize));
  
  
  NodeContainer nodes;
  nodes.Create(numNodes);
  
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.Set("TxPowerStart",DoubleValue(10*log10(txPower)));
  wifiPhy.Set("TxPowerEnd",DoubleValue(10*log10(txPower)));
  
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
  wifiPhy.SetChannel (wifiChannel.Create ());
  
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  
  wifiMac.SetType ("ns3::AdhocWifiMac");
  
  WifiHelper wifi;
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
  if(routingProtocol == "aodv")
  {
    AodvHelper aodv;
    internet.SetRoutingHelper(aodv);
    cout<<"Routing Protocol = AODV"<<endl;
  }
  else
  {
    OlsrHelper olsr;
    internet.SetRoutingHelper(olsr);
    cout<<"Routing Protocol = OLSR"<<endl;
  }
  internet.Install (nodes);
  
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);
  
  RngSeedManager::SetSeed(seed);
  
  Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable>();
  U->SetAttribute ("Stream", IntegerValue (123));
  
  Ptr<UniformRandomVariable> V = CreateObject<UniformRandomVariable>();
  V->SetAttribute ("Stream", IntegerValue (456));
  V->SetAttribute ("Min", DoubleValue (0.0));
  V->SetAttribute ("Max", DoubleValue (0.1));
  
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
    ApplicationContainer temp1 = source1.Install(NodeContainer(nodes.Get(j[0])));
    temp1.Start((Seconds(V->GetValue())));
    sourceApps.Add(temp1);
    
    PacketSinkHelper sink1("ns3::UdpSocketFactory",InetSocketAddress(interfaces.GetAddress(j[0]),port+j[0]));
    sinkApps.Add(sink1.Install(nodes.Get(j[0])));
    
    OnOffHelper source2 ("ns3::UdpSocketFactory",InetSocketAddress(interfaces.GetAddress(j[0]),port+j[0]));
    ApplicationContainer temp2 = source2.Install(NodeContainer(nodes.Get(j[peer])));
    temp2.Start((Seconds(V->GetValue())));
    sourceApps.Add(temp2);
    
    PacketSinkHelper sink2("ns3::UdpSocketFactory",InetSocketAddress(interfaces.GetAddress(j[peer]),port+j[peer]));
    sinkApps.Add(sink2.Install(nodes.Get(j[peer])));
    
    j.erase(j.begin());
    j.erase(j.begin()+peer-1);
  }
  
  sourceApps.Stop(Seconds(stopTime));
  sinkApps.Start(Seconds(startTime));
  sinkApps.Stop(Seconds(stopTime));
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  
  //AnimationInterface anim ("wireless-animation.xml");
  
  for(ApplicationContainer::Iterator k = sourceApps.Begin();k != sourceApps.End();k++)
  {
    Ptr<OnOffApplication> temp = DynamicCast<OnOffApplication>(*k);
    temp->TraceConnectWithoutContext("Tx", MakeCallback(&trace));
  }
  
  Simulator::Stop(Seconds(stopTime));
  Simulator::Run();
  Simulator::Destroy();
  
  cout<<"numNodes = "<<numNodes<<endl;
  cout<<"txPower = "<<txPower<<endl;
  cout<<"routingProtocol = "<<routingProtocol<<endl;
  cout<<"trafficIntensity = "<<trafficIntensity<<endl;
  cout<<"dataRate = "<<dataRate<<endl;
  
  double rcvd = 0;
  cout<<"---------------------------------------------------"<<endl;
  for(ApplicationContainer::Iterator k = sinkApps.Begin();k != sinkApps.End();k++)
  {
    Ptr<PacketSink> temp = DynamicCast<PacketSink>(*k);
    rcvd += temp->GetTotalRx();
  }
  rcvd /= 1024;
  cout<<"Total Received Data = "<<rcvd<<" kB"<<endl;
  
  double tx = total * 512 /1024;
  cout<<"---------------------------------------------------"<<endl;
  cout<<"Number of Packets Send = "<<total<<endl;
  cout<<"---------------------------------------------------"<<endl;
  cout<<"Total Transmitted Data = "<<tx<<" kB"<<endl;
  cout<<"---------------------------------------------------"<<endl;
  cout<<"Efficiency = "<<rcvd/tx<<endl;
  cout<<"---------------------------------------------------"<<endl;
  ofstream output("p3-output.txt", ios::app);
  output<<rcvd/tx<<" ";
  output.close();

  return 0; 
}
