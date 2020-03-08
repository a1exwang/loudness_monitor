#pragma once

#include <JuceHeader.h>
#include "loudmon/filter_ui.h"
#include "synth/synth.h"


class MainComponent;
class NewProjectAudioProcessor  : public AudioProcessor {
 public:
  //==============================================================================
  NewProjectAudioProcessor();
  ~NewProjectAudioProcessor() override;

  //==============================================================================
  void prepareToPlay (double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

  void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

  AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram (int index) override;
  const String getProgramName (int index) override;
  void changeProgramName (int index, const String& newName) override;

  void getStateInformation (MemoryBlock& destData) override;
  void setStateInformation (const void* data, int sizeInBytes) override;

 private:
  std::vector<std::vector<float>> loudness_buffer;
  AudioBuffer<float> synth_output_buffer;
  size_t synth_channels = 2;
  std::chrono::high_resolution_clock::time_point last_process_time;
  size_t late_block_count_ = 0;
  MPESynthesiser synthesiser_;

  float freq_split_lowmid = 200, freq_split_midhigh = 2000;
  float q = 0.1;
  std::vector<dsp::IIR::Filter<float>> low_filter, high_filter;
  std::vector<PeakFilter<float>> mid_filter;
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessor)
};
