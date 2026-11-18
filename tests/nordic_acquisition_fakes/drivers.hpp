// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>

inline std::uint32_t SystemCoreClock = 16'000'000U;
inline void nrfx_coredep_delay_us(std::uint32_t) {}
#define NRFX_IRQ_DISABLE(irq) ((void)(irq))
inline std::uint32_t nrfx_get_irq_number(void*)
{
  return 0U;
}

struct NRF_TIMER_Type
{
  std::array<std::uint32_t, 6U> cc{};
};
inline std::array<NRF_TIMER_Type, 5U> fake_timers{};
#define NRF_TIMER0 (&fake_timers[0])
#define NRF_TIMER1 (&fake_timers[1])
#define NRF_TIMER2 (&fake_timers[2])
#define NRF_TIMER3 (&fake_timers[3])
#define NRF_TIMER4 (&fake_timers[4])
enum nrf_timer_event_t
{
  NRF_TIMER_EVENT_COMPARE0 = 0x140,
  NRF_TIMER_EVENT_COMPARE1 = 0x144,
  NRF_TIMER_EVENT_COMPARE2 = 0x148,
  NRF_TIMER_EVENT_COMPARE3 = 0x14C,
  NRF_TIMER_EVENT_COMPARE4 = 0x150,
  NRF_TIMER_EVENT_COMPARE5 = 0x154
};
enum nrf_timer_cc_channel_t
{
  NRF_TIMER_CC_CHANNEL0,
  NRF_TIMER_CC_CHANNEL1,
  NRF_TIMER_CC_CHANNEL2,
  NRF_TIMER_CC_CHANNEL3,
  NRF_TIMER_CC_CHANNEL4,
  NRF_TIMER_CC_CHANNEL5
};
constexpr auto NRF_TIMER_MODE_TIMER = 0U, NRF_TIMER_BIT_WIDTH_32 = 3U, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK = 1U;
struct nrfx_timer_config_t
{
  std::uint32_t frequency{}, mode{}, bit_width{}, interrupt_priority{};
  void* p_context{};
};
struct nrfx_timer_t
{
  NRF_TIMER_Type* p_reg;
  struct
  {
    bool initialized{}, running{};
    void* context{};
    void (*handler)(nrf_timer_event_t, void*){};
  } cb;
};
inline bool nrfx_timer_init_check(const nrfx_timer_t* timer)
{
  return timer->cb.initialized;
}
inline int nrfx_timer_init(nrfx_timer_t* timer, const nrfx_timer_config_t* config,
                           void (*handler)(nrf_timer_event_t, void*))
{
  timer->cb.initialized = true;
  timer->cb.context = config->p_context;
  timer->cb.handler = handler;
  return 0;
}
inline bool nrfx_timer_is_enabled(const nrfx_timer_t* timer)
{
  return timer->cb.running;
}
inline void nrfx_timer_enable(nrfx_timer_t* timer)
{
  timer->cb.running = true;
}
inline void nrfx_timer_disable(nrfx_timer_t* timer)
{
  timer->cb.running = false;
}
inline void nrfx_timer_pause(nrfx_timer_t* timer)
{
  timer->cb.running = false;
}
inline void nrfx_timer_resume(nrfx_timer_t* timer)
{
  timer->cb.running = true;
}
inline void nrfx_timer_clear(nrfx_timer_t*) {}
inline void nrfx_timer_compare(nrfx_timer_t* timer, nrf_timer_cc_channel_t channel, std::uint32_t value, bool)
{
  timer->p_reg->cc[channel] = value;
}
inline void nrfx_timer_extended_compare(nrfx_timer_t* timer, nrf_timer_cc_channel_t channel, std::uint32_t value,
                                        std::uint32_t, bool interrupt)
{
  nrfx_timer_compare(timer, channel, value, interrupt);
}
inline std::uint32_t nrfx_timer_capture(nrfx_timer_t*, nrf_timer_cc_channel_t)
{
  return 73U;
}
inline std::uint32_t nrfx_timer_capture_get(nrfx_timer_t* timer, nrf_timer_cc_channel_t channel)
{
  return timer->p_reg->cc[channel];
}
inline std::uint32_t nrfx_timer_capture_task_address_get(nrfx_timer_t*, nrf_timer_cc_channel_t channel)
{
  return 0x40000000U + channel * 4U;
}
inline void nrfx_timer_compare_int_disable(nrfx_timer_t*, std::uint32_t) {}
inline void nrfx_timer_irq_handler(nrfx_timer_t*) {}

