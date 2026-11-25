#include <infini/rt.h>

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>

#include "test_helper.h"

namespace {

namespace runtime = infini::rt::runtime;
using Context = infini::rt::test::TestContext;

bool Success(Context* context, runtime::Error status) {
  return context->Expect(status == runtime::kSuccess,
                         "Priority stream operation should succeed.");
}

void CheckPriority(Context* context, int requested, int greatest, int least) {
  runtime::Stream stream{};
  if (!Success(context,
               runtime::StreamCreateWithPriority(&stream, 1, requested))) {
    return;
  }

  int actual = INT_MAX;
  Success(context, runtime::StreamGetPriority(stream, &actual));
  context->ExpectEqual(actual, std::clamp(requested, greatest, least),
                       "Stream priority should match the clamped request.");

  void* data = nullptr;
  if (!Success(context, runtime::Malloc(&data, 128))) {
    Success(context, runtime::StreamDestroy(stream));
    return;
  }

  runtime::Event ready{};
  runtime::Stream consumer{};
  const bool event_ok =
      Success(context, runtime::EventCreateWithFlags(&ready, 2));
  const bool consumer_ok = Success(context, runtime::StreamCreate(&consumer));
  if (event_ok && consumer_ok) {
    for (int iteration = 0; iteration < 3; ++iteration) {
      const int pattern = 19 + iteration;
      Success(context, runtime::MemsetAsync(data, pattern, 128, stream));
      Success(context, runtime::EventRecord(ready, stream));
      Success(context, runtime::StreamWaitEvent(consumer, ready, 0));
      Success(context, runtime::StreamSynchronize(consumer));
      std::array<std::uint8_t, 128> output{};
      Success(context, runtime::Memcpy(output.data(), data, output.size(),
                                       runtime::kMemcpyDeviceToHost));
      context->Expect(
          std::all_of(output.begin(), output.end(),
                      [pattern](auto value) { return value == pattern; }),
          "Consumer should see each producer update.");
    }
  }

  if (consumer_ok) Success(context, runtime::StreamDestroy(consumer));
  if (event_ok) Success(context, runtime::EventDestroy(ready));
  Success(context, runtime::Free(data));
  Success(context, runtime::StreamDestroy(stream));
}

}  // namespace

int main() {
  Context context;
  infini::rt::set_runtime_device_type(infini::rt::Device::Type::kHygon);
  if (!Success(&context, runtime::SetDevice(0))) return context.ExitCode();

  int least = INT_MIN;
  int greatest = INT_MAX;
  if (!Success(&context,
               runtime::DeviceGetStreamPriorityRange(&least, &greatest))) {
    return context.ExitCode();
  }
  if (!context.Expect(greatest <= least, "Priority range should be ordered.")) {
    return context.ExitCode();
  }
  for (int requested : {least, greatest, 0, INT_MIN, INT_MAX}) {
    CheckPriority(&context, requested, greatest, least);
  }

#ifdef WITH_CPU
  infini::rt::set_runtime_device_type(infini::rt::Device::Type::kCpu);
  int unchanged_least = 17;
  int unchanged_greatest = 19;
  context.Expect(
      runtime::DeviceGetStreamPriorityRange(
          &unchanged_least, &unchanged_greatest) != runtime::kSuccess,
      "CPU should report unsupported stream priority.");
  context.ExpectEqual(unchanged_least, 17,
                      "Unsupported query should preserve the least output.");
  context.ExpectEqual(unchanged_greatest, 19,
                      "Unsupported query should preserve the greatest output.");
  runtime::Stream untouched{};
  context.Expect(
      runtime::StreamCreateWithPriority(&untouched, 1, 0) != runtime::kSuccess,
      "CPU should reject priority stream creation.");
  context.Expect(untouched == nullptr,
                 "Unsupported creation should preserve the stream output.");
  int untouched_priority = 23;
  context.Expect(runtime::StreamGetPriority(untouched, &untouched_priority) !=
                     runtime::kSuccess,
                 "CPU should reject stream priority queries.");
  context.ExpectEqual(untouched_priority, 23,
                      "Unsupported query should preserve the priority output.");
#endif

  return context.ExitCode();
}
