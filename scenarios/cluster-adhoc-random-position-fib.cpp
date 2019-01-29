#include "../extensions/cluster-library.hpp"

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

#include <iostream>
#include <fstream>
#include <vector>
#include <string>


using namespace std;

namespace ns3 {
int
    main(int argc, char* argv[])
    {
        double frequency = 10.0;
        int propogationRange = 100;
        double simTime = 10.0;

        int consumerCluster1 = 6;
        int consumerCluster2 = 6;
        int consumerCluster3 = 6;
        int consumerCluster4 = 6;
        int totalConsumer = consumerCluster1 + consumerCluster2 + consumerCluster3 + consumerCluster4;


        string animFile = "cluster.xml";

        CommandLine cmd;
        cmd.AddValue ("animFile", "File Name for Animation Output", animFile);
        cmd.Parse (argc, argv);

        ////// string name
        string prefix = "/prefix";

        ////// Reading file for topology setup
        AnnotatedTopologyReader topologyReader("", 1);
        topologyReader.SetFileName("scenarios/cluster-topo.txt");
        topologyReader.Read();

        ////// Getting containers for the producer/wifi-ap
        NodeContainer produserNodes;
        produserNodes.Add(Names::Find<Node>("prod"));

        NodeContainer wifiAPNodes;
        wifiAPNodes.Add(Names::Find<Node>("ap1"));
        wifiAPNodes.Add(Names::Find<Node>("ap2"));
        wifiAPNodes.Add(Names::Find<Node>("ap3"));
        wifiAPNodes.Add(Names::Find<Node>("ap4"));

        NodeContainer routerNodes;
        routerNodes. Add(Names::Find<Node>("rtr1"));
        routerNodes. Add(Names::Find<Node>("rtr2"));
        routerNodes. Add(Names::Find<Node>("rtr3"));

        NodeContainer Cluster1;
        Cluster1. Create(consumerCluster1);
        NodeContainer Cluster2;
        Cluster2. Create(consumerCluster2);
        NodeContainer Cluster3;
        Cluster3. Create(consumerCluster3);
        NodeContainer Cluster4;
        Cluster4. Create(consumerCluster4);

        NodeContainer consumersNodes;
        for (int i = 0; i < consumerCluster1; ++i) {
            consumersNodes. Add(Cluster1.Get(i));
        };
        for (int i = 0; i < consumerCluster2; ++i) {
            consumersNodes. Add(Cluster2.Get(i));
        };
        for (int i = 0; i < consumerCluster3; ++i) {
            consumersNodes. Add(Cluster3.Get(i));
        };
        for (int i = 0; i < consumerCluster4; ++i) {
            consumersNodes.Add (Cluster4.Get (i));
        };

        Cluster cluster;
        cluster.installWifiAdHoc (consumersNodes, wifiAPNodes, propogationRange );
        cluster.setPositionRandomMobilityRandom (-300, 500, 80, 120, consumersNodes, 20);
        cluster.installndn (false);
        cluster.installGlobalRouting (prefix, consumersNodes, produserNodes);
        cluster.installGlobalRouting (prefix, wifiAPNodes, produserNodes);
        cluster.installProducer (produserNodes, prefix);
        cluster.installConsumer (consumersNodes, prefix, frequency);
        cluster.installMulticastStrategy (prefix);

        // Manually configure FIB routes
        ndn::FibHelper::AddRoute(produserNodes.Get(0), prefix, routerNodes.Get(2), 10);
        ndn::FibHelper::AddRoute(routerNodes.Get(2), prefix, produserNodes.Get(0), 10);
        ndn::FibHelper::AddRoute(routerNodes.Get(0), prefix, routerNodes.Get(2), 10);
        ndn::FibHelper::AddRoute(routerNodes.Get(2), prefix, routerNodes.Get(0), 10);
        ndn::FibHelper::AddRoute(routerNodes.Get(1), prefix, routerNodes.Get(2), 10);
        ndn::FibHelper::AddRoute(routerNodes.Get(2), prefix, routerNodes.Get(1), 10);

        //Simulator::Stop (Seconds (10.0));

        Simulator::Stop (Seconds (simTime));

        AnimationInterface anim (animFile);

        for (int i=0; i<totalConsumer ; i++) {
            string str = "cluster" + std::to_string(i+1) + ".txt";
            ndn::L3RateTracer::Install(consumersNodes.Get(i), str, Seconds(simTime-0.5));
        }

        Simulator::Run();
        Simulator::Destroy();

        return 0;
    }

} // namespace ns3

int
main(int argc, char* argv[])
{
    return ns3::main(argc, argv);
}
