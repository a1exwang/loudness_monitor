#include "synth.h"
#include "../loudmon/debug_output.h"

#include <memory>

void SynthControl::resized() {
  auto area = getLocalBounds();
  size_t columns = 1;
  size_t rows = static_cast<size_t>(area.getHeight() / 30.0f);
  size_t height = area.getHeight();
  size_t width = area.getWidth();

  auto it = components_.begin();
  for (size_t i = 0; i < columns; i++) {
    auto current_column = area.removeFromLeft(static_cast<int>((float)width / columns));
    for (int j = 0; j < rows; j++) {
      if (it == components_.end()) {
        break;
      }
      (*it)->setBounds(current_column.removeFromTop(static_cast<int>((float)height / rows)));
      it++;
    }
  }
}


MPESimpleVoice::MPESimpleVoice(std::function<SynthControl* ()> get_control) :get_control_(std::move(get_control)) {
  ADSR::Parameters params{};
  params.attack = 0.001f;
  params.decay = 0.1f;
  params.sustain = 0.5f;
  params.release = 1.0f;
  adsr_.setParameters(params);
}

void MPESimpleVoice::renderNextBlock(AudioBuffer<float> &output_buffer, int startSample, int num_sample) {
  auto control = get_control_();
  if (!control) {
    return;
  }
  int harmonics = control->harmonics.value();
  int harmonic_diff = control->harmonic_diff.value();
  int freq_width = control->freq_width.value();
  float amp = pow(10.0f, control->amp.value()/10.0f);

  AudioBuffer<float> process_buffer(output_buffer.getNumChannels(), num_sample);
  process_buffer.clear();
  for (int channel = 0; channel < output_buffer.getNumChannels(); channel++) {
    auto buf = process_buffer.getWritePointer(channel);
    for (int i = 0; i < num_sample; i++) {
      float x = 0;
      for (int harm = 0; harm < harmonics; harm++) {
        auto f = frequency * (harm * harmonic_diff + 1);
        float nearest_df = 0.001f*f;
        for (int j = -freq_width; j <= freq_width; j++) {
          auto df = j*nearest_df;
          x += nearest_df/(abs(df)+nearest_df) * sin(2 * juce::MathConstants<float>::pi * (f+df) * (sample_pos_+i) / static_cast<float>(getSampleRate()));
        }
      }
      buf[i] += amp * x / harmonics;
    }
  }
  adsr_.applyEnvelopeToBuffer(output_buffer, 0, num_sample);
  if (!adsr_.isActive()) {
    clearCurrentNote();
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
}
void MPESimpleVoice::noteStopped(bool allowTailOff) {
  jassert (currentlyPlayingNote.keyState == MPENote::off);
  if (allowTailOff) {
    adsr_.noteOff();
  } else {
    clearCurrentNote();
  }
}
void MPESimpleVoice::notePressureChanged() {
}
void MPESimpleVoice::notePitchbendChanged() {
  frequency = static_cast<float>(currentlyPlayingNote.getFrequencyInHertz());
}