struct NRF_PWM_Type
{
};
inline std::array<NRF_PWM_Type, 4U> fake_pwms{};
#define NRF_PWM0 (&fake_pwms[0])
#define NRF_PWM1 (&fake_pwms[1])
#define NRF_PWM2 (&fake_pwms[2])
#define NRF_PWM3 (&fake_pwms[3])
constexpr auto NRF_PWM_PIN_NOT_CONNECTED = UINT32_MAX;
constexpr auto NRF_PWM_MODE_UP = 0U, NRF_PWM_LOAD_INDIVIDUAL = 1U, NRF_PWM_STEP_AUTO = 0U;
constexpr auto NRF_PWM_CHANNEL_COUNT = 4U, NRFX_PWM_FLAG_LOOP = 1U;
enum nrf_pwm_clk_t
{
  NRF_PWM_CLK_16MHz
};
struct nrf_pwm_values_individual_t
{
  std::uint16_t channel_0{}, channel_1{}, channel_2{}, channel_3{};
};
struct nrf_pwm_sequence_t
{
  struct
  {
    nrf_pwm_values_individual_t* p_individual{};
  } values;
  std::uint16_t length{};
};
struct nrfx_pwm_config_t
{
  std::array<std::uint32_t, 4U> output_pins{};
  std::uint32_t irq_priority{}, count_mode{}, load_mode{}, step_mode{};
  nrf_pwm_clk_t base_clock{};
  std::uint16_t top_value{};
};
struct nrfx_pwm_t
{
  NRF_PWM_Type* p_reg;
  struct
  {
    bool initialized{}, running{};
  } cb;
};
inline std::uint32_t fake_pwm_starts{}, fake_pwm_stops{};
inline bool fake_pwm_can_stop = true;
inline bool nrfx_pwm_init_check(const nrfx_pwm_t* pwm)
{
  return pwm->cb.initialized;
}
inline int nrfx_pwm_init(nrfx_pwm_t* pwm, const nrfx_pwm_config_t*, void*, void*)
{
  pwm->cb.initialized = true;
  return 0;
}
inline std::uint32_t nrfx_pwm_simple_playback(nrfx_pwm_t* pwm, const nrf_pwm_sequence_t*, std::uint16_t, std::uint32_t)
{
  ++fake_pwm_starts;
  pwm->cb.running = true;
  return 0U;
}
inline bool nrfx_pwm_stop(nrfx_pwm_t* pwm, bool)
{
  ++fake_pwm_stops;
  if (fake_pwm_can_stop)
    pwm->cb.running = false;
  return fake_pwm_can_stop;
}
inline bool nrfx_pwm_stopped_check(nrfx_pwm_t* pwm)
{
  return !pwm->cb.running;
}
inline void nrf_pwm_pin_set(NRF_PWM_Type*, std::uint8_t, std::uint32_t) {}
inline void nrf_pwm_configure(NRF_PWM_Type*, nrf_pwm_clk_t, std::uint32_t, std::uint16_t) {}
inline void nrf_gpio_pin_write(std::uint32_t, bool) {}
inline void nrf_gpio_cfg_output(std::uint32_t) {}
enum nrf_gpio_pin_pull_t
{
  NRF_GPIO_PIN_NOPULL,
  NRF_GPIO_PIN_PULLUP,
  NRF_GPIO_PIN_PULLDOWN
};
inline bool nrf_gpio_pin_present_check(std::uint32_t pin)
{
  return pin < 48U;
}
inline void nrf_gpio_cfg_input(std::uint32_t, nrf_gpio_pin_pull_t) {}
inline void nrf_gpio_pin_set(std::uint32_t) {}
inline void nrf_gpio_pin_clear(std::uint32_t) {}
inline std::uint32_t nrf_gpio_pin_read(std::uint32_t)
{
  return 0U;
}
inline void nrf_gpio_pin_toggle(std::uint32_t) {}

