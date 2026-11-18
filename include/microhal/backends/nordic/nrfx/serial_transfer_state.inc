// SPDX-License-Identifier: Apache-2.0
struct transfer_state
{
  std::atomic<bool> active{false};
  std::atomic<bool> complete{false};
  std::atomic<std::uint32_t> generation{};
  status result{status::error};
  void (*completion)(status) noexcept {nullptr};

  /**
   * @brief Claims an idle transfer state and installs its completion callback.
   * @param[in] callback Optional callback invoked by @ref finish; null selects no callback.
   * @return True when the state was claimed, or false if a transfer is already active.
   */
  bool begin(void (*callback)(status) noexcept) noexcept
  {
    if (active.exchange(true))
      return false;
    generation.fetch_add(1U, std::memory_order_release);
    complete.store(false);
    result = status::error;
    completion = callback;
    return true;
  }

  /**
   * @brief Publishes the result, releases the transfer, and invokes its callback.
   * @param[in] value Completion status delivered to the callback.
   * @note The callback runs in the caller's context, which may be an interrupt.
   */
  void finish(const status value) noexcept
  {
    result = value;
    const auto callback = completion;
    completion = nullptr;
    complete.store(true, std::memory_order_release);
    active.store(false);
    if (callback != nullptr)
      callback(value);
  }
};
