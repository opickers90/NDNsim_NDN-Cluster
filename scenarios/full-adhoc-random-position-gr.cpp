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
        int propogationRange = 105;
        double simTime = 10.0;

        int produsen = 4;
        int consumer = 16;

        string animFile = "cluster.xml";

        CommandLine cmd;
        cmd.AddValue ("animFile", "File Name for Animation Output", animFile);
        cmd.Parse (argc, argv);

        ////// string name
        string prefix = "/prefix";

        NodeContainer produsenNodes;
        produsenNodes. Create(produsen);

        NodeContainer consumerNodes;
        consumerNodes. Create(consumer);

        Cluster cluster;
        cluster.installWifiAdHoc (consumerNodes, produsenNodes, propogationRange );
        cluster.setPositionStaticMobileStatic (100.0, 100.0, 0.0, produsenNodes.Get(0));
        cluster.setPositionStaticMobileStatic (100.0, -100.0, 0.0, produsenNodes.Get(1));
        cluster.setPositionStaticMobileStatic (-100.0, 100.0, 0.0, produsenNodes.Get(2));
        cluster.setPositionStaticMobileStatic (-100.0, -100.0, 0.0, produsenNodes.Get(3));
        cluster.setPositionRandomMobilityRandom (-100, 100, -100, 100, consumerNodes, 20);
        cluster.installndn (false);
        cluster.installGlobalRoutingAll (prefix, produsenNodes);
        cluster.installProducer (produsenNodes, prefix);
        cluster.installConsumer (consumerNodes, prefix, frequency);
        cluster.installBestRouteStrategy (prefix);

        Simulator::Stop (Seconds (simTime));

        AnimationInterface anim (animFile);

        for (int i=0; i<consumer ; i++) {
            string str = "cluster" + std::to_string(i+1) + ".txt";
            ndn::L3RateTracer::Install(consumerNodes.Get(i), str, Seconds(simTime-0.5));
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
