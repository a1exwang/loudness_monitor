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

IIRCoefficients operator+(const IIRCoefficients &lhs, const IIRCoefficients &rhs) {
  IIRCoefficients ret;
  for (int i = 0; i < sizeof(lhs.coefficients)/sizeof(lhs.coefficients[0]); i++) {
    ret.coefficients[i] = lhs.coefficients[i] + rhs.coefficients[i];
  }
  return ret;
}
//==============================================================================
void NewProjectAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
  auto input_channels = getTotalNumInputChannels();
  auto output_channels = getTotalNumOutputChannels();
  assert(output_channels >= input_channels);
  editor->prepare_to_play(sampleRate, samplesPerBlock, input_channels);

  low_filter.resize(input_channels);
  mid_filter.resize(input_channels);
  high_filter.resize(input_channels);
  for (int i = 0; i < mid_filter.size(); i++) {
    low_filter[i].coefficients = dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq_split_lowmid, q);
    mid_filter[i].get<0>() = dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq_split_midhigh, q);
    mid_filter[i].get<1>() = dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, freq_split_lowmid, q);
    high_filter[i].coefficients = dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq_split_midhigh, q);
  }
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

template <typename T>
T calculate_rms(const T *data, size_t size) {
  double sum = 0;
  for (size_t i = 0; i < size; i++) {
    sum += data[i] * data[i];
  }
  sum /= size;
  std::stringstream ss;
  if (sum > 0) {
    return static_cast<T>(10.0 * log10(sum));
  }
  else {
    return std::numeric_limits<T>::min();
  }
}

void NewProjectAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
  auto t0 = std::chrono::high_resolution_clock::now();

  ScopedNoDenormals noDenormals;
  auto totalNumInputChannels  = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  auto input0 = getBusBuffer(buffer, true, 0);

  /* RMS calculation. Calculate for the whole block */
  std::vector<float> rms;
  for (int channel = 0; channel < totalNumInputChannels; ++channel) {
    rms.push_back(calculate_rms(input0.getArrayOfReadPointers()[channel], getBlockSize()));
  }
  editor->set_input_rms(rms);

  // Apply filters
  std::vector<float> channel_data(buffer.getNumSamples());

  dsp::AudioBlock<float> audio_block(input0);
  dsp::ProcessContextReplacing<float> context_replacing_main(audio_block);

  std::stringstream mid_rms, low_rms, high_rms;
  for (int channel = 0; channel < totalNumInputChannels; channel++) {
//    AudioBuffer<float> audio_buffer_main(1, getBlockSize());
//    audio_buffer_main.copyFrom(0, 0, input0, channel, 0, getBlockSize());
//    dsp::AudioBlock<float> audio_block_main(audio_buffer_main);
//    dsp::ProcessContextReplacing<float> replacing_context_main(audio_block_main);
//    editor->filter_process(channel, replacing_context_main);
//    input0.copyFrom(channel, 0, audio_buffer_main, 0, 0, getBlockSize());

    AudioBuffer<float> audio_buffer_filter(1, getBlockSize());
    audio_buffer_filter.copyFrom(0, 0, input0, channel, 0, getBlockSize());
    dsp::AudioBlock<float> audio_block_filter(audio_buffer_filter);
    dsp::ProcessContextReplacing<float> replacing_context(audio_block_filter);
    mid_filter[channel].process(replacing_context);
    auto channel_rms = calculate_rms(audio_buffer_filter.getArrayOfReadPointers()[0], getBlockSize());
    mid_rms << std::fixed << std::setprecision(2) << channel_rms << "dB ";

    audio_buffer_filter.copyFrom(0, 0, input0, channel, 0, getBlockSize());
    low_filter[channel].process(replacing_context);
    channel_rms = calculate_rms(audio_buffer_filter.getArrayOfReadPointers()[0], getBlockSize());
    low_rms << std::fixed << std::setprecision(2) << channel_rms << "dB ";

    audio_buffer_filter.copyFrom(0, 0, input0, channel, 0, getBlockSize());
    high_filter[channel].process(replacing_context);
    channel_rms = calculate_rms(audio_buffer_filter.getArrayOfReadPointers()[0], getBlockSize());
    high_rms << std::fixed << std::setprecision(2) << channel_rms << "dB ";
  }
  editor->add_display_value("Output Low RMS", low_rms.str());
  editor->add_display_value("Output Mid RMS", mid_rms.str());
  editor->add_display_value("Output High RMS", high_rms.str());

  // Calculate latency
  auto callback_interval = std::chrono::duration<float>(t0 - last_process_time).count();
  editor->set_process_block_interval(callback_interval);
  last_process_time = t0;

  editor->send_block(input0);

  auto t1 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<float> total_latency = t1 - t0;
  auto max_latency_expected = float(getBlockSize()) / getSampleRate();
  if (total_latency.count() > max_latency_expected) {
    late_block_count_++;
  }
  editor->set_latency_ms(total_latency.count() * 1000, max_latency_expected * 1000, late_block_count_);
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
