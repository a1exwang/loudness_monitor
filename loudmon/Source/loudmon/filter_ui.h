#pragma once

#include <JuceHeader.h>

#include <utility>
#include <list>
#include "log_slider.h"
#include "utils.h"
#include "plot.h"

template <typename T>
class FilterSeries {
 public:

  void reset() {
    for (auto &filter : filters_) {
      filter.reset();
    }
  }

  T processSample(T value) noexcept {
    for (auto &filter : filters_) {
      value = filter.processSample(value);
    }
    return value;
  }

  template <typename Context>
  void process (Context &context) {
    for (auto &filter : filters_) {
      filter.template process<Context>(context);
    }
  }

  std::vector<dsp::IIR::Filter<T>> filters_;
};


template<typename T>
using PeakFilter = dsp::ProcessorChain<dsp::IIR::Filter<T>, dsp::IIR::Filter<T>>;

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
    plot_.setBounds(area);
  }

  void filter_parameter_changed() {
    set_parameters(frequency_slider_.getValue(), quality_slider_.getValue());
  }

  template <typename Context>
  void process(int channel, Context &context) {
    std::unique_lock<spinlock> _(filters_lock_);
    filters_[channel].process(context);
  }

 private:
  void set_parameters(double frequency, float quality);

 private:
  double sample_rate_;
  size_t input_channels_;
  double frequency_, quality_;
  size_t fft_order_, fft_size_;

  LogSlider frequency_slider_, quality_slider_;
  PlotComponent plot_;
  std::vector<PeakFilter<float>> filters_;
//  IIRFilter filter_for_display_;
  PeakFilter<float> filter_for_display_;
  spinlock filters_lock_;

  dsp::FFT forward_fft;
  AudioBuffer<float> spectrum_;

  // UI values
  float slider_height_ = 120;
  float slider_count_ = 4;
};
