#pragma once

#include <list>
#include <string>
#include <map>
#include <utility>

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "loudmon/filter_ui.h"

class DebugOutputWindow;
class MainInfo {
 public:
  explicit MainInfo(std::function<void()> update_callback) : update_callback_(std::move(update_callback)) { }
  void set_sample_rate(double sample_rate) {
    sample_rate_ = sample_rate;
    update_callback_();
  }
  void set_samples_per_block(size_t value) {
    samples_per_block_ = value;
    update_callback_();
  }
  void set_fps(float fps) {
    fps_ = fps;
    update_callback_();
  }
  void set_input_rms(std::vector<float> values) {
    input_rms_ = std::move(values);
    update_callback_();
  }
  void set_latency(float ms, float max_expected) {
    latency_ms_ = ms;
    latency_max_expected_ = max_expected;
    update_callback_();
  }
  void set_process_block_interval(float seconds) {
    process_block_interval_ = seconds;
    update_callback_();
  }
  void set_input_channels(size_t n) {
    input_channels_ = n;
    update_callback_();
  }

  void add_display_value(const std::string& key, float value) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3) << value;
    display_values_[key] = ss.str();
  }
  void add_display_value(const std::string& key, std::string value) {
    if (display_values_.find(key) == display_values_.end()) {
      display_value_keys_in_order_.push_back(key);
    }
    display_values_[key] = std::move(value);
  }

  std::string to_string() const {
    std::stringstream ss;
    ss << "FPS: " << fps_ << std::endl
       << "Latency(out/in/max): " << std::fixed << std::setprecision(2) << std::setfill('0')
       << process_block_interval_*1000 << "/" << latency_ms_ << "/" << latency_max_expected_ << "ms" << std::endl
       << "Sample Rate: " << sample_rate_ << std::endl
       << "Samples per Block: " << samples_per_block_ << std::endl
       << "Input channels: " << input_channels_ << std::endl
       << "Input RMS: ";
    for (float value : input_rms_) {
      ss << std::fixed << std::setprecision(2) << std::setfill('0') << value << "dB ";
    }
    ss << std::endl;
    for (auto &key : display_value_keys_in_order_) {
      ss << key << ": " << display_values_.at(key) << std::endl;
    }
    return ss.str();
  }
 private:
  friend class MainComponent;
  double sample_rate_ = 44100;
  size_t samples_per_block_ = 2048;
  size_t input_channels_ = 2;
  float fps_ = 0;
  std::vector<float> input_rms_;
  float latency_ms_ = 0, latency_max_expected_ = 0, process_block_interval_ = 0;

  std::map<std::string, std::string> display_values_;
  std::list<std::string> display_value_keys_in_order_;
  std::function<void()> update_callback_;
};

class MainComponent : public AudioProcessorEditor, public juce::Timer {
 public:
  explicit MainComponent(NewProjectAudioProcessor& p);
  ~MainComponent() override;

  /* Component callbacks, UI thread */
  void visibilityChanged() override;
  void paint(Graphics& g) override;
  void timerCallback() override {
    while (dequeue()) {}
  }
  void resized() override {
    resize_children();
  }

  /* May be in any thread */
  void set_input_rms(const std::vector<float>& values) {
    enqueue([this, values]() {
      main_info_.set_input_rms(values);
      repaint();
    });
  }
  void set_latency_ms(float ms, float max_expected) {
    enqueue([this, ms, max_expected]() {
      main_info_.set_latency(ms, max_expected);
      repaint();
    });
  }
  void set_process_block_interval(float seconds) {
    enqueue([this, seconds]() {
      main_info_.set_process_block_interval(seconds);
      repaint();
    });
  }
  void repaint_safe() {
    enqueue([this]() {

      info_text.setText(main_info_.to_string(), dontSendNotification);
      repaint();
    });
  }
  void prepare_to_play(double sample_rate, size_t samples_per_block, size_t input_channels) {
    enqueue([this, sample_rate, samples_per_block, input_channels]() {
      main_info_.set_sample_rate(sample_rate);
      main_info_.set_samples_per_block(samples_per_block);
      main_info_.set_input_channels(input_channels);

      if (filter) {
        removeChildComponent(filter.get());
      }
      filter = std::make_unique<FilterTransferFunctionComponent>(sample_rate, input_channels);
      addAndMakeVisible(*filter);
      resize_children();
    });
  }

  template <typename Context>
  void filter_process(int channel, Context &context) {
    if (filter) {
      filter->process(channel, context);
    }
  }

  void add_display_value(const std::string& key, std::string value) {
    enqueue([this, key, value{std::move(value)}]() {
      main_info_.add_display_value(key, value);
      repaint();
    });
  }
  void add_display_value(const std::string& key, float value) {
    enqueue([this, key, value{std::move(value)}]() {
      main_info_.add_display_value(key, value);
      repaint();
    });
  }

 protected:
  void resize_children() {
    auto area = getLocalBounds();
    auto widget_height = 200;
    info_text.setBounds(area.removeFromTop(widget_height));
    if (filter) {
      filter->setBounds(area.removeFromTop(500));
    }
  }

  void enqueue(std::function<void()> action) {
    std::unique_lock<std::mutex> _(queue_lock);
    queued_actions.emplace_back(std::move(action));
  }
  bool dequeue() {
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

 private:
  MainInfo main_info_;

  Label info_text;
  std::list<std::function<void()>> queued_actions;
  std::mutex queue_lock;
  Component::SafePointer<DebugOutputWindow> dw;
  std::unique_ptr<FilterTransferFunctionComponent> filter;
  std::chrono::high_resolution_clock::time_point last_paint_time;
  //==============================================================================
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

