#include "ui_updater.h"

UIUpdater::UIUpdater(float fps) {
  startTimer(static_cast<int>(1000.0f / fps));
}

void UIUpdater::enqueue(std::function<void()> action) {
  std::unique_lock<std::mutex> _(queue_lock);
  queued_actions.emplace_back(std::move(action));
}

bool UIUpdater::try_dequeue() {
  std::function<void()> action;
  {
    std::unique_lock<std::mutex> _(queue_lock);
    if (queued_actions.empty()) {
      return false;
    }
    action = std::move(queued_actions.front());
    queued_actions.pop_front();
  }
  action();
  return true;
}
void UIUpdater::timerCallback() {
  for (size_t i = 0; i < max_actions_per_interval_; i++) {
    if (!try_dequeue()) {
      break;
    }
  }
}
