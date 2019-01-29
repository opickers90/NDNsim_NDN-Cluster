#include "cluster-consumer-app.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/ndnSIM/ndn-cxx/link.hpp"
#include "ns3/ndnSIM/ndn-cxx/interest.hpp"

#include "ns3/ndnSIM/apps/ndn-consumer.hpp"


NS_LOG_COMPONENT_DEFINE("ClusterConsumerApp");

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(ClusterConsumerApp);

// register NS-3 type
    TypeId
    ClusterConsumerApp::GetTypeId(void)
    {
      static TypeId tid = TypeId("ClusterConsumerApp").
              SetGroupName("Ndn").
              SetParent<Consumer>()
              .AddConstructor<ClusterConsumerApp>()

              .AddAttribute("Frequency", "Frequency of interest packets", StringValue("1.0"),
                            MakeDoubleAccessor(&ClusterConsumerApp::m_frequency), MakeDoubleChecker<double>())
      ;
        return tid;
    }

    ClusterConsumerApp::ClusterConsumerApp ()
            : m_frequency(1.0)
            , m_firstTime(true)
    {
        NS_LOG_FUNCTION_NOARGS();
        m_seqMax = std::numeric_limits<uint32_t>::max();
    }

    ClusterConsumerApp::~ClusterConsumerApp ()
    {
    }

    void
    ClusterConsumerApp::ScheduleNextPacket()
    {
        // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
        // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";

        if (m_firstTime) {
            m_sendEvent = Simulator::Schedule(Seconds(0.0), &Consumer::SendPacket, this);
            m_firstTime = false;
        }
        else if (!m_sendEvent.IsRunning())
            m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                              : Seconds(m_random->GetValue()),
                                              &Consumer::SendPacket, this);
    }

} // namespace ns3
