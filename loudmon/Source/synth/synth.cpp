#include "synth.h"
#include "../loudmon/debug_output.h"

#include <memory>

void MPESimpleVoice::renderNextBlock(AudioBuffer<float> &outputBuffer, int startSample, int numSamples) {
  for (int channel = 0; channel < outputBuffer.getNumChannels(); channel++) {
    auto buf = outputBuffer.getWritePointer(channel);
    for (int i = 0; i < numSamples; i++) {
      buf[startSample + i] += 0.1 * sin(2 * juce::MathConstants<float>::pi * frequency * (sample_pos_+i) / getSampleRate());
    }
  }
  sample_pos_ += numSamples;
}

void MPESimpleVoice::noteStarted() {
  jassert (currentlyPlayingNote.isValid());
  jassert (currentlyPlayingNote.keyState == MPENote::keyDown
               || currentlyPlayingNote.keyState == MPENote::keyDownAndSustained);

  frequency = currentlyPlayingNote.getFrequencyInHertz();
  sample_pos_ = 0;
}
void MPESimpleVoice::noteStopped(bool allowTailOff) {
  jassert (currentlyPlayingNote.keyState == MPENote::off);
  stopNote();
}
void MPESimpleVoice::notePressureChanged() {
}
void MPESimpleVoice::notePitchbendChanged() {
  frequency = currentlyPlayingNote.getFrequencyInHertz();
}
