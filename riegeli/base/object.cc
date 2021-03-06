// Copyright 2017 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "riegeli/base/object.h"

#include <stdint.h>
#include <atomic>
#include <utility>

#include "absl/base/optimization.h"
#include "riegeli/base/base.h"
#include "riegeli/base/status.h"

namespace riegeli {

// Before C++17 if a constexpr static data member is ODR-used, its definition at
// namespace scope is required. Since C++17 these definitions are deprecated:
// http://en.cppreference.com/w/cpp/language/static
#if __cplusplus < 201703
constexpr uintptr_t Object::kHealthy;
constexpr uintptr_t Object::kClosedSuccessfully;
#endif

inline Object::FailedStatus::FailedStatus(Status&& status)
    : status(std::move(status)) {}

bool Object::Fail(Status status) {
  RIEGELI_ASSERT(!status.ok())
      << "Failed precondition of Object::Fail(): status not failed";
  RIEGELI_ASSERT(!closed())
      << "Failed precondition of Object::Fail(): Object closed";
  const uintptr_t new_status_ptr =
      reinterpret_cast<uintptr_t>(new FailedStatus(std::move(status)));
  uintptr_t old_status = kHealthy;
  if (ABSL_PREDICT_FALSE(!status_ptr_.compare_exchange_strong(
          old_status, new_status_ptr, std::memory_order_release))) {
    // status_ptr_ was already set, new_status_ptr loses.
    DeleteStatus(new_status_ptr);
  }
  return false;
}

bool Object::Fail(const Object& dependency) {
  RIEGELI_ASSERT(!dependency.healthy())
      << "Failed precondition of Object::Fail(): dependency healthy";
  RIEGELI_ASSERT(!closed())
      << "Failed precondition of Object::Fail(): Object closed";
  return Fail(dependency.status());
}

bool Object::Fail(const Object& dependency, Status fallback) {
  RIEGELI_ASSERT(!fallback.ok())
      << "Failed precondition of Object::Fail(): status not failed";
  RIEGELI_ASSERT(!closed())
      << "Failed precondition of Object::Fail(): Object closed";
  return Fail(!dependency.healthy() ? dependency.status()
                                    : std::move(fallback));
}

TypeId Object::GetTypeId() const { return TypeId(); }

}  // namespace riegeli