using nrf_saadc_value_t = std::int16_t;
enum nrf_saadc_resolution_t
{
  NRF_SAADC_RESOLUTION_8BIT,
  NRF_SAADC_RESOLUTION_10BIT,
  NRF_SAADC_RESOLUTION_12BIT,
  NRF_SAADC_RESOLUTION_14BIT
};
constexpr auto NRF_SAADC_MODE_SINGLE_ENDED = 0U, NRF_SAADC_MODE_DIFFERENTIAL = 1U;
constexpr auto NRF_SAADC_OVERSAMPLE_DISABLED = 0U, NRF_SAADC_BURST_DISABLED = 0U;
constexpr auto NRF_SAADC_TASK_SAMPLE = 4U;
constexpr auto NRF_SAADC_SAMPLERATE_CC_MIN = 80U, NRF_SAADC_SAMPLERATE_CC_MAX = 2047U;
struct nrfx_saadc_channel_t
{
  struct
  {
    std::uint32_t mode{};
  } channel_config;
  std::uint8_t channel_index{};
};
struct nrfx_saadc_adv_config_t
{
  std::uint32_t oversampling{}, burst{};
  std::uint16_t internal_timer_cc{};
  bool start_on_end{};
};
enum nrfx_saadc_evt_type_t
{
  NRFX_SAADC_EVT_DONE,
  NRFX_SAADC_EVT_LIMIT,
  NRFX_SAADC_EVT_CALIBRATEDONE,
  NRFX_SAADC_EVT_BUF_REQ,
  NRFX_SAADC_EVT_READY,
  NRFX_SAADC_EVT_FINISHED
};
struct nrfx_saadc_evt_t
{
  nrfx_saadc_evt_type_t type{};
  struct
  {
    struct
    {
      nrf_saadc_value_t* p_buffer{};
      std::uint16_t size{};
    } done;
  } data;
};
inline bool fake_saadc_initialized{};
inline int fake_saadc_error{};
inline bool fake_saadc_complete_on_trigger{};
inline nrf_saadc_value_t fake_saadc_trigger_sample{};
inline std::uint32_t fake_saadc_uninits{}, fake_saadc_buffers{};
inline void (*fake_saadc_callback)(const nrfx_saadc_evt_t*){};
inline nrf_saadc_value_t* fake_saadc_buffer{};
inline std::uint16_t fake_saadc_size{};
inline bool nrfx_saadc_init_check()
{
  return fake_saadc_initialized;
}
inline int nrfx_saadc_init(std::uint8_t)
{
  fake_saadc_initialized = true;
  return 0;
}
inline void nrfx_saadc_uninit()
{
  ++fake_saadc_uninits;
  fake_saadc_initialized = false;
}
inline int nrfx_saadc_channels_config(const nrfx_saadc_channel_t*, std::size_t)
{
  return 0;
}
inline int nrfx_saadc_simple_mode_set(std::uint32_t, nrf_saadc_resolution_t, std::uint32_t,
                                      void (*callback)(const nrfx_saadc_evt_t*))
{
  fake_saadc_callback = callback;
  return fake_saadc_error;
}
inline int nrfx_saadc_advanced_mode_set(std::uint32_t, nrf_saadc_resolution_t, const nrfx_saadc_adv_config_t*,
                                        void (*callback)(const nrfx_saadc_evt_t*))
{
  fake_saadc_callback = callback;
  return fake_saadc_error;
}
inline int nrfx_saadc_buffer_set(nrf_saadc_value_t* buffer, std::uint16_t count)
{
  fake_saadc_buffer = buffer;
  fake_saadc_size = count;
  ++fake_saadc_buffers;
  return fake_saadc_error;
}
inline int nrfx_saadc_mode_trigger()
{
  if (fake_saadc_error == 0 && fake_saadc_complete_on_trigger)
  {
    *fake_saadc_buffer = fake_saadc_trigger_sample;
    nrfx_saadc_evt_t event{};
    event.type = NRFX_SAADC_EVT_DONE;
    event.data.done.p_buffer = fake_saadc_buffer;
    event.data.done.size = fake_saadc_size;
    fake_saadc_callback(&event);
  }
  return fake_saadc_error;
}
inline void nrfx_saadc_irq_handler() {}
inline std::uint32_t nrf_saadc_task_address_get(void*, std::uint32_t)
{
  return 0x40007004U;
}
#define NRF_SAADC nullptr

