#pragma once

#include <JuceHeader.h>

class LogSliderInternal : public juce::Slider {
 public:
  LogSliderInternal() {
    setSliderStyle(Slider::SliderStyle::LinearVertical);
  }

  void setRangeLogarithm(float min, float max) {
    setRange(min, max);
    auto mid = sqrt(min* max);
    setSkewFactorFromMidPoint(mid);
    setValue(mid);
    repaint();
  }

  String getTextFromValue(double) override;
};

class LogSlider : public juce::Component {
 public:
  explicit LogSlider(std::string name) : name_(std::move(name)) {
    label_.setText(name_, dontSendNotification);
    label_.setJustificationType(Justification::horizontallyCentred);
    addAndMakeVisible(label_);
    addAndMakeVisible(slider_);
    resize_children();
  }

  void setSliderStyle(juce::Slider::SliderStyle new_style) {
    slider_.setSliderStyle(new_style);
  }

  void resize_children() {
    auto area = getLocalBounds();
    label_.setBounds(area.removeFromTop(std::min(label_min_height, label_height_ratio*area.getHeight())));
    slider_.setBounds(area);
  }

  void setRangeLogarithm(float min, float max) {
    slider_.setRangeLogarithm(min, max);
  }

  void resized() override {
    resize_children();
  }

  double getValue() {
    return slider_.getValue();
  }

  void setValue(double value) {
    slider_.setValue(value);
  }

  void setOnValueChange(std::function<void()> callback) {
    slider_.onValueChange = std::move(callback);
  }

 private:
  std::string name_;
  juce::Label label_;
  LogSliderInternal slider_;
  double label_height_ratio = 0.3;
  double label_min_height = 30;
};

