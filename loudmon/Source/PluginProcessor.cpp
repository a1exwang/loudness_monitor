#include <cassert>

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
    editor = new MainComponent(*this);
}

NewProjectAudioProcessor::~NewProjectAudioProcessor() {
}

const String NewProjectAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool NewProjectAudioProcessor::acceptsMidi() const {
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::producesMidi() const {
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
void NewProjectAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
  auto input_channels = getTotalNumInputChannels();
  auto output_channels = getTotalNumOutputChannels();
  assert(output_channels >= input_channels);
  editor->prepare_to_play(sampleRate, samplesPerBlock, input_channels);
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

void NewProjectAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
  auto t0 = std::chrono::high_resolution_clock::now();

  ScopedNoDenormals noDenormals;
  auto totalNumInputChannels  = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  auto input0 = getBusBuffer(buffer, true, 0);

  /* RMS calculation. Calculate for the whole block */
  std::vector<float> rms;
  for (int channel = 0; channel < totalNumInputChannels; ++channel) {
    double sum = 0;
    for (size_t i = 0; i < buffer.getNumSamples(); i++) {
      auto y = *input0.getReadPointer(channel, i);
      sum += y * y;
    }
    sum /= buffer.getNumSamples();
    std::stringstream ss;
    if (sum > 0) {
      auto db = static_cast<float>(10.0 * log10(sum));
      rms.push_back(db);
    }
    else {
      rms.push_back(std::numeric_limits<float>::min());
    }
  }
  editor->set_rms(rms);


  // Apply filters
  for (int channel = 0; channel < totalNumInputChannels; ++channel) {
    for (size_t i = 0; i < buffer.getNumSamples(); i++) {
      auto x = *input0.getReadPointer(channel, i);
      auto y = editor->filter_process(channel, x);
      *input0.getWritePointer(channel, i) = y;
    }
  }

  // Calculate latency
  auto callback_interval = std::chrono::duration<float>(t0 - last_process_time).count();
  editor->set_process_block_interval(callback_interval);
  last_process_time = t0;

  auto t1 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<float> total_latency = t1 - t0;
  auto max_latency_expected = float(getBlockSize()) / getSampleRate() * 1000;
  editor->set_latency_ms(total_latency.count() * 1000, max_latency_expected);
}

//==============================================================================
bool NewProjectAudioProcessor::hasEditor() const {
    return true;
}

AudioProcessorEditor* NewProjectAudioProcessor::createEditor() {
    return editor;
}

//==============================================================================
void NewProjectAudioProcessor::getStateInformation (MemoryBlock& destData) {
}

void NewProjectAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new NewProjectAudioProcessor();
}
