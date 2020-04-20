#pragma once

#include <JuceHeader.h>


class UIUpdater :public juce::Timer {
 public:
  explicit UIUpdater(float fps = 30);
  ~UIUpdater() override;
  void timerCallback() override;

  void enqueue_ui(std::function<void()> action);
  void enqueue_ui_processing(std::function<void()> action);

  float get_average_ui_latency() const {
    return average_ui_latency_;
  }
  size_t get_ui_queue_size() const {
    return last_ui_queue_size_;
  }
  size_t get_ui_queue_loss() const {
    return ui_queue_loss_;
  }

  float get_average_ui_processing_latency() const {
    return average_ui_processing_latency_;
  }

 private:
  void ui_processing_worker_callback();
 private:
  size_t max_actions_per_interval_ = 256;
  size_t queue_max_size_ = 256;
  size_t ui_processing_queue_max_size_ = 4096;
  size_t thread_count_ = 1;

  // UI jobs
  std::list<std::tuple<std::function<void()>, std::chrono::high_resolution_clock::time_point>> queued_actions;
  std::atomic<float> average_ui_latency_ = 0;
  std::atomic<size_t> ui_queue_loss_ = 0;
  std::atomic<size_t> last_ui_queue_size_ = 0;
  std::vector<float> ui_latencies_ = std::vector<float>(16);
  size_t ui_latency_index_ = 0;
  std::mutex queue_lock;

  // UI processing workers
  std::atomic<bool> worker_quit_ = false;
  std::atomic<float> average_ui_processing_latency_ = 0;
  std::vector<float> ui_processing_latencies_ = std::vector<float>(16);
  size_t ui_processing_latency_index_ = 0;
  std::vector<std::thread> thread_pool_;
  std::list<std::tuple<std::function<void()>, std::chrono::high_resolution_clock::time_point>> ui_processing_queue;
  std::mutex ui_processing_queue_lock;
};