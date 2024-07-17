#pragma once

#include "platform/common.h"

namespace vdd {
  [[nodiscard]] std::unique_ptr<platf::deinit_t>
  start();
}
