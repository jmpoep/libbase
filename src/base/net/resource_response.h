#pragma once

#if defined(LIBBASE_MODULE_NET)

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "base/net/result.h"
#include "base/time/time_delta.h"

namespace base {
namespace net {

struct ResourceResponse {
  Result result = Result::kOk;
  int code = -1;
  std::string final_url;
  std::map<std::string, std::string> headers;
  std::vector<uint8_t> data;

  base::TimeDelta timing_queue = base::Seconds(-1);
  base::TimeDelta timing_connect = base::Seconds(-1);
  base::TimeDelta timing_start_transfer = base::Seconds(-1);
  base::TimeDelta timing_total = base::Seconds(-1);

  std::string DataAsString() const;
  std::string_view DataAsStringView() const;
};

}  // namespace net
}  // namespace base

#endif  // defined(LIBBASE_MODULE_NET)
