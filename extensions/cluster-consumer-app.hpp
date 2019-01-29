#ifndef CLUSTERCONSUMER_APP_H_
#define CLUSTERCONSUMER_APP_H_

#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/apps/ndn-consumer.hpp"


namespace ns3 {

class ClusterConsumerApp : public ndn::Consumer {
    public:
        static TypeId
        GetTypeId();

    ClusterConsumerApp();
        virtual ~ClusterConsumerApp ();

    protected:
        virtual void
        ScheduleNextPacket();

    protected:
        double m_frequency; // Frequency of interest packets (in hertz)
        bool m_firstTime;
        Ptr<RandomVariableStream> m_random;
        std::string m_randomType;

    private:

    };

} // namespace ns3

#endif // CLUSTERCONSUMER_APP_H_
