#pragma once

#include <JuceHeader.h>

#include <utility>
#include "log_slider.h"

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

class FilterTransferFunctionComponent :public juce::Component {
 public:
  FilterTransferFunctionComponent(double sample_rate, size_t input_channels, size_t fft_order = 10);
  void paint(Graphics &g) override;
  void paint_transfer_function(Graphics &g, float left, float top, float width, float height);
  void resized() override {
    auto area = getLocalBounds();
    auto slider_area = area.removeFromTop(slider_height_);
    frequency_slider_.setBounds(slider_area.removeFromLeft(area.getWidth()/slider_count_));
    quality_slider_.setBounds(slider_area.removeFromLeft(area.getWidth()/slider_count_));
  }

  void filter_parameter_changed() {
    set_parameters(frequency_slider_.getValue(), quality_slider_.getValue());
  }

  float process(size_t channel, float value) {
    std::unique_lock<spinlock> _(filters_lock_);
    return filters_[channel].processSingleSampleRaw(value);
  }
 private:
  void set_parameters(double frequency, float quality);

 private:
  double sample_rate_;
  size_t input_channels_;
  double frequency_, quality_;
  size_t fft_order_, fft_size_;

  LogSlider frequency_slider_, quality_slider_;
  std::vector<IIRFilter> filters_;
  IIRFilter filter_for_display_;
  spinlock filters_lock_;

  dsp::FFT forward_fft;
  std::vector<float> spectrum_;

  // UI values
  float slider_height_ = 120;
  float slider_count_ = 4;
};
