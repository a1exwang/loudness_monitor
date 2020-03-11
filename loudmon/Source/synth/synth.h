#pragma once
#include <JuceHeader.h>

#include <type_traits>
#include "../loudmon/log_slider.h"
#include "../loudmon/oscilloscope.h"
#include "waveform.h"

template <typename ValueType>
class SynthParameterRaw {
 public:
  SynthParameterRaw(
      const std::string& name,
      ValueType min,
      ValueType max,
      ValueType interval,
      ValueType default_value,
      bool log_scale)
      :name_(name), min_(min), max_(max), interval_(interval), default_value_(default_value), log_scale_(log_scale) {
  }
  const std::string &name() const {
    return name_;
  }
  ValueType min() const { return min_; }
  ValueType max() const { return max_; }
  ValueType interval() const { return interval_; }
  ValueType default_value() const { return default_value_; }
  bool log_scale() const { return log_scale_; }
 private:
  std::string name_;
  ValueType min_, max_, interval_, default_value_;
  bool log_scale_;
};
template <typename ValueType, typename ControlType>
class SynthParameter : public juce::Component {
 public:
  SynthParameter(
      ControlType *ctrl, /*non-owning*/
      SynthParameterRaw<ValueType> p);
  void resized() override {
    auto area = getLocalBounds();
    name_label_.setBounds(area.removeFromTop(15));
    if (param_.log_scale()) {
      log_slider_->setBounds(area);
    } else {
      slider_->setBounds(area);
    }
  }
  ValueType value() const {
    return value_;
  }
 private:
  ControlType *ctrl_;
  Label name_label_;
  std::unique_ptr<Slider> slider_;
  std::unique_ptr<LogSlider> log_slider_;
  SynthParameterRaw<ValueType> param_;
  std::atomic<ValueType> value_;
};

#define SYNTH_PARAM(control_type, type, var, display_name, min, max, interval, default_value) \
  SynthParameter<type, control_type> var = SynthParameter<type, control_type>(this, SynthParameterRaw<type>(display_name, min, max, interval, default_value, false))

#define SYNTH_PARAM_LOG(control_type, type, var, display_name, min, max, interval, default_value) \
  SynthParameter<type, control_type> var = SynthParameter<type, control_type>(this, SynthParameterRaw<type>(display_name, min, max, interval, default_value, true))


class SynthControl : public juce::Component {
 public:
  explicit SynthControl(float sample_rate = 44100.0f);
  void resized() override;
  void add_component(juce::Component *p);

  void update_waveform() {
    auto [data, size] = waveform().get_original_waveform();
    oscilloscope_.set_values(data, size);
    oscilloscope_.set_x_slider_range(0, size);
  }

  void set_sample_rate(float sample_rate) {
    sample_rate_ = sample_rate;
  }

  WaveForm &waveform() {
    return *waveform_ptr_;
  }
 private:
  // Waveform related
  ComboBox waveform_select_;
  OscilloscopeComponent oscilloscope_;
  float sample_rate_;
  std::shared_ptr<WaveForm> waveform_ptr_;
  std::vector<std::tuple<std::string, std::function<std::unique_ptr<WaveForm>()>>> available_waveforms_ = {
      {"Sine", [this](){ return std::make_unique<SineWave>(sample_rate_, 256, 12*10, 12*10*8); }},
      {"Saw", [this](){ return std::make_unique<SawWave>(sample_rate_, 256, 12*10, 12*10*8); }}
  };

  std::vector<juce::Component*> components_;
 public:
  // These paramaters must be place after components_
  SYNTH_PARAM(SynthControl, float, amp, "Amp(dB)", -50, 10, 0.1f, -15);
  SYNTH_PARAM(SynthControl, int, harmonics, "Harmonics", 1, 10, 1, 1);
  SYNTH_PARAM(SynthControl, int, harmonic_diff, "Harmonic Diff", 1, 10, 1, 1);
  SYNTH_PARAM(SynthControl, int, freq_width, "Frequency Width", 1, 10, 1, 5);
};

template<typename ValueType, typename ControlType>
SynthParameter<ValueType, ControlType>::SynthParameter(
    ControlType *ctrl,
    SynthParameterRaw<ValueType> p)
    :ctrl_(ctrl), param_(std::move(p)) {

  if (param_.log_scale()) {
    log_slider_ = std::make_unique<LogSlider>();
    log_slider_->setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    log_slider_->setRangeLogarithm(static_cast<double>(param_.min()), static_cast<double>(param_.max()));
    log_slider_->setValue(param_.default_value());
    log_slider_->setOnValueChange([this]() {
      value_ = static_cast<ValueType>(log_slider_->getValue());
    });
    addAndMakeVisible(*log_slider_);
  } else {
    slider_ = std::make_unique<juce::Slider>();
    slider_->setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    slider_->setRange(param_.min(), param_.max(), param_.interval());
    slider_->setValue(param_.default_value());
    slider_->onValueChange = [this]() {
      value_ = static_cast<ValueType>(slider_->getValue());
    };
    addAndMakeVisible(*slider_);
  }
  addAndMakeVisible(name_label_);
  name_label_.setText(param_.name(), juce::dontSendNotification);
  name_label_.setJustificationType(juce::Justification::verticallyCentred);
  ctrl_->add_component(this);
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

  void render_note(SynthControl *control, AudioBuffer<float> &output_buffer);

  void renderNextBlock (AudioBuffer<float>& output_buffer,
                        int startSample,
                        int num_sample) override;

 private:
  void clear_note() {
    clearCurrentNote();
    waveform_voice_ = nullptr;
  }

 private:
  float frequency;
  size_t sample_pos_;
  ADSR adsr_;
  std::function<SynthControl *()> get_control_;
  std::unique_ptr<WaveFormVoice> waveform_voice_;
};

