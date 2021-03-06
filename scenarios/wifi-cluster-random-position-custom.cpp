#include "../extensions/cluster-library.hpp"
#include "../extensions/cluster-member-strategy.hpp"
#include "../extensions/cluster-header-app.hpp"
#include "../extensions/cluster-member-app.hpp"
#include "../extensions/cluster-consumer-app.hpp"

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
        int propogationRange = 200;
        double simTime = 10.0;

        int accesspoint = 4;
        int consumer = 16;

        string animFile = "cluster.xml";

        CommandLine cmd;
        cmd.AddValue ("animFile", "File Name for Animation Output", animFile);
        cmd.Parse (argc, argv);

        ////// string name
        string prefix = "/prefix";

        NodeContainer clusterNodes;
        clusterNodes. Create(accesspoint);

        NodeContainer consumerNodes;
        consumerNodes. Create(consumer);

        Cluster cluster;
        //cluster.installWifiAdHoc (consumerNodes, clusterNodes, propogationRange );
        cluster.installWifiStaAp (consumerNodes,clusterNodes,propogationRange);
        //cluster.installWifiInsfrastructure (consumerNodes, clusterNodes, propogationRange);
        cluster.setPositionStaticMobileStatic (200.0, 200.0, 0.0, clusterNodes.Get(0));
        cluster.setPositionStaticMobileStatic (200.0, -200.0, 0.0, clusterNodes.Get(1));
        cluster.setPositionStaticMobileStatic (-200.0, 200.0, 0.0, clusterNodes.Get(2));
        cluster.setPositionStaticMobileStatic (-200.0, -200.0, 0.0, clusterNodes.Get(3));
        cluster.setPositionRandomMobilityRandom (-180, 180, -180, 180, consumerNodes, 20);
        ndn::StackHelper ndnHelper;
        ndnHelper.InstallAll ();
        cluster.installGlobalRoutingAll (prefix, clusterNodes);
        cluster.installProducer (clusterNodes, prefix);
        //cluster.installConsumer (consumerNodes, prefix, frequency);
        //cluster.installClusterHead (clusterNodes, prefix);
        cluster.installClusterConsumer (consumerNodes, prefix, frequency);

        //cluster.installMulticastStrategy (prefix);
        cluster.installBestRouteStrategy (prefix);
        //cluster.installBroadcastStrategy (prefix);
        //cluster.installClusterStrategy (prefix);
        //ndn::StrategyChoiceHelper::InstallAll (prefix, "/localhost/nfd/strategy/client-control");
        //ndn::StrategyChoiceHelper::Install (clusterNodes, prefix, "/localhost/nfd/strategy/broadcast");
        //ndn::StrategyChoiceHelper::InstallAll (prefix, "/localhost/nfd/strategy/best-route/%FD%05");
        //ndn::StrategyChoiceHelper::InstallAll (prefix, "/localhost/nfd/strategy/access");

        Simulator::Stop (Seconds (simTime));

        AnimationInterface anim (animFile);

        for (int i=0; i<consumer ; i++) {
            string str = "/home/taufik/Workspace/ns-dev/cluster-scenario-final/results/l3-tracer" + std::to_string(i+1) + ".txt";
            ndn::L3RateTracer::Install(consumerNodes.Get(i), str, Seconds(simTime-0.5));
        }

        //ndn::CsTracer::InstallAll("/home/taufik/Workspace/ns-dev/cluster-scenario-final/results/cs-trace.txt", Seconds(1));
        L2RateTracer::InstallAll("/home/taufik/Workspace/ns-dev/cluster-scenario-final/results/drop-trace.txt", Seconds(0.5));
        ndn::AppDelayTracer::InstallAll("/home/taufik/Workspace/ns-dev/cluster-scenario-final/results/app-delays-trace.txt");

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
