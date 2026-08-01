// SPDX-License-Identifier: Apache-2.0
/**
 * @file status.hpp
 * @brief Error-reporting vocabulary types for the microHAL public API.
 * @details microHAL is compiled with `-fno-exceptions`, so every fallible operation reports
 *          outcomes through @ref microhal::status or @ref microhal::result. Backends may report
 *          success as `void`, `bool`, or @ref microhal::status; the adapters in this header
 *          normalise all three so facades can propagate a single vocabulary type.
 * @copyright Copyright (c) 2026 James Baldwin
 * @author microHAL contributors
 * @ingroup cpp_api
 */
#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>

namespace microhal
{

/**
 * @brief Outcome of a fallible peripheral operation.
 */
enum class status : std::uint8_t
{
  ok = 0,           ///< Operation completed successfully.
  error,            ///< Unspecified backend failure.
  timeout,          ///< Operation exceeded its configured timeout.
  busy,             ///< Peripheral is currently occupied by another transfer.
  unsupported,      ///< Backend does not implement the requested operation.
  invalid_argument, ///< Caller supplied an out-of-range or malformed argument.
  io_error          ///< Bus-level failure such as an I2C NAK or CRC mismatch.
};

/// @brief True when @p value represents success.
[[nodiscard]] constexpr bool is_ok(const status value) noexcept
{
  return value == status::ok;
}

/// @brief Human-readable name for @p value, intended for logging and test output.
[[nodiscard]] constexpr std::string_view to_string(const status value) noexcept
{
  switch (value)
  {
  case status::ok:
    return "ok";
  case status::error:
    return "error";
  case status::timeout:
    return "timeout";
  case status::busy:
    return "busy";
  case status::unsupported:
    return "unsupported";
  case status::invalid_argument:
    return "invalid_argument";
  case status::io_error:
    return "io_error";
  }
  return "unknown";
}

/**
 * @brief A value paired with a @ref status, used by operations that return data.
 * @tparam T Trivially copyable payload type.
 */
template <typename T> class result final
{
public:
  using value_type = T;

  constexpr result(const T value) noexcept : value_{value}, status_{status::ok} {}

  constexpr result(const status error_status) noexcept : value_{}, status_{error_status} {}

  [[nodiscard]] constexpr bool has_value() const noexcept
  {
    return status_ == status::ok;
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept
  {
    return has_value();
  }

  [[nodiscard]] constexpr status error() const noexcept
  {
    return status_;
  }

  /// @brief Returns the payload. Only meaningful when @ref has_value is true.
  [[nodiscard]] constexpr T value() const noexcept
  {
    return value_;
  }

  /// @brief Returns the payload, or @p fallback when the operation failed.
  [[nodiscard]] constexpr T value_or(const T fallback) const noexcept
  {
    return has_value() ? value_ : fallback;
  }

private:
  T value_{};
  status status_{status::error};
};

namespace detail
{

/**
 * @brief Normalises a backend call into a @ref status.
 * @details Backends predate this vocabulary and variously return `void` (operation cannot fail),
 *          `bool` (success flag), or @ref status. Centralising the conversion here keeps the
 *          facades uniform without forcing a lockstep rewrite of every backend.
 */
template <typename Fn> [[nodiscard]] constexpr status as_status(Fn&& fn) noexcept
{
  using return_type = std::invoke_result_t<Fn>;
  if constexpr (std::is_void_v<return_type>)
  {
    std::forward<Fn>(fn)();
    return status::ok;
  }
  else if constexpr (std::is_same_v<std::remove_cv_t<return_type>, status>)
  {
    return std::forward<Fn>(fn)();
  }
  else if constexpr (std::is_same_v<std::remove_cv_t<return_type>, bool>)
  {
    return std::forward<Fn>(fn)() ? status::ok : status::error;
  }
  else
  {
    // Backend reported a payload rather than a success flag; the value is retrieved separately.
    static_cast<void>(std::forward<Fn>(fn)());
    return status::ok;
  }
}

/**
 * @brief Normalises a value-returning backend call into a @ref result.
 */
template <typename T, typename Fn> [[nodiscard]] constexpr result<T> as_result(Fn&& fn) noexcept
{
  using return_type = std::invoke_result_t<Fn>;
  if constexpr (std::is_same_v<std::remove_cv_t<return_type>, result<T>>)
  {
    return std::forward<Fn>(fn)();
  }
  else
  {
    return result<T>{static_cast<T>(std::forward<Fn>(fn)())};
  }
}

} // namespace detail

} // namespace microhal
