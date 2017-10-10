#include "sample-server.hpp"

namespace sample {

class SampleServerImpl : public SampleServer {
 public:
  using SampleServer::SampleServer;

  int Add(int* result, int left, int right) override;

  int Sub(int* result, int left, int right) override;
};

} // namespace sample
