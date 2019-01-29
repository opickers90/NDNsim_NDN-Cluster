#include "cluster-member-strategy.hpp"

#include <boost/random/uniform_int_distribution.hpp>
#include <ndn-cxx/util/random.hpp>

#include "fw/algorithm.hpp"

#include "core/logger.hpp"


NFD_LOG_INIT("ClusterStrategy");

namespace nfd {
    namespace fw {

        NFD_REGISTER_STRATEGY(ClusterStrategy);

        const time::milliseconds ClusterStrategy::RETX_SUPPRESSION_INITIAL(10);
        const time::milliseconds ClusterStrategy::RETX_SUPPRESSION_MAX(250);

        ClusterStrategy::ClusterStrategy(Forwarder& forwarder, const Name& name)
                : Strategy(forwarder)
                , m_retxSuppression(RETX_SUPPRESSION_INITIAL,
                                    RetxSuppressionExponential::DEFAULT_MULTIPLIER,
                                    RETX_SUPPRESSION_MAX)
        {
          ParsedInstanceName parsed = parseInstanceName(name);
          this->setInstanceName(makeInstanceName(name, getStrategyName()));
        }

        void
        ClusterStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
                                              const shared_ptr<pit::Entry>& pitEntry)
        {
            const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
            const fib::NextHopList& nexthops = fibEntry.getNextHops();

            int nEligibleNextHops = 0;

            bool isSuppressed = false;

            for (const auto& nexthop : nexthops) {
                Face& outFace = nexthop.getFace();

                RetxSuppressionResult suppressResult = m_retxSuppression.decidePerUpstream(*pitEntry, outFace);

                if (suppressResult == RetxSuppressionResult::SUPPRESS) {
                    NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                                           << "to=" << outFace.getId() << " suppressed");
                    isSuppressed = true;
                    continue;
                }

                if ((outFace.getId() == inFace.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
                    wouldViolateScope(inFace, interest, outFace)) {
                    continue;
                }

                this->sendInterest(pitEntry, outFace, interest);
                NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                                       << " pitEntry-to=" << outFace.getId());

                if (suppressResult == RetxSuppressionResult::FORWARD) {
                    m_retxSuppression.incrementIntervalForOutRecord(*pitEntry->getOutRecord(outFace));
                }
                ++nEligibleNextHops;
            }

            if (nEligibleNextHops == 0 && !isSuppressed) {
                NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " noNextHop");
                this->rejectPendingInterest(pitEntry);
            }
        }

        const Name&
        ClusterStrategy::getStrategyName()
        {
          static Name strategyName("ndn:/localhost/nfd/strategy/cluster-member/%FD%01");
          return strategyName;
        }

    } // namespace fw
} // namespace nfd
