// SPDX-License-Identifier: Apache-2.0
// Verify vendor entropy-provider lifecycle and failure propagation, not a software PRNG.
#include "microhal/core/status.hpp"
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>

#define MICROHAL_NORDIC_HAS_RNG_HAL 0
#define MICROHAL_NORDIC_HAS_CC3XX_ENTROPY 0
#define MICROHAL_NORDIC_HAS_CRACEN_ENTROPY 1
#define SX_OK 0
#define SX_ERR_HW_PROCESSING -1
struct sx_trng
{
};
static int opens{}, closes{}, attempts{};
static bool bad_open{}, bad_close{}, bad_entropy{}, stuck{};
static int sx_trng_open(sx_trng*, void const*)
{
  ++opens;
  return bad_open ? -2 : SX_OK;
}
static int sx_trng_close(sx_trng*)
{
  ++closes;
  return bad_close ? -2 : SX_OK;
}
static int sx_trng_get(sx_trng*, std::uint8_t* output, std::size_t bytes)
{
  ++attempts;
  assert(bytes == sizeof(std::uint32_t));
  if (stuck || attempts == 1)
    return SX_ERR_HW_PROCESSING;
  if (bad_entropy)
    return -2;
  constexpr std::uint32_t word{0x12345678U};
  std::memcpy(output, &word, bytes);
  return SX_OK;
}
namespace microhal::backends::nordic::nrfx
{
#include "microhal/backends/nordic/nrfx/rng_engine.inc"
}
int main()
{
  using microhal::status;
  using driver = microhal::backends::nordic::nrfx::system_detail::entropy_driver;
  assert(driver::read().error() == status::unsupported);
  bad_open = true;
  assert(driver::initialize() == status::io_error && closes == 0);
  bad_open = false;
  assert(driver::initialize() == status::ok && closes == 1);
  assert(driver::read().value() == 0x12345678U && attempts == 2 && closes == 2);
  bad_entropy = true;
  assert(driver::read().error() == status::io_error && closes == 3 && !driver::active.load());
  bad_entropy = false;
  bad_close = true;
  assert(driver::read().error() == status::io_error && closes == 4 && !driver::active.load());
  bad_close = false;
  stuck = true;
  assert(driver::read().error() == status::timeout && closes == 5 && !driver::active.load());
  driver::active.store(true);
  assert(driver::read().error() == status::busy && opens == 6);
  driver::active.store(false);
}
