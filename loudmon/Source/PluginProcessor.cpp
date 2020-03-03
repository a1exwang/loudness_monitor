/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NewProjectAudioProcessor::NewProjectAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    size_t delay_count = 24000;
    delay_buffer.resize(getNumOutputChannels());
    for (auto& item : delay_buffer) {
        item.resize(delay_count);
    }
    delay_offset = 0;

    loudness_buffer.resize(getNumOutputChannels());
    for (auto& item : loudness_buffer) {
        item.resize(4096);
    }
    loudness_offset = 0;

    editor = new MainComponent(*this);
}

NewProjectAudioProcessor::~NewProjectAudioProcessor()
{
}

//==============================================================================
const String NewProjectAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NewProjectAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NewProjectAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NewProjectAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NewProjectAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NewProjectAudioProcessor::setCurrentProgram (int index)
{
}

const String NewProjectAudioProcessor::getProgramName (int index)
{
    return {};
}

void NewProjectAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void NewProjectAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void NewProjectAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NewProjectAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void NewProjectAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
  auto t0 = std::chrono::high_resolution_clock::now();
  ScopedNoDenormals noDenormals;
  auto totalNumInputChannels  = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  auto input0 = getBusBuffer(buffer, true, 0);
  //std::stringstream ss;
  //ss << "output channel count " << totalNumOutputChannels << ", " << " blocksize " << getBlockSize();
  //log(10, ss.str());

//  auto buf_size = buffer.getNumSamples();
//  auto delay_count = delay_buffer[0].size();
//  std::vector<std::vector<float>> input_buf(totalNumInputChannels);
//  for (auto& item : input_buf) {
//    item.resize(buffer.getNumSamples());
//  }
//  for (int i = 0; i < buffer.getNumSamples(); i++) {
//    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
//      input_buf[channel][i] = *input0.getReadPointer(channel, i);
//    }
//  }
//  size_t delay_offset_bk = delay_offset;
//  for (int i = 0; i < buffer.getNumSamples(); i++) {
//    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
//      *input0.getWritePointer(channel, i) = delay_buffer[channel][delay_offset];
//    }
//    delay_offset++;
//    if (delay_offset >= delay_count) {
//      delay_offset = 0;
//    }
//  }
//
//  for (int i = 0; i < buffer.getNumSamples(); i++) {
//    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
//      delay_buffer[channel][delay_offset_bk] = input_buf[channel][i];
//    }
//    delay_offset_bk++;
//    if (delay_offset_bk >= delay_count) {
//      delay_offset_bk = 0;
//    }
//  }

  for (int channel = 0; channel < totalNumInputChannels; ++channel) {
    double sum = 0;
    for (size_t i = 0; i < buffer.getNumSamples(); i++) {
      auto y = *input0.getReadPointer(channel, i);
      sum += y * y;
    }
    sum /= buffer.getNumSamples();
    std::stringstream ss;
    std::vector<float> rms;
    if (sum > 0) {
      float db = 10 * log10(sum);
      rms.push_back(db);
    }
    else {
      rms.push_back(std::numeric_limits<float>::min());
    }
    editor->set_rms(rms);
  }

  auto callback_interval = std::chrono::duration<float>(t0 - last_process_time).count();
  std::stringstream ss;
  ss << "callback interval " << std::fixed << std::setprecision(3) << std::setfill('0') << callback_interval * 1000 << "ms";
  log(10, ss.str());
  last_process_time = t0;

  auto t1 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<float> total_latency = t1 - t0;
  auto max_latency_expected = float(getBlockSize()) / getSampleRate() * 1000;
  editor->set_latency_ms(total_latency.count() * 1000, max_latency_expected);
}

//==============================================================================
bool NewProjectAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* NewProjectAudioProcessor::createEditor()
{
    return editor;
}

//==============================================================================
void NewProjectAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void NewProjectAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewProjectAudioProcessor();
}
