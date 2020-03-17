#pragma once

#include <JuceHeader.h>


class UIUpdater :public juce::Timer {
 public:
  explicit UIUpdater(float fps = 30);
  void timerCallback() override;

  void enqueue(std::function<void()> action);
  bool try_dequeue();
 private:
  std::list<std::function<void()>> queued_actions;
  std::mutex queue_lock;
  size_t max_actions_per_interval_ = 64;
};