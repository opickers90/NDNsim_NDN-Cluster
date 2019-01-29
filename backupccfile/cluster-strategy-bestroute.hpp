#ifndef CLUSTER_STRATEGY_FORWARDING_HPP
#define CLUSTER_STRATEGY_FORWARDING_HPP

#include <boost/random/mersenne_twister.hpp>

#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "fw/algorithm.hpp"

#include "fw/retx-suppression-exponential.hpp"

namespace nfd {
    namespace fw {
        class ClusterStrategy : public Strategy

        {
        public:
            explicit
            ClusterStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

            static const Name&
            getStrategyName();

            void
            afterReceiveInterest(const Face& inFace, const Interest& interest,
                                 const shared_ptr<pit::Entry>& pitEntry) override;


            PUBLIC_WITH_TESTS_ELSE_PRIVATE:
            static const time::milliseconds RETX_SUPPRESSION_INITIAL;
            static const time::milliseconds RETX_SUPPRESSION_MAX;
            RetxSuppressionExponential m_retxSuppression;

        };

    } // namespace fw
} // namespace nfd

#endif // CLUSTER_STRATEGY_FORWARDING_HPP
