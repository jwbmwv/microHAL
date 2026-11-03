// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>

namespace microhal
{
/// External trigger selector values are the device's unshifted EXTSEL encoding.
enum class adc_trigger_edge : std::uint8_t
{
  software,
  rising,
  falling,
  both
};
} // namespace microhal
