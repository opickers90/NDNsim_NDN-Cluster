#include "cluster-strategy-bestroute.hpp"

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
          if (!parsed.parameters.empty()) {
            BOOST_THROW_EXCEPTION(std::invalid_argument("ClusterStrategy does not accept parameters"));
          }
          if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
            BOOST_THROW_EXCEPTION(std::invalid_argument(
                    "ClusterStrategy does not support version " + to_string(*parsed.version)));
          }
          this->setInstanceName(makeInstanceName(name, getStrategyName()));
        }


        static inline bool
        isNextHopEligible(const Face& inFace, const Interest& interest,
                          const fib::NextHop& nexthop,
                          const shared_ptr<pit::Entry>& pitEntry,
                          bool wantUnused = false,
                          time::steady_clock::TimePoint now = time::steady_clock::TimePoint::min())
        {
          const Face& outFace = nexthop.getFace();
          // do not forward back to the same face, unless it is ad hoc
          if (outFace.getId() == inFace.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC)
            return false;
          // forwarding would violate scope
          if (wouldViolateScope(inFace, interest, outFace))
            return false;
          if (wantUnused) {
            // nexthop must not have unexpired out-record
            pit::OutRecordCollection::iterator outRecord = pitEntry->getOutRecord(outFace);
            if (outRecord != pitEntry->out_end() && outRecord->getExpiry() > now) {
              return false;
            }
          }
          return true;
        }


        static inline fib::NextHopList::const_iterator
        findEligibleNextHopWithEarliestOutRecord(const Face& inFace, const Interest& interest,
                                                 const fib::NextHopList& nexthops,
                                                 const shared_ptr<pit::Entry>& pitEntry)
        {
          fib::NextHopList::const_iterator found = nexthops.end();
          time::steady_clock::TimePoint earliestRenewed = time::steady_clock::TimePoint::max();
          for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
            if (!isNextHopEligible(inFace, interest, *it, pitEntry))
              continue;
            pit::OutRecordCollection::iterator outRecord = pitEntry->getOutRecord(it->getFace());
            BOOST_ASSERT(outRecord != pitEntry->out_end());
            if (outRecord->getLastRenewed() < earliestRenewed) {
              found = it;
              earliestRenewed = outRecord->getLastRenewed();
            }
          }
          return found;
        }


        void
        ClusterStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
                                              const shared_ptr<pit::Entry>& pitEntry)
        {
          RetxSuppressionResult suppression = m_retxSuppression.decidePerPitEntry(*pitEntry);
          if (suppression == RetxSuppressionResult::SUPPRESS) {
            NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                                   << " suppressed");
            return;
          }


          const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
          const fib::NextHopList& nexthops = fibEntry.getNextHops();
          fib::NextHopList::const_iterator it = nexthops.end();


          if (suppression == RetxSuppressionResult::NEW) {
            // forward to nexthop with lowest cost except downstream
            it = std::find_if(nexthops.begin(), nexthops.end(),
                              bind(&isNextHopEligible, cref(inFace), interest, _1, pitEntry,
                                   false, time::steady_clock::TimePoint::min()));
            if (it == nexthops.end()) {
              NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " noNextHop");
              this->rejectPendingInterest(pitEntry);
              return;
            }
            Face& outFace = it->getFace();
            this->sendInterest(pitEntry, outFace, interest);
            NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                                   << " newPitEntry-to=" << outFace.getId());
            return;
          }


          // find an unused upstream with lowest cost except downstream
          it = std::find_if(nexthops.begin(), nexthops.end(),
                            bind(&isNextHopEligible, cref(inFace), interest, _1, pitEntry,
                                 true, time::steady_clock::now()));
          if (it != nexthops.end()) {
            Face& outFace = it->getFace();
            this->sendInterest(pitEntry, outFace, interest);
            NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                                   << " retransmit-unused-to=" << outFace.getId());
            return;
          }


          // find an eligible upstream that is used earliest
          it = findEligibleNextHopWithEarliestOutRecord(inFace, interest, nexthops, pitEntry);
          if (it == nexthops.end()) {
            NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " retransmitNoNextHop");
          }
          else {
            Face& outFace = it->getFace();
            this->sendInterest(pitEntry, outFace, interest);
            NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                                   << " retransmit-retry-to=" << outFace.getId());
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
