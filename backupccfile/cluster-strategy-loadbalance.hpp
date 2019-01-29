
#ifndef CLUSTER_STRATEGY_FORWARDING_HPP
#define CLUSTER_STRATEGY_FORWARDING_HPP

#include <boost/random/mersenne_twister.hpp>
#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "fw/algorithm.hpp"

namespace nfd {
namespace fw {

class ClusterStrategy : public Strategy {
public:
    ClusterStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

    static const Name&
    getStrategyName();

    virtual ~ClusterStrategy() override;

    virtual void
    afterReceiveInterest(const Face& inFace, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;


protected:

    boost::random::mt19937 m_randomGenerator;

    };

  } // namespace fw
} // namespace nfd

#endif // CLUSTER_STRATEGY_FORWARDING_HPP
