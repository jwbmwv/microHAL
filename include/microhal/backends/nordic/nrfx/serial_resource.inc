// SPDX-License-Identifier: Apache-2.0
// Serial personalities on the same register block must not be enabled together.
inline std::array<std::atomic<std::uintptr_t>, 32U> serial_resources{};
inline std::atomic_flag serial_resource_lock = ATOMIC_FLAG_INIT;
inline status serial_claim(const void* peripheral, std::uintptr_t owner) noexcept
{
  // Never spin: an interrupt may preempt another initialization on this core.
  if (serial_resource_lock.test_and_set(std::memory_order_acquire))
    return status::busy;
  struct unlock
  {
    ~unlock()
    {
      serial_resource_lock.clear(std::memory_order_release);
    }
  } guard;
  const auto address = reinterpret_cast<std::uintptr_t>(peripheral);
  for (auto& resource : serial_resources)
    if ((resource.load(std::memory_order_acquire) & ~std::uintptr_t{7U}) == address)
      return status::busy;
  for (auto& resource : serial_resources)
  {
    std::uintptr_t empty = 0U;
    if (resource.compare_exchange_strong(empty, address | owner, std::memory_order_acq_rel))
      return status::ok;
  }
  return status::busy;
}
inline void serial_release(const void* peripheral, std::uintptr_t owner) noexcept
{
  const auto token = reinterpret_cast<std::uintptr_t>(peripheral) | owner;
  for (auto& resource : serial_resources)
  {
    auto expected = token;
    if (resource.compare_exchange_strong(expected, 0U, std::memory_order_acq_rel))
      return;
  }
}
template <typename Initialize>
status serial_initialize(const void* peripheral, std::uintptr_t owner, Initialize initialize) noexcept
{
  const auto claimed = serial_claim(peripheral, owner);
  if (claimed != status::ok)
    return claimed;
  const auto result = driver_result(initialize());
  if (result != status::ok)
    serial_release(peripheral, owner);
  return result;
}
