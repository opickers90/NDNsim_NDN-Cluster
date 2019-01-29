#include "cluster-header-app.hpp"

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

#include "ns3/core-module.h"

#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/ndn-cxx/link.hpp"
#include "ns3/ndnSIM/ndn-cxx/interest.hpp"

#include <memory>

NS_LOG_COMPONENT_DEFINE("ClusterHeadApp");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(ClusterHeadApp);

// register NS-3 type
TypeId
ClusterHeadApp::GetTypeId(void)
{
  static TypeId tid = TypeId("ClusterHeadApp")
          .SetGroupName("Ndn")
          .SetParent<App>()
          .AddConstructor<ClusterHeadApp>()
          .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                        MakeNameAccessor(&ClusterHeadApp::m_prefix), ndn::MakeNameChecker())
          ;
  return tid;
}

ClusterHeadApp::ClusterHeadApp(){
    NS_LOG_FUNCTION_NOARGS();
}

    void
    ClusterHeadApp::StartApplication()
    {
       NS_LOG_FUNCTION_NOARGS();
       ndn::App::StartApplication();
       ndn::FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
    }

    void
    ClusterHeadApp::StopApplication()
    {
        ndn::App::StopApplication();
    }

    void
    ClusterHeadApp::SendInterest()
    {
        auto interest = std::make_shared<ndn::Interest>("/prefix/sub");
        Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
        interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
        interest->setInterestLifetime(ndn::time::seconds(1));

        NS_LOG_DEBUG("Sending Interest packet for " << *interest);

        m_transmittedInterests(interest, this, m_face);

        m_appLink->onReceiveInterest(*interest);
    }

    void
    ClusterHeadApp::OnInterest(std::shared_ptr<const ndn::Interest> interest)
    {
        ndn::App::OnInterest(interest);

        NS_LOG_DEBUG("Received Interest packet for " << interest->getName());

        auto data = std::make_shared<ndn::Data>(interest->getName());
        data->setFreshnessPeriod(ndn::time::milliseconds(1000));
        data->setContent(std::make_shared< ::ndn::Buffer>(1024));
        ndn::StackHelper::getKeyChain().sign(*data);

        NS_LOG_DEBUG("Sending Data packet for " << data->getName());

        m_transmittedDatas(data, this, m_face);

        m_appLink->onReceiveData(*data);
    }

    void
    ClusterHeadApp::OnData(std::shared_ptr<const ndn::Data> data)
    {
        NS_LOG_DEBUG("Receiving Data packet for " << data->getName());

        std::cout << "DATA received for name " << data->getName() << std::endl;
    }

} // namespace ns3
