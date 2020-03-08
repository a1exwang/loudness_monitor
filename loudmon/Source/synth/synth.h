#pragma once
#include <JuceHeader.h>

class MPESimpleVoice  : public MPESynthesiserVoice {
 public:
  void noteStarted() override;

  void noteStopped (bool allowTailOff) override;

  void notePressureChanged() override;

  void notePitchbendChanged() override;

  void noteTimbreChanged()   override {}
  void noteKeyStateChanged() override {}

  void renderNextBlock (AudioBuffer<float>& outputBuffer,
                        int startSample,
                        int numSamples) override;

 private:
  void stopNote() {
    clearCurrentNote();
    sample_pos_ = 0;
  }

  double frequency;
  size_t sample_pos_;
};

