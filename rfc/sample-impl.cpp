#include "sample-impl.hpp"

namespace sample {

int SampleServerImpl::Add(int* result, int left, int right) {
    *result = left + right;
    return 0;
}

int SampleServerImpl::Sub(int* result, int left, int right) {
    *result = left + right;
}

} // namespace sample
