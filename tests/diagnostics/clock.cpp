// SPDX-License-Identifier: Apache-2.0

#if defined(MICROHAL_TEST_PROBE_FEATURE)
static_assert(__cpp_static_assert >= 202306L);
#else
#include "microhal/i2c/i2c.hpp"
#include "microhal/spi/spi.hpp"

#if defined(MICROHAL_TEST_I2C)
using config = microhal::i2c::master_config<0U, MICROHAL_TEST_CLOCK_HZ>;
static_assert(microhal::validate_i2c_config<microhal::mock_tag, config>());
#else
using config = microhal::spi::master_config<0U, MICROHAL_TEST_CLOCK_HZ>;
static_assert(microhal::validate_spi_config<microhal::mock_tag, config>());
#endif
#endif

int main() {}
