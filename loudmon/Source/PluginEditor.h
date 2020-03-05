#pragma once

#include <list>
#include <string>
#include <map>
#include <utility>

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "loudmon/filter_ui.h"
#include "loudmon/oscilloscope.h"

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
  void set_latency(float ms, float max_expected, size_t late) {
    latency_ms_ = ms;
    latency_max_expected_ = max_expected;
    late_block_count_ = late;
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
       << "Latency(out/in/max/late): " << std::fixed << std::setprecision(2) << std::setfill('0')
       << process_block_interval_*1000 << "/" << latency_ms_ << "/" << latency_max_expected_ << "/" << late_block_count_ << std::endl
       << "Sample Rate: " << sample_rate_ << ", Samples per Block: " << samples_per_block_ << ", Input channels: " << input_channels_ << std::endl
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
  size_t late_block_count_;

  std::map<std::string, std::string> display_values_;
  std::list<std::string> display_value_keys_in_order_;
  std::function<void()> update_callback_;
};

class MainComponent : public AudioProcessorEditor, public juce::Timer, public juce::MenuBarModel {
 public:
  explicit MainComponent(NewProjectAudioProcessor& p);
  ~MainComponent() override;

  std::vector<std::tuple<std::string, std::vector<std::tuple<std::string, std::function<void()>>>>> menu_items_ = {
      {
          "Debug",
          {
              {"Show Debug Window", std::bind(&MainComponent::toggle_debug_window, this)}
             }
      }
  };

  StringArray getMenuBarNames() override {
    StringArray names;
    for (auto &[name, _] : menu_items_) {
      names.add(name);
    }
    return names;
  }

  PopupMenu getMenuForIndex(int topLevelMenuIndex, const String &menuName) override {
    PopupMenu menu;
    // An ID of 0 is used as a return value to indicate that the user
    // didn't pick anything, so you shouldn't use it as the ID for an item..
    size_t i = 1;
    for (auto &item : std::get<1>(menu_items_[topLevelMenuIndex])) {
      auto& [sub_menu_name, action] = item;
      juce::PopupMenu::Item popup_item(sub_menu_name);
      popup_item.setID(i++);
      menu.addItem(popup_item);
    }
    return menu;
  }

  void menuItemSelected (int menu_item_id, int top_level_menu_index) override {
    if (top_level_menu_index >= 0 && top_level_menu_index < menu_items_.size()) {
      auto menu_item = std::get<1>(menu_items_[top_level_menu_index]);
      auto menu_item_index = menu_item_id-1;
      if (menu_item_index >= 0 && menu_item_index < menu_item.size()) {
        auto action = std::get<1>(menu_item[menu_item_index]);
        if (action) {
          action();
        }
      }
    }
  }

  void toggle_debug_window();

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
  void set_latency_ms(float ms, float max_expected, size_t late) {
    enqueue([this, ms, max_expected, late]() {
      main_info_.set_latency(ms, max_expected, late);
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

  void send_block(AudioBuffer<float> buffer) {
    enqueue([this, buffer{std::move(buffer)}]() {
      buffer_ = buffer;
      oscilloscope_.set_values(buffer_.getArrayOfReadPointers()[0], buffer_.getNumSamples());
    });
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
    menu_bar_.setBounds(area.removeFromTop (LookAndFeel::getDefaultLookAndFeel().getDefaultMenuBarHeight()));

    auto widget_height = 200;
    auto total_height = area.getHeight();
    info_text.setBounds(area.removeFromTop(total_height/4));
    oscilloscope_.setBounds(area.removeFromTop(total_height/4));

    if (filter) {
      filter->setBounds(area);
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
  juce::MenuBarComponent menu_bar_;

  Label info_text;
  OscilloscopeComponent oscilloscope_;
  std::list<std::function<void()>> queued_actions;
  std::mutex queue_lock;
  Component::SafePointer<DebugOutputWindow> debug_window;
  bool debug_window_visible_ = false;
  std::unique_ptr<FilterTransferFunctionComponent> filter;
  std::chrono::high_resolution_clock::time_point last_paint_time;
  AudioBuffer<float> buffer_;
  //==============================================================================
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

