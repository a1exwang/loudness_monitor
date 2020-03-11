#include "synth.h"
#include "../loudmon/debug_output.h"

#include <memory>

SynthControl::SynthControl(float sample_rate) :sample_rate_(sample_rate) {
  for (size_t i = 0; i < available_waveforms_.size(); i++) {
    waveform_select_.addItem(std::get<0>(available_waveforms_[i]), (int)i+1);
  }
  waveform_select_.onChange = [this]() {
    auto selected_id = waveform_select_.getSelectedId();
    if (selected_id && sample_rate_ != 0) {
      auto id = selected_id-1;
      waveform_ptr_ = std::get<1>(available_waveforms_[id])();
      update_waveform();
    }
  };
  oscilloscope_.set_yx_display_ratio(2 / juce::MathConstants<float>::twoPi);
  addAndMakeVisible(oscilloscope_);

  components_.push_back(&waveform_select_);
  for (auto &c : components_) {
    addAndMakeVisible(c);
  }
}

void SynthControl::resized() {
  auto area = getLocalBounds();
  oscilloscope_.setBounds(area.removeFromTop(area.getHeight()/2));

  auto control_area = area;
  size_t columns = 2;
  size_t rows = static_cast<size_t>(control_area.getHeight() / 50);
  size_t height = control_area.getHeight();
  size_t width = control_area.getWidth();

  auto it = components_.begin();
  for (size_t i = 0; i < columns; i++) {
    auto current_column = control_area.removeFromLeft(static_cast<int>((float)width / columns));
    for (int j = 0; j < rows; j++) {
      if (it == components_.end()) {
        break;
      }
      (*it)->setBounds(current_column.removeFromTop(static_cast<int>((float)height / rows)));
      it++;
    }
  }
}
void SynthControl::add_component(juce::Component *p) {
  components_.push_back(p);
}

MPESimpleVoice::MPESimpleVoice(std::function<SynthControl* ()> get_control) :get_control_(std::move(get_control)) {
  ADSR::Parameters params{};
  params.attack = 0.001f;
  params.decay = 0.1f;
  params.sustain = 0.5f;
  params.release = 0.02f;
  adsr_.setParameters(params);
}

void MPESimpleVoice::renderNextBlock(AudioBuffer<float> &output_buffer, int startSample, int num_sample) {
  std::stringstream ss;
  ss << "SimpleVoice::renderNextBlock " << this;
  log(10, ss.str());
  auto control = get_control_();
  if (!control) {
    return;
  }

  AudioBuffer<float> process_buffer(output_buffer.getNumChannels(), num_sample);
  process_buffer.clear();
  render_note(control, process_buffer);
  adsr_.applyEnvelopeToBuffer(output_buffer, 0, num_sample);
  if (!adsr_.isActive()) {
    clear_note();
  }
  for (int channel = 0; channel < output_buffer.getNumChannels(); channel++) {
    output_buffer.addFrom(channel, startSample, process_buffer, channel, 0, num_sample);
  }
  sample_pos_ += num_sample;
}

void MPESimpleVoice::noteStarted() {
  jassert (currentlyPlayingNote.isValid());
  jassert (currentlyPlayingNote.keyState == MPENote::keyDown
               || currentlyPlayingNote.keyState == MPENote::keyDownAndSustained);

  frequency = static_cast<float>(currentlyPlayingNote.getFrequencyInHertz());
  sample_pos_ = 0;
  adsr_.noteOn();
  auto control = get_control_();
  if (control) {
    waveform_voice_ = control->waveform().get_voice();
  }
}
void MPESimpleVoice::noteStopped(bool allowTailOff) {
  jassert (currentlyPlayingNote.keyState == MPENote::off);
  if (allowTailOff) {
    adsr_.noteOff();
  } else {
    clear_note();
  }
}
void MPESimpleVoice::notePressureChanged() {
}
void MPESimpleVoice::notePitchbendChanged() {
  frequency = static_cast<float>(currentlyPlayingNote.getFrequencyInHertz());
}

void MPESimpleVoice::render_note(SynthControl *control, AudioBuffer<float> &output_buffer) {
  auto num_samples = output_buffer.getNumSamples();
  auto &waveform = control->waveform();
  auto harmonics = control->harmonics.value();
  auto harmonic_diff = control->harmonic_diff.value();
  auto freq_width = control->freq_width.value();
  auto amp = pow(10.0f, control->amp.value()/10.0f);

  waveform_voice_->fill_next_samples(frequency, output_buffer.getWritePointer(0), num_samples);
  juce::FloatVectorOperations::multiply(output_buffer.getWritePointer(0), amp, num_samples);

  for (int channel = 1; channel < output_buffer.getNumChannels(); channel++) {
//    auto buf = output_buffer.getWritePointer(channel);
//    for (int i = 0; i < num_samples; i++) {
//      float x = 0;
//      for (int harm = 0; harm < harmonics; harm++) {
//        auto f = frequency * (harm * harmonic_diff + 1);
//        float nearest_df = 0.001f*f;
//        for (int j = -freq_width; j <= freq_width; j++) {
//          auto df = j*nearest_df;
//          x += nearest_df/(abs(df)+nearest_df) * sin(2 * juce::MathConstants<float>::pi * (f+df) * (sample_pos_+i) / static_cast<float>(getSampleRate()));
//        }
//      }
//      buf[i] += amp * x / harmonics;
//    }
    output_buffer.copyFrom(channel, 0, output_buffer, 0, 0, num_samples);
  }
}
