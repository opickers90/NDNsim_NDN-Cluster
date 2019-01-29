//
// Created by taufik on 11/15/18.
//

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#endif

#include "cluster-library.hpp"
#include "cluster-member-strategy.hpp"
#include "cluster-header-app.hpp"
#include "cluster-member-app.hpp"
#include "cluster-consumer-app.hpp"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-velocity-mobility-model.h"

#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/net-device.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/names.h"
#include "ns3/node-list.h"
#include "ns3/channel-list.h"
#include "ns3/object-factory.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/concept/assert.hpp>

#include <unordered_map>

#include <math.h>

namespace ns3{

        ////// Install NDN Stack
        void
        Cluster::installndn (bool route) {
            ndn::StackHelper ndnHelper;
            //ndnHelper.SetOldContentStore ("ns3::ndn::cs::Lru", "MaxSize", "20");
            ndnHelper.SetDefaultRoutes (route);
            ndnHelper.InstallAll ();
        };

        /////// Install Global ROuting All
        void
        Cluster::installGlobalRoutingAll (const std::string prefix, const NodeContainer &node) {
            ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
            ndnGlobalRoutingHelper.InstallAll ();
            ndnGlobalRoutingHelper.AddOrigins (prefix, node);
            ndn::GlobalRoutingHelper::CalculateRoutes();
        };

        /////// Install Global ROuting
        void
        Cluster::installGlobalRouting (const std::string prefix, const NodeContainer &node, const NodeContainer &prodnode) {
            ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
            ndnGlobalRoutingHelper.Install(node);
            ndnGlobalRoutingHelper.AddOrigins (prefix, prodnode);
            ndn::GlobalRoutingHelper::CalculateRoutes();
        };

       ////// Install Wifi App Adhoc
        void
        Cluster::installWifiAdHoc (const NodeContainer &consumernode, const NodeContainer &apnode, const double wifirange) {
            WifiHelper wifi;
            wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
            wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                         "DataMode", StringValue ("DsssRate1Mbps"),
                                         "ControlMode", StringValue ("DsssRate1Mbps"));

