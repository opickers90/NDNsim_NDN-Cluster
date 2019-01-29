#ifndef CLUSTERHEAD_APP_H_
#define CLUSTERHEAD_APP_H_

#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/nstime.h"
#include "ns3/ptr.h"

namespace ns3 {

class ClusterHeadApp : public ndn::App {
public:
  static TypeId
  GetTypeId(void);

  ClusterHeadApp();

  virtual void
  StartApplication();

  virtual void
  StopApplication();

  virtual void
  OnInterest(std::shared_ptr<const ndn::Interest> interest);

  virtual void
  OnData(std::shared_ptr<const ndn::Data> contentObject);

protected:


private:
  ndn::Name m_prefix;

  void
  SendInterest();
};

} // namespace ns3

#endif // CLUSTERHEAD_APP_H_
