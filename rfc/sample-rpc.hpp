#include "rpc-include.hpp"

namespace sample {

DECLARE_RPC(Sample) {
    DECLARE_RPC_CONSTRUCTOR(Sample);
    DECLARE_RPC_METHOD(Add, int, int, int);
    DECLARE_RPC_METHOD(Sub, int, int, int);
};

} // namespace sample