            YansWifiChannelHelper wifiChannel;
            wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel",
                                           "Speed", DoubleValue(299792458));
            wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel",
                                           "MaxRange", DoubleValue(wifirange));

            YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
            wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
            wifiPhy.Set ("TxPowerStart",DoubleValue (4.0));
            wifiPhy.Set ("TxPowerEnd", DoubleValue (4.0));
            wifiPhy.SetChannel (wifiChannel.Create ());

            WifiMacHelper wifiMac;
            wifiMac.SetType ("ns3::AdhocWifiMac");
            NetDeviceContainer consDevices = wifi.Install (wifiPhy, wifiMac, consumernode);
            NetDeviceContainer apDevices = wifi.Install (wifiPhy, wifiMac, apnode);
            NetDeviceContainer wifiDevice;
            wifiDevice.Add (consDevices);
            wifiDevice.Add (apDevices);

            wifiPhy.EnablePcap ("cluster-adhoc", wifiDevice);
        };

    ////// Install Wifi App Sta and App
    void
    Cluster::installWifiStaAp(const NodeContainer &consumernode, const NodeContainer &apnode, const double wifirange)
    {
        WifiHelper wifi;
        wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
        wifi.SetRemoteStationManager ("ns3::CaraWifiManager",
                                      "ProbeThreshold", UintegerValue (1),
                                      "FailureThreshold", UintegerValue (2),
                                      "SuccessThreshold", UintegerValue (10),
                                      "Timeout", UintegerValue (15));

        YansWifiChannelHelper wifiChannel;
        wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel",
                                        "Speed", DoubleValue(299792458));
        wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel",
                                        "MaxRange", DoubleValue(wifirange));

        YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
        wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
        wifiPhy.Set ("TxPowerStart",DoubleValue (4.0));
        wifiPhy.Set ("TxPowerEnd", DoubleValue (4.0));
        wifiPhy.SetChannel (wifiChannel.Create ());

        Ssid ssid = Ssid ("wifi-default");

        WifiMacHelper wifiMacConsumer;
        wifiMacConsumer.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid),
                                 "ActiveProbing", BooleanValue (true),
                                 "ProbeRequestTimeout", TimeValue(Seconds(0.25)),
                                 "MaxMissedBeacons", UintegerValue (10),
                                 "AssocRequestTimeout", TimeValue(Seconds(10))
                                 );

        WifiMacHelper wifiMacAP;
        wifiMacAP.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid),
                           "BeaconInterval", TimeValue (MicroSeconds (102400)),
                           "BeaconJitter",  StringValue ("ns3::UniformRandomVariable"),
                           "BeaconGeneration", BooleanValue(true),
                           "EnableNonErpProtection", BooleanValue(true),
                           "RifsMode", BooleanValue(true)
                           );

        NetDeviceContainer consDevices = wifi.Install (wifiPhy, wifiMacConsumer, consumernode);
        NetDeviceContainer apDevices = wifi.Install (wifiPhy, wifiMacAP, apnode);
        NetDeviceContainer wifiDevice;
        wifiDevice.Add (consDevices);
        wifiDevice.Add (apDevices);

        wifiPhy.EnablePcap ("cluster-staap", wifiDevice);
       };

        void
        Cluster::installWifiInsfrastructure(const NodeContainer &consumernode, const NodeContainer &apnode, const double wifirange)
        {
            WifiHelper wifi;
            wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
            wifi.SetRemoteStationManager ("ns3::CaraWifiManager",
                                          "ProbeThreshold", UintegerValue (1),
                                          "FailureThreshold", UintegerValue (2),
                                          "SuccessThreshold", UintegerValue (10),
                                          "Timeout", UintegerValue (15));

            YansWifiChannelHelper wifiChannel;
            wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel",
                                            "Speed", DoubleValue(299792458));
            wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel",
                                            "MaxRange", DoubleValue(wifirange));

            YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
            wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
            wifiPhy.Set ("TxPowerStart",DoubleValue (4.0));
            wifiPhy.Set ("TxPowerEnd", DoubleValue (4.0));
            wifiPhy.SetChannel (wifiChannel.Create ());

            Ssid ssid = Ssid ("wifi-default");

            WifiMacHelper wifiMac;
            wifiMac.SetType ("ns3::InfrastructureWifiMac", "Ssid", SsidValue (ssid),
                                     "PcfSupported", BooleanValue (true));

            NetDeviceContainer consDevices = wifi.Install (wifiPhy, wifiMac, consumernode);
            NetDeviceContainer apDevices = wifi.Install (wifiPhy, wifiMac, apnode);
            NetDeviceContainer wifiDevice;
            wifiDevice.Add (consDevices);
            wifiDevice.Add (apDevices);

           wifiPhy.EnablePcap ("cluster-infra", wifiDevice);
        };

        ////// Install Produsen Application
        void
        Cluster::installProducer (const NodeContainer &node, const std::string prefix) {
            ndn::AppHelper producerHelper ("ns3::ndn::Producer");
            producerHelper.SetPrefix (prefix);
            producerHelper.SetAttribute ("PayloadSize", StringValue ("1024"));
            producerHelper.Install (node);
        };

        ////// Install Consumer Application
        void
        Cluster::installConsumer (const NodeContainer &node, const std::string prefix, const double frequency ) {
            ndn::AppHelper helper ("ns3::ndn::ConsumerCbr");
            helper.SetAttribute ("Frequency", DoubleValue (frequency));
            helper.SetPrefix (prefix);
            helper.Install (node);
        };


        ////// Install Cluster Header Application
        void
        Cluster::installClusterHead (const ns3::NodeContainer &node, const std::string prefix) {
            ndn::AppHelper producerHelper ("ClusterHeadApp");
            producerHelper.SetPrefix (prefix);
            producerHelper.Install (node);
        };

        ////// Install Cluster Consumer Application
        void
        Cluster::installClusterConsumer (const NodeContainer &node, const std::string prefix, const double frequency ) {
            ndn::AppHelper helper ("ClusterConsumerApp");
            helper.SetAttribute ("Frequency", DoubleValue (frequency));
            helper.SetPrefix (prefix);
            helper.Install (node);
        };

        ////// Install Cluster Strategy
        void
        Cluster::installClusterStrategy (const std::string prefix) {
            ndn::StrategyChoiceHelper::InstallAll<nfd::fw::ClusterStrategy>(prefix);
        };


        void
        Cluster::installClusterMemberStrategy (const std::string prefix) {
            ndn::StrategyChoiceHelper::InstallAll (prefix, "/localhost/nfd/strategy/cluster-member/");
        };

        ////// Install Best Route Strategy
        void
        Cluster::installBestRouteStrategy (const std::string prefix) {
            ndn::StrategyChoiceHelper::InstallAll (prefix, "/localhost/nfd/strategy/best-route");
        };

        ////// Install Multicast Strategy
        void
        Cluster::installMulticastStrategy (const std::string prefix) {
        ndn::StrategyChoiceHelper::InstallAll (prefix, "/localhost/nfd/strategy/multicast");
        };

        ////// Install Broadcast Strategy
        void
        Cluster::installBroadcastStrategy (const std::string prefix) {
        ndn::StrategyChoiceHelper::InstallAll (prefix, "/localhost/nfd/strategy/broadcast");
        };


        ////// Install Constant Position Static Mobility
        void
        Cluster::setPositionRandomMobilityStatic (const double xmin, const double xmax, const ns3::NodeContainer &node) {
            Ptr <UniformRandomVariable> randomizer = CreateObject<UniformRandomVariable> ();
            randomizer->SetAttribute ("Min", DoubleValue (xmin));
            randomizer->SetAttribute ("Max", DoubleValue (xmax));

            MobilityHelper mobility;
            mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator", "X", PointerValue (randomizer),
                                       "Y", PointerValue (randomizer), "Z", PointerValue (randomizer));

            mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

            AsciiTraceHelper ascii;
            MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("PositionRandomMobilityStatic.mob"));

            mobility.Install(node);
        };


        ////// Install Random Position Static Mobility
        void
        Cluster::setPositionRandomMobilityRandom (const double xmin, const double xmax, const double ymin, const double ymax, const NodeContainer &node, const int speed) {
            Ptr <UniformRandomVariable> randomizerx = CreateObject<UniformRandomVariable> ();
            randomizerx->SetAttribute ("Min", DoubleValue (xmin));
            randomizerx->SetAttribute ("Max", DoubleValue (xmax));
            Ptr <UniformRandomVariable> randomizery = CreateObject<UniformRandomVariable> ();
            randomizery->SetAttribute ("Min", DoubleValue (ymin));
            randomizery->SetAttribute ("Max", DoubleValue (ymax));
            MobilityHelper mobility;
            mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator", "X", PointerValue (randomizerx),
                                           "Y", PointerValue (randomizery));
            mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                                       "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=" + std::to_string (speed) + "]"),
                                       "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"),
                                       "Bounds", RectangleValue (RectangleValue (Rectangle (xmin, xmax, ymin, ymax))));

            AsciiTraceHelper ascii;
            MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("PositionRandomMobilityRandom.mob"));

            mobility.Install (node);
        };

        ///// Install Linear Position and Mobility
        void
        Cluster::setPositionStaticMobilityLinear(const NodeContainer &node, const int totalConsumer, int startPosition, const int speed) {
            MobilityHelper mobile;
            mobile.SetMobilityModel("ns3::ConstantVelocityMobilityModel");

            AsciiTraceHelper ascii;
            MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("PositionStaticMobilityLinear.mob"));

            mobile.Install(node);

         for (int i=0; i<totalConsumer ; i++) {
                Ptr<ConstantVelocityMobilityModel> cvmm = node.Get(i)->GetObject<ConstantVelocityMobilityModel> ();
             Vector pos (0-startPosition, 20, 0);
             Vector vel (speed, 0, 0);
             cvmm->SetPosition(pos);
             cvmm->SetVelocity(vel);
             startPosition += 100;
            }
        };

        void
        Cluster::setPositionStaticMobileStatic(const double x, const double y, const double z, const NodeContainer &node){
            MobilityHelper mobility;
            mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            Ptr<ListPositionAllocator> nodePosition = CreateObject<ListPositionAllocator> ();
            nodePosition = CreateObject<ListPositionAllocator> ();
            nodePosition->Add (Vector (x, y, z));
            mobility.SetPositionAllocator(nodePosition);

            AsciiTraceHelper ascii;
            MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("PositionStaticMobileStatic.mob"));

            mobility.Install (node);
        };

}
