#ifndef INFINI_RT_HYGON_DATA_TYPE__H_
#define INFINI_RT_HYGON_DATA_TYPE__H_

// clang-format off
#include <cuda_bf16.h>
#include <cuda_fp16.h>
// clang-format on

#include "data_type.h"
#include "native/cuda/hygon/device_.h"

namespace infini::rt {

using cuda_bfloat16 = __nv_bfloat16;

using cuda_bfloat162 = __nv_bfloat162;

template <>
struct TypeMap<Device::Type::kHygon, DataType::kFloat16> {
  using type = __half;
};

template <>
struct TypeMap<Device::Type::kHygon, DataType::kBFloat16> {
  using type = __nv_bfloat16;
};

}  // namespace infini::rt

#endif
