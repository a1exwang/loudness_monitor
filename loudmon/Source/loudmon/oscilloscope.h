#pragma once

#include <JuceHeader.h>
#include "plot.h"
#include "log_slider.h"

class OscilloscopeComponent :public juce::Component {
 public:
  explicit OscilloscopeComponent(size_t max_samples = 1) {
    addAndMakeVisible(plot_);
    addAndMakeVisible(x_slider_);
    x_slider_.onValueChange = std::bind(&OscilloscopeComponent::on_slider_change, this);
    x_slider_.setRange(0, static_cast<double>(max_samples));
    x_slider_.setValue(static_cast<double>(max_samples));
    x_slider_.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);

    addAndMakeVisible(y_slider_);
    y_slider_.setOnValueChange(std::bind(&OscilloscopeComponent::on_slider_change, this));
    y_slider_.setRangeLogarithm(0.001, 1.1);
    y_slider_.setValue(1);
    y_slider_.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
  }

  void set_yx_display_ratio(float ratio) {
    yx_display_ratio_ = ratio;
    resized();
  }

  void on_slider_change() {
    auto x_value = static_cast<float>(x_slider_.getValue());
    auto y_value = static_cast<float>(y_slider_.getValue());
    plot_.set_value_range(0, x_value, -y_value, y_value, false, false);
  }

  // non-owning
  void set_values(const float *ptr, size_t size) {
    std::unique_lock<spinlock> _(lock_);
    values_.resize(size);
    std::copy(ptr, ptr+size, values_.begin());
    plot_.clear();
    std::vector<std::tuple<float, float>> values;
    values.reserve(values_.size());
    for (size_t i = 0; i < values_.size(); i++) {
      values.emplace_back(static_cast<float>(i), values_[i]);
    }
    plot_.add_new_values("osc", values);
  }

  void set_x_slider_range(size_t min, size_t max) {
    x_slider_.setRange(double(min), double(max));
    x_slider_.setValue(double(max));
  }

  void resized() override {
    auto area = getLocalBounds();
    auto control_area = area.removeFromTop(40);
    x_slider_.setBounds(control_area.removeFromLeft(80));
    y_slider_.setBounds(control_area.removeFromLeft(80));
    if (yx_display_ratio_ > 0) {
      auto height = area.getWidth() * yx_display_ratio_;
      plot_.setBounds(area.removeFromTop(static_cast<int>(height)));
    } else {
      plot_.setBounds(area);
    }
  }

 private:

  juce::Slider x_slider_;
  LogSlider y_slider_;
  std::vector<float> values_;
  spinlock lock_;
  PlotComponent plot_;
  float yx_display_ratio_;
};
