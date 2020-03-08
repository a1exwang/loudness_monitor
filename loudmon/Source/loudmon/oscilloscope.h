#pragma once

#include <JuceHeader.h>
#include "plot.h"
#include "log_slider.h"

class OscilloscopeComponent :public juce::Component {
 public:
  explicit OscilloscopeComponent(size_t max_samples) :y_slider_("y") {
    addAndMakeVisible(plot_);
    addAndMakeVisible(x_slider_);
    x_slider_.onValueChange = std::bind(&OscilloscopeComponent::on_slider_change, this);
    x_slider_.setRange(1, static_cast<double>(max_samples));
    x_slider_.setValue(static_cast<double>(max_samples));
    x_slider_.setSliderStyle(Slider::SliderStyle::LinearHorizontal);

    addAndMakeVisible(y_slider_);
    y_slider_.setOnValueChange(std::bind(&OscilloscopeComponent::on_slider_change, this));
    y_slider_.setRangeLogarithm(0.001, 1.1);
    y_slider_.setValue(1);
    y_slider_.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
  }

  void on_slider_change() {
    auto x_value = x_slider_.getValue();
    auto y_value = y_slider_.getValue();
    plot_.set_value_range(0, x_value, -y_value, y_value, false, false);
  }

  void set_values(const float *ptr, size_t size) {
    std::unique_lock<spinlock> _(lock_);
    values_.resize(size);
    std::copy(ptr, ptr+size, values_.begin());
    plot_.clear();
    std::vector<std::tuple<double, double>> values;
    values.reserve(values_.size());
    for (size_t i = 0; i < values_.size(); i++) {
      values.emplace_back(i, values_[i]);
    }
    plot_.add_new_values("osc", values);
  }

  void resized() override {
    auto area = getLocalBounds();
    title_.setBounds(area.removeFromTop(20));
    x_slider_.setBounds(area.removeFromTop(20));
    y_slider_.setBounds(area.removeFromTop(50));
    plot_.setBounds(area);
  }

 private:

  juce::Slider x_slider_;
  LogSlider y_slider_;
  std::vector<float> values_;
  spinlock lock_;
  Label title_;
  PlotComponent plot_;
};
