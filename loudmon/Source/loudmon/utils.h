#pragma once

#include <string>
#include <atomic>

std::string compact_value_text(float f);

class spinlock {
 public:
  void lock() {
    while (!flag_.test_and_set(std::memory_order_acq_rel));
  }
  void unlock() {
    flag_.clear(std::memory_order_release);
  }
  bool try_lock() {
    return flag_.test_and_set(std::memory_order_acq_rel);
  }
 private:
  std::atomic_flag flag_;
};


template <typename T>
T clip(T value, T min, T max) {
  if (value > max) {
    return max;
  } else if (value < min) {
    return min;
  } else {
    return value;
  }
}
