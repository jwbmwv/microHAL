// SPDX-License-Identifier: Apache-2.0
struct transfer_state
{
  std::atomic<bool> active{false};
  std::atomic<bool> complete{false};
  status result{status::error};
  void (*completion)(status) noexcept {nullptr};

  bool begin(void (*callback)(status) noexcept) noexcept
  {
    if (active.exchange(true))
      return false;
    complete.store(false);
    result = status::error;
    completion = callback;
    return true;
  }
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
