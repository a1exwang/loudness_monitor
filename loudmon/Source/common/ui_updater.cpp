#include "ui_updater.h"
#include <numeric>

UIUpdater::UIUpdater(float fps) {
  startTimer(static_cast<int>(1000.0f / fps));
  for (size_t i = 0; i < thread_count_; i++) {
    thread_pool_.emplace_back(std::bind(&UIUpdater::ui_processing_worker_callback, this));
  }
}

UIUpdater::~UIUpdater() {
  worker_quit_ = true;
  for (auto &t : thread_pool_) {
    t.join();
  }
}

void UIUpdater::enqueue_ui(std::function<void()> action) {
  std::unique_lock<std::mutex> _(queue_lock);
  if (queued_actions.size() >= queue_max_size_) {
    queued_actions.pop_front();
    ui_queue_loss_++;
  }
  queued_actions.emplace_back(std::move(action), std::chrono::high_resolution_clock::now());
}

void UIUpdater::timerCallback() {
  for (size_t i = 0; i < max_actions_per_interval_; i++) {
    std::function<void()> action;
    {
      std::unique_lock<std::mutex> _(queue_lock);
      if (queued_actions.empty()) {
        break;
      }
      auto latency = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - std::get<1>(queued_actions.front()));
      ui_latencies_[ui_latency_index_++] = latency.count();
      ui_latency_index_ %= ui_latencies_.size();
      average_ui_latency_ = std::accumulate(ui_latencies_.begin(), ui_latencies_.end(), 0.0f) / ui_latencies_.size();
      action = std::move(std::get<0>(queued_actions.front()));
      queued_actions.pop_front();
      last_ui_queue_size_ = queued_actions.size();
    }
    action();
  }
}

void UIUpdater::enqueue_ui_processing(std::function<void()> action) {
  std::unique_lock<std::mutex> _(ui_processing_queue_lock);
  if (ui_processing_queue.size() >= ui_processing_queue_max_size_) {
    ui_processing_queue.pop_front();
  }
  ui_processing_queue.emplace_back(std::move(action), std::chrono::high_resolution_clock::now());
}
void UIUpdater::ui_processing_worker_callback() {
  while (!worker_quit_) {
    std::this_thread::yield();

    std::function<void()> action;
    {
      std::unique_lock<std::mutex> _(ui_processing_queue_lock);
      if (ui_processing_queue.empty()) {
        continue;
      }
      ui_processing_latencies_[ui_processing_latency_index_++] = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - std::get<1>(ui_processing_queue.front())).count();
      ui_processing_latency_index_ %= ui_processing_latencies_.size();
      average_ui_processing_latency_ = std::accumulate(ui_processing_latencies_.begin(), ui_processing_latencies_.end(), 0.0f) / ui_processing_latencies_.size();
      action = std::move(std::get<0>(ui_processing_queue.front()));
      ui_processing_queue.pop_front();
    }
    action();
  }
}
