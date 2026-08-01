// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_pwm_motor_control.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

// STM32 PWM Motor Control Example
// Demonstrates: Center-aligned PWM output, alternate-function pin muxing, soft-start ramp

#include "microhal/gpio/gpio.hpp"
#include "microhal/pwm/pwm.hpp"
#include "microhal/timers/timer.hpp"
#include <cstdint>

namespace
{

using backend = microhal::stm32_hal_tag;

// Timer Configuration: TIM1 clocked from the 84 MHz timer domain in center-aligned mode
using timer_cfg =
    microhal::timers::timer_config<1U,        // TIM1
                                   84000000U, // 84 MHz
                                   microhal::timer_counter_mode::center_aligned, microhal::timer_clock_source::internal,
                                   true, // Preload enable
                                   false // One-pulse mode
                                   >;

// PWM Configuration: Channel 1 at 20 kHz with backend-managed duty updates
using pwm_cfg = microhal::pwm::pwm_config<1U,    // TIM1
                                          1U,    // Channel 1
                                          20000U // 20 kHz PWM frequency
                                          >;

// GPIO: TIM1_CH1 (PA8), TIM1_CH1N (PA7) - alternate-function outputs for the power stage
using pwm_high_side = microhal::gpio::pin_config<0U, 8U, microhal::gpio_mode::alternate, microhal::gpio_pull::none,
                                                 microhal::gpio_speed::very_high, 1U>; // AF1

using pwm_low_side = microhal::gpio::pin_config<0U, 7U, microhal::gpio_mode::alternate, microhal::gpio_pull::none,
                                                microhal::gpio_speed::very_high, 1U>; // AF1

// Motor control state
struct motor_controller
{
  std::uint16_t target_duty = 0U;  // Target duty cycle (0-1000 permille)
  std::uint16_t current_duty = 0U; // Current duty cycle
  std::uint16_t ramp_rate = 5U;    // Duty cycle change per update (permille)
  bool running = false;

  enum class direction
  {
    forward,
    reverse,
    brake
  };

  direction motor_direction = direction::forward;
};

motor_controller motor{};

// Soft-start ramp function (call periodically, e.g., from timer ISR)
[[nodiscard]] microhal::status update_motor_pwm() noexcept
{
  if (!motor.running)
  {
    motor.current_duty = 0U;
    return microhal::pwm::output<backend, pwm_cfg>::set_duty_cycle(0U);
  }

  // Ramp current duty toward target
  if (motor.current_duty < motor.target_duty)
  {
    motor.current_duty += motor.ramp_rate;
    if (motor.current_duty > motor.target_duty)
    {
      motor.current_duty = motor.target_duty;
    }
  }
  else if (motor.current_duty > motor.target_duty)
  {
    motor.current_duty -= motor.ramp_rate;
    if (motor.current_duty < motor.target_duty)
    {
      motor.current_duty = motor.target_duty;
    }
  }

  // Update PWM output
  return microhal::pwm::output<backend, pwm_cfg>::set_duty_cycle(motor.current_duty);
}

// Motor control API
void set_motor_speed(const std::uint16_t speed_permille) noexcept
{
  motor.target_duty = (speed_permille > 1000U) ? 1000U : speed_permille;
}

[[nodiscard]] microhal::status start_motor() noexcept
{
  motor.running = true;
  return microhal::pwm::output<backend, pwm_cfg>::start();
}

[[nodiscard]] microhal::status stop_motor() noexcept
{
  motor.running = false;
  motor.target_duty = 0U;
  motor.current_duty = 0U;
  return microhal::pwm::output<backend, pwm_cfg>::stop();
}

[[maybe_unused]] [[nodiscard]] microhal::status emergency_brake() noexcept
{
  // Immediate stop without soft-stop ramp. The duty cycle is forced to zero first so the bridge is
  // already idle even if stopping the timer fails.
  motor.running = false;
  motor.target_duty = 0U;
  motor.current_duty = 0U;

  const microhal::status duty_status = microhal::pwm::output<backend, pwm_cfg>::set_duty_cycle(0U);
  const microhal::status stop_status = microhal::pwm::output<backend, pwm_cfg>::stop();

  return microhal::is_ok(duty_status) ? stop_status : duty_status;
}

} // namespace

int main()
{
  // Compile-time validation
  static_assert(microhal::PwmConcept<backend, pwm_cfg>);
  static_assert(microhal::TimerConcept<backend, timer_cfg>);

  // Configure GPIO pins for PWM output
  if (!microhal::is_ok(microhal::gpio::pin<backend, pwm_high_side>::configure()) ||
      !microhal::is_ok(microhal::gpio::pin<backend, pwm_low_side>::configure()))
  {
    return 1;
  }

  // Initialize timer and PWM
  if (!microhal::is_ok(microhal::timers::timer<backend, timer_cfg>::initialize()) ||
      !microhal::is_ok(microhal::pwm::output<backend, pwm_cfg>::initialize()))
  {
    return 1;
  }

  // Set initial duty cycle to 0%
  if (!microhal::is_ok(microhal::pwm::output<backend, pwm_cfg>::set_duty_cycle(0U)))
  {
    return 1;
  }

  // Example motor control sequence
  if (!microhal::is_ok(start_motor()))
  {
    return 1;
  }

  // Ramp to 30% speed over time
  set_motor_speed(300U); // 30%

  // Simulate periodic update loop (typically called from timer ISR at 1-10 kHz)
  for (std::uint32_t i = 0U; i < 1000U; ++i)
  {
    if (!microhal::is_ok(update_motor_pwm()))
    {
      (void)emergency_brake();
      return 1;
    }

    // Simulate delay (replace with actual timer interrupt)
    for (std::uint32_t delay = 0U; delay < 10000U; ++delay)
    {
    }
  }

  // Ramp to 70% speed
  set_motor_speed(700U); // 70%

  for (std::uint32_t i = 0U; i < 1000U; ++i)
  {
    if (!microhal::is_ok(update_motor_pwm()))
    {
      (void)emergency_brake();
      return 1;
    }
    for (std::uint32_t delay = 0U; delay < 10000U; ++delay)
    {
    }
  }

  // Soft stop
  set_motor_speed(0U);

  for (std::uint32_t i = 0U; i < 1000U; ++i)
  {
    if (!microhal::is_ok(update_motor_pwm()))
    {
      (void)emergency_brake();
      return 1;
    }
    for (std::uint32_t delay = 0U; delay < 10000U; ++delay)
    {
    }
  }

  return microhal::is_ok(stop_motor()) ? 0 : 1;
}
