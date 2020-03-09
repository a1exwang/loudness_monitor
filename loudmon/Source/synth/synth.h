#pragma once
#include <JuceHeader.h>

#include <type_traits>
#include "../loudmon/log_slider.h"
class SynthControl;

template <typename ValueType>
class SynthParameter : public juce::Component {
 public:
  SynthParameter(
      SynthControl *ctrl, /*non-owning*/
      const std::string& name,
      ValueType min, ValueType max, ValueType interval,
      ValueType default_value, bool log_scale);
  void resized() override {
    auto area = getLocalBounds();
    name_.setBounds(area.removeFromTop(15));
    if (use_log_slider_) {
      log_slider_->setBounds(area);
    } else {
      slider_->setBounds(area);
    }
  }
  ValueType value() const {
    return value_;
  }
 private:
  SynthControl *ctrl_;
  Label name_;
  bool use_log_slider_;
  std::unique_ptr<Slider> slider_;
  std::unique_ptr<LogSlider> log_slider_;
  std::atomic<ValueType> value_;
};

class SynthControl : public juce::Component {
 public:
  SynthControl() {
    for (auto &c : components_) {
      addAndMakeVisible(c);
    }
  }
  void resized() override;

 private:
  template<typename T>
  friend class SynthParameter;
  std::vector<juce::Component*> components_;

  // These paramaters must be place after components_
 public:
  SynthParameter<float> amp = SynthParameter<float>(this, "Amp(dB)", -50, 10, 0.1f, -15, false);
  SynthParameter<int> harmonics = SynthParameter<int>(this, "Harmonics", 1, 10, 1, 1, false);
  SynthParameter<int> harmonic_diff = SynthParameter<int>(this, "Harmonic Diff", 1, 10, 1, 1, false);
  SynthParameter<int> freq_width = SynthParameter<int>(this, "Frequency Width", 1, 10, 1, 5, false);
};

template<typename ValueType>
SynthParameter<ValueType>::SynthParameter(
    SynthControl *ctrl,
    const std::string& name,
    ValueType min,
    ValueType max,
    ValueType interval,
    ValueType default_value,
    bool log_scale)
    :ctrl_(ctrl), use_log_slider_(log_scale), value_(default_value) {

  if (use_log_slider_) {
    log_slider_ = std::make_unique<LogSlider>();
    log_slider_->setRangeLogarithm(static_cast<double>(min), static_cast<double>(max));
    log_slider_->setValue(default_value);
    log_slider_->setOnValueChange([this]() {
      value_ = static_cast<ValueType>(log_slider_->getValue());
    });
    addAndMakeVisible(*log_slider_);
  } else {
    slider_ = std::make_unique<juce::Slider>();
    slider_->setRange(min, max, interval);
    slider_->setValue(default_value);
    slider_->onValueChange = [this]() {
      value_ = static_cast<ValueType>(slider_->getValue());
    };
    addAndMakeVisible(*slider_);
  }
  addAndMakeVisible(name_);
  name_.setText(name, juce::dontSendNotification);
  name_.setJustificationType(juce::Justification::verticallyCentred);
  ctrl_->components_.push_back(this);
}

class MPESimpleVoice  : public MPESynthesiserVoice {
 public:
  MPESimpleVoice(std::function<SynthControl *()> get_control);
  void noteStarted() override;

  void noteStopped (bool allowTailOff) override;

  void notePressureChanged() override;

  void notePitchbendChanged() override;

  void noteTimbreChanged()   override {}
  void noteKeyStateChanged() override {}

  void renderNextBlock (AudioBuffer<float>& output_buffer,
                        int startSample,
                        int num_sample) override;

 private:
  float frequency;
  size_t sample_pos_;
  ADSR adsr_;
  std::function<SynthControl *()> get_control_;
};