struct NRF_I2S_Type
{
};
inline NRF_I2S_Type fake_i2s_reg{};
#define NRF_I2S (&fake_i2s_reg)
constexpr auto NRF_I2S_PIN_NOT_CONNECTED = UINT32_MAX;
constexpr auto NRF_I2S_MODE_MASTER = 0U, NRF_I2S_MODE_SLAVE = 1U;
constexpr auto NRF_I2S_FORMAT_I2S = 0U, NRF_I2S_FORMAT_ALIGNED = 1U;
constexpr auto NRF_I2S_ALIGN_RIGHT = 0U, NRF_I2S_ALIGN_LEFT = 1U;
constexpr auto NRF_I2S_SWIDTH_16BIT = 1U, NRF_I2S_CHANNELS_STEREO = 0U;
constexpr auto NRFX_I2S_STATUS_TRANSFER_STOPPED = 1U, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED = 2U;
#define I2S_RXTXD_MAXCNT_MAXCNT_Msk 16383U
#define I2S_RXTXD_MAXCNT_MAXCNT_Pos 0U
struct nrfx_i2s_clk_params_t
{
  std::uint32_t source_clock{}, sample_rate{}, sample_width{};
  bool bypass{};
};
struct nrfx_i2s_config_t
{
  std::uint32_t sck_pin{}, lrck_pin{}, sdout_pin{}, sdin_pin{}, mck_pin{}, irq_priority{}, mode{};
  std::uint32_t format{}, alignment{}, sample_width{}, channels{}, prescalers{};
};
struct nrfx_i2s_t
{
  NRF_I2S_Type* p_reg;
  struct
  {
    bool initialized{};
  } cb;
};
struct nrfx_i2s_buffers_t
{
  const std::uint32_t* p_tx_buffer{};
  std::uint32_t* p_rx_buffer{};
  std::uint16_t buffer_size{};
};
inline void (*fake_i2s_callback)(const nrfx_i2s_buffers_t*, std::uint32_t){};
inline nrfx_i2s_buffers_t fake_i2s_first{}, fake_i2s_next{};
inline std::uint32_t fake_i2s_stops{}, fake_i2s_uninits{};
inline int fake_i2s_error{};
inline bool fake_i2s_complete_on_irq{};
inline std::uint8_t fake_i2s_irq_phase{};
inline bool nrfx_i2s_init_check(const nrfx_i2s_t* i2s)
{
  return i2s->cb.initialized;
}
inline int nrfx_i2s_prescalers_calc(const nrfx_i2s_clk_params_t*, std::uint32_t*)
{
  return 0;
}
inline int nrfx_i2s_init(nrfx_i2s_t* i2s, const nrfx_i2s_config_t*,
                         void (*callback)(const nrfx_i2s_buffers_t*, std::uint32_t))
{
  i2s->cb.initialized = true;
  fake_i2s_callback = callback;
  return 0;
}
inline int nrfx_i2s_start(nrfx_i2s_t*, const nrfx_i2s_buffers_t* buffers, std::uint32_t)
{
  fake_i2s_first = *buffers;
  fake_i2s_irq_phase = 0U;
  return fake_i2s_error;
}
inline int nrfx_i2s_next_buffers_set(nrfx_i2s_t*, const nrfx_i2s_buffers_t* buffers)
{
  fake_i2s_next = *buffers;
  return fake_i2s_error;
}
inline void nrfx_i2s_stop(nrfx_i2s_t*)
{
  ++fake_i2s_stops;
}
inline void nrfx_i2s_uninit(nrfx_i2s_t* i2s)
{
  ++fake_i2s_uninits;
  i2s->cb.initialized = false;
}
inline void nrfx_i2s_irq_handler(nrfx_i2s_t*)
{
  if (!fake_i2s_complete_on_irq)
    return;
  const nrfx_i2s_buffers_t empty{};
  switch (fake_i2s_irq_phase++)
  {
  case 0U:
    fake_i2s_callback(&empty, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
    break;
  case 1U:
    fake_i2s_callback(&fake_i2s_first, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
    break;
  case 2U:
    fake_i2s_callback(&empty, NRFX_I2S_STATUS_TRANSFER_STOPPED);
    break;
  default:
    break;
  }
}
