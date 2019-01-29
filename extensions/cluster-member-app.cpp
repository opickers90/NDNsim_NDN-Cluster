#include "cluster-member-app.hpp"

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

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <ndn-cxx/lp/tags.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

NS_LOG_COMPONENT_DEFINE("ndn.ClusterMemberApp");

namespace ns3 {
    namespace ndn {
        NS_OBJECT_ENSURE_REGISTERED(ClusterMemberApp);
        TypeId
        ClusterMemberApp::GetTypeId(void)
        {
            static TypeId tid = TypeId("ClusterMemberApp")
                            .SetGroupName("Ndn")
                            .SetParent<App>()
                            //.AddConstructor<ClusterMemberApp>()
                            .AddAttribute("StartSeq", "Initial sequence number", IntegerValue(0),
                                          MakeIntegerAccessor(&ClusterMemberApp::m_seq), MakeIntegerChecker<int32_t>())
                            .AddAttribute("Prefix", "Name of the Interest", StringValue("/"),
                                          MakeNameAccessor(&ClusterMemberApp::m_interestName), MakeNameChecker())
                            .AddAttribute("LifeTime", "LifeTime for interest packet", StringValue("2s"),
                                          MakeTimeAccessor(&ClusterMemberApp::m_interestLifeTime), MakeTimeChecker())
                            .AddAttribute("RetxTimer",
                                          "Timeout defining how frequent retransmission timeouts should be checked",
                                          StringValue("50ms"),
                                          MakeTimeAccessor(&ClusterMemberApp::GetRetxTimer, &ClusterMemberApp::SetRetxTimer),
                                          MakeTimeChecker())
                            .AddTraceSource("LastRetransmittedInterestDataDelay",
                                            "Delay between last retransmitted Interest and received Data",
                                            MakeTraceSourceAccessor(&ClusterMemberApp::m_lastRetransmittedInterestDataDelay),
                                            "ns3::ndn::ClusterMemberApp::LastRetransmittedInterestDataDelayCallback")
                            .AddTraceSource("FirstInterestDataDelay",
                                            "Delay between first transmitted Interest and received Data",
                                            MakeTraceSourceAccessor(&ClusterMemberApp::m_firstInterestDataDelay),
                                            "ns3::ndn::ClusterMemberApp::FirstInterestDataDelayCallback")
                            .AddAttribute("Frequency", "Frequency of interest packets", StringValue("1.0"),
                                          MakeDoubleAccessor(&ClusterMemberApp::m_frequency), MakeDoubleChecker<double>())
            ;
            return tid;
        }

        ClusterMemberApp::ClusterMemberApp()
                : m_rand(CreateObject<UniformRandomVariable>())
                , m_seq(0)
                , m_seqMax(0)
                , m_frequency(1.0)
                , m_firstTime(true)
        {
            NS_LOG_FUNCTION_NOARGS();
            m_rtt = CreateObject<RttMeanDeviation>();
            m_seqMax = std::numeric_limits<uint32_t>::max();
        }

        //ClusterMemberApp::~ClusterMemberApp ()
       // {
       // }

        void
        ClusterMemberApp::SetRetxTimer(Time retxTimer)
        {
            m_retxTimer = retxTimer;
            if (m_retxEvent.IsRunning()) {

                Simulator::Remove(m_retxEvent); // slower, but better for memory
            }

            m_retxEvent = Simulator::Schedule(m_retxTimer, &ClusterMemberApp::CheckRetxTimeout, this);
        }

        Time
        ClusterMemberApp::GetRetxTimer() const
        {
            return m_retxTimer;
        }

        void
        ClusterMemberApp::CheckRetxTimeout()
        {
            Time now = Simulator::Now();
            Time rto = m_rtt->RetransmitTimeout();
            // NS_LOG_DEBUG ("Current RTO: " << rto.ToDouble (Time::S) << "s");
            while (!m_seqTimeouts.empty()) {
                SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
                        m_seqTimeouts.get<i_timestamp>().begin();
                if (entry->time + rto <= now) // timeout expired?
                {
                    uint32_t seqNo = entry->seq;
                    m_seqTimeouts.get<i_timestamp>().erase(entry);
                    OnTimeout(seqNo);
                }
                else
                    break; // nothing else to do. All later packets need not be retransmitted
            }
            m_retxEvent = Simulator::Schedule(m_retxTimer, &ClusterMemberApp::CheckRetxTimeout, this);
        }

        void
        ClusterMemberApp::StartApplication() // Called at time specified by Start
        {
            NS_LOG_FUNCTION_NOARGS();
            // do base stuff
            App::StartApplication();
            ScheduleNextPacket();
        }

        void
        ClusterMemberApp::StopApplication() // Called at time specified by Stop
        {
            NS_LOG_FUNCTION_NOARGS();
            // cancel periodic packet generation
            Simulator::Cancel(m_sendEvent);
            // cleanup base stuff
            App::StopApplication();
        }

