#ifndef CLUSTER_LIBRARY_HPP
#define CLUSTER_LIBRARY_HPP

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ptr.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>


namespace ns3{
    class Node;
    class NodeContainer;
    class Channel;

    class Cluster {
        public:
            ////// Install NDN Stack
            void installndn(bool route);

            /////// Install Global ROuting All Node
            void installGlobalRoutingAll (const std::string prefix, const NodeContainer &node);

            ////// Install Global Routing
            void installGlobalRouting (const std::string prefix, const NodeContainer &node, const NodeContainer &prodnode);

            ////// Install Wifi App Adhoc
            void installWifiAdHoc (const NodeContainer &consumernode, const NodeContainer &apnode, const double wifirange);

            ////// Install Wifi App Sta & Ap
            void installWifiStaAp(const NodeContainer &consumernode, const NodeContainer &apnode, const double wifirange);

            ////// Install Wifi Infrasructure
            void installWifiInsfrastructure(const NodeContainer &consumernode, const NodeContainer &apnode, const double wifirange);

            ////// Install Produsen Application
            void installProducer (const NodeContainer &node, const std::string prefix);

            ////// Install Consumer Application
            void installConsumer (const NodeContainer &node, const std::string prefix, const double frequency);

            ////// Install ClusterHead Application
            void installClusterHead (const NodeContainer &node, const std::string prefix);

            ////// Install ClusterConsumer Application
            void installClusterConsumer (const NodeContainer &node, const std::string prefix, const double frequency);

            ////// Install Cluster Strategy
            void installClusterStrategy (const std::string prefix);

            ////// Install Cluster Member Strategy
            void installClusterMemberStrategy (const std::string prefix);

            ////// Install Best Route Strategy
            void installBestRouteStrategy (const std::string prefix);

            ////// Install Mulsticast Strategy
            void installMulticastStrategy (const std::string prefix);

            ////// Install Broadcast Strategy
            void installBroadcastStrategy (const std::string prefix);

            ////// Install Position Random Mobility Static
            void setPositionRandomMobilityStatic (const double xmin, const double xmax, const NodeContainer &node);

            ////// Install Position Random Mobility Random
            void setPositionRandomMobilityRandom (const double xmin, const double xmax, const double ymin, const double ymax, const NodeContainer &node, const int speed);

            ///// Install Position Static Mobility Linear
            void setPositionStaticMobilityLinear(const NodeContainer &node, const int totalConsumer, int startPosition, const int speed);

            ////// Install Position Static Mobile Static
            void setPositionStaticMobileStatic(const double x, const double y, const double z, const NodeContainer &node);
            ///// Install NRT
            //void addNRT (const NodeContainer &node, const std::string prefix);

        private:
        };

}

#endif