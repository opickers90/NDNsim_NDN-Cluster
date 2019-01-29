#ifndef CLUSTER_MEMBER_APP_H
#define CLUSTER_MEMBER_APP_H

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/apps/ndn-app.hpp"

#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/utils/ndn-rtt-estimator.hpp"

#include <set>
#include <map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace ns3 {
    namespace ndn {

    class ClusterMemberApp : public App {
        public:
            static TypeId
            GetTypeId();

            ClusterMemberApp();
            virtual ~ClusterMemberApp(){};

            virtual void
            OnData(shared_ptr<const Data> contentObject);

            virtual void
            OnNack(shared_ptr<const lp::Nack> nack);

            virtual void
            OnTimeout(uint32_t sequenceNumber);

            void
            SendPacket();

            virtual void
            WillSendOutInterest(uint32_t sequenceNumber);
        public:
            typedef void (*LastRetransmittedInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, int32_t hopCount);
            typedef void (*FirstInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, uint32_t retxCount, int32_t hopCount);

        protected:
            virtual void
            StartApplication();

            virtual void
            StopApplication();

            virtual void
            ScheduleNextPacket() = 0;

            void
            CheckRetxTimeout();

            void
            SetRetxTimer(Time retxTimer);

            Time
            GetRetxTimer() const;

        protected:
            Ptr<UniformRandomVariable> m_rand; ///< @brief nonce generator
            uint32_t m_seq;      ///< @brief currently requested sequence number
            uint32_t m_seqMax;   ///< @brief maximum number of sequence number
            EventId m_sendEvent; ///< @brief EventId of pending "send packet" event
            Time m_retxTimer;    ///< @brief Currently estimated retransmission timer
            EventId m_retxEvent; ///< @brief Event to check whether or not retransmission should be performed
            Ptr<RttEstimator> m_rtt; ///< @brief RTT estimator
            Time m_offTime;          ///< \brief Time interval between packets
            Name m_interestName;     ///< \brief NDN Name of the Interest (use Name)
            Time m_interestLifeTime; ///< \brief LifeTime for interest packet
            double m_frequency; // Frequency of interest packets (in hertz)
            bool m_firstTime;
            Ptr<RandomVariableStream> m_random;
            std::string m_randomType;


            struct RetxSeqsContainer : public std::set<uint32_t> {
            };
            RetxSeqsContainer m_retxSeqs; ///< \brief ordered set of sequence numbers to be retransmitted

            struct SeqTimeout {
                SeqTimeout(uint32_t _seq, Time _time)
                        : seq(_seq)
                        , time(_time)
                {
                }
                uint32_t seq;
                Time time;
            };

            class i_seq {
            };
            class i_timestamp {
            };

            struct SeqTimeoutsContainer
                    : public boost::multi_index::
                    multi_index_container<SeqTimeout,
                            boost::multi_index::
                            indexed_by<boost::multi_index::
                            ordered_unique<boost::multi_index::tag<i_seq>,
                      boost::multi_index::
                      member<SeqTimeout, uint32_t,
                              &SeqTimeout::seq>>,
            boost::multi_index::
            ordered_non_unique<boost::multi_index::
            tag<i_timestamp>,
            boost::multi_index::
            member<SeqTimeout, Time,
                    &SeqTimeout::time>>>> {
            };
            SeqTimeoutsContainer m_seqTimeouts; ///< \brief multi-index for the set of SeqTimeout structs
            SeqTimeoutsContainer m_seqLastDelay;
            SeqTimeoutsContainer m_seqFullDelay;
            std::map<uint32_t, uint32_t> m_seqRetxCounts;
            TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */, int32_t /*hop count*/>
                    m_lastRetransmittedInterestDataDelay;
            TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */,
            uint32_t /*retx count*/, int32_t /*hop count*/> m_firstInterestDataDelay;
            /// @endcond
        };
    } // namespace ndn
} // namespace ns3
#endif