        void
        ClusterMemberApp::SendPacket()
        {
            if (!m_active)
                return;
            NS_LOG_FUNCTION_NOARGS();
            uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid
            while (m_retxSeqs.size()) {
                seq = *m_retxSeqs.begin();
                m_retxSeqs.erase(m_retxSeqs.begin());
                break;
            }
            if (seq == std::numeric_limits<uint32_t>::max()) {
                if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
                    if (m_seq >= m_seqMax) {
                        return; // we are totally done
                    }
                }
                seq = m_seq++;
            }
            //
            shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
            nameWithSequence->appendSequenceNumber(seq);
            //
            // shared_ptr<Interest> interest = make_shared<Interest> ();
            shared_ptr<Interest> interest = make_shared<Interest>();
            interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
            interest->setName(*nameWithSequence);
            time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
            interest->setInterestLifetime(interestLifeTime);
            // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
            NS_LOG_INFO("> Interest for " << seq);
            WillSendOutInterest(seq);
            m_transmittedInterests(interest, this, m_face);
            m_appLink->onReceiveInterest(*interest);
            ScheduleNextPacket();
        }

        void
        ClusterMemberApp::OnData(shared_ptr<const Data> data)
        {
            if (!m_active)
                return;
            App::OnData(data); // tracing inside
            NS_LOG_FUNCTION(this << data);

            uint32_t seq = data->getName().at(-1).toSequenceNumber();
            NS_LOG_INFO("< DATA for " << seq);
            int hopCount = 0;
            auto hopCountTag = data->getTag<lp::HopCountTag>();
            if (hopCountTag != nullptr) { // e.g., packet came from local node's cache
                hopCount = *hopCountTag;
            }
            NS_LOG_DEBUG("Hop count: " << hopCount);
            SeqTimeoutsContainer::iterator entry = m_seqLastDelay.find(seq);
            if (entry != m_seqLastDelay.end()) {
                m_lastRetransmittedInterestDataDelay(this, seq, Simulator::Now() - entry->time, hopCount);
            }
            entry = m_seqFullDelay.find(seq);
            if (entry != m_seqFullDelay.end()) {
                m_firstInterestDataDelay(this, seq, Simulator::Now() - entry->time, m_seqRetxCounts[seq], hopCount);
            }
            m_seqRetxCounts.erase(seq);
            m_seqFullDelay.erase(seq);
            m_seqLastDelay.erase(seq);
            m_seqTimeouts.erase(seq);
            m_retxSeqs.erase(seq);
            m_rtt->AckSeq(SequenceNumber32(seq));
        }

        void
        ClusterMemberApp::OnNack(shared_ptr<const lp::Nack> nack)
        {

            App::OnNack(nack);
            NS_LOG_INFO("NACK received for: " << nack->getInterest().getName()
                                              << ", reason: " << nack->getReason());
        }

        void
        ClusterMemberApp::OnTimeout(uint32_t sequenceNumber)
        {
            NS_LOG_FUNCTION(sequenceNumber);
            // std::cout << Simulator::Now () << ", TO: " << sequenceNumber << ", current RTO: " <<
            // m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";
            m_rtt->IncreaseMultiplier(); // Double the next RTO
            m_rtt->SentSeq(SequenceNumber32(sequenceNumber),
                           1); // make sure to disable RTT calculation for this sample
            m_retxSeqs.insert(sequenceNumber);
            ScheduleNextPacket();
        }

        void
        ClusterMemberApp::WillSendOutInterest(uint32_t sequenceNumber)
        {
            NS_LOG_DEBUG("Trying to add " << sequenceNumber << " with " << Simulator::Now() << ". already "
                                          << m_seqTimeouts.size() << " items");
            m_seqTimeouts.insert(SeqTimeout(sequenceNumber, Simulator::Now()));
            m_seqFullDelay.insert(SeqTimeout(sequenceNumber, Simulator::Now()));
            m_seqLastDelay.erase(sequenceNumber);
            m_seqLastDelay.insert(SeqTimeout(sequenceNumber, Simulator::Now()));
            m_seqRetxCounts[sequenceNumber]++;
            m_rtt->SentSeq(SequenceNumber32(sequenceNumber), 1);
        }
/*
        void
        ClusterMemberApp::ScheduleNextPacket()
        {
            // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
            // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";

            if (m_firstTime) {
                m_sendEvent = Simulator::Schedule(Seconds(0.0), &ClusterMemberApp::SendPacket, this);
                m_firstTime = false;
            }
            else if (!m_sendEvent.IsRunning())
                m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                                  : Seconds(m_random->GetValue()),
                                                  &ClusterMemberApp::SendPacket, this);
        }
*/
    } // namespace ndn
} // namespace ns3
