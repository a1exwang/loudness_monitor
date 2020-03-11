#pragma once
#include <JuceHeader.h>

constexpr float DefaultOriginalFrequency = 440.0f;
constexpr float DefaultWaveFormSize = 256;
constexpr size_t DefaultFrequencyPerOctave = 12 * 100;

class WaveForm;
class WaveFormVoice {
 public:
  WaveFormVoice(std::weak_ptr<const WaveForm> parent) :parent_(parent) { }

  float next_sample(float frequency);
  void fill_next_samples(float frequency, float *buffer, size_t count_to_fill);
  void fill_next_samples(float frequency, AudioBuffer<float> &buffer, size_t count_to_fill);
  void fill_next_samples(float frequency, AudioBuffer<float> &buffer);
  void reset();
 private:
  float phase = 0;
  std::weak_ptr<const WaveForm> parent_;
};

class WaveForm :public std::enable_shared_from_this<WaveForm> {
 public:
  WaveForm(AudioBuffer<float> data, float original_freq, size_t freqs_per_octave, size_t resample_cache_size);
  virtual ~WaveForm() = default;

  size_t memory_size() const;

//  float next_sample(float frequency);
//  void fill_next_samples(float frequency, float *buffer, size_t count_to_fill);
//  void fill_next_samples(float frequency, AudioBuffer<float> &buffer, size_t count_to_fill);
//  void fill_next_samples(float frequency, AudioBuffer<float> &buffer);
//  void reset();
  std::unique_ptr<WaveFormVoice> get_voice() const {
    return std::make_unique<WaveFormVoice>(weak_from_this());
  }

  std::tuple<const float*, size_t> get_original_waveform() const;
 private:
  friend class WaveFormVoice;
  size_t get_index(float frequency) const;
  float get_frequency(size_t index) const;
  void setup_resample_cache();
 private:
  AudioBuffer<float> original_samples_;
  float original_frequency_;

  float min_frequency_ = 55.0f;
  size_t freqs_per_octave_;
  size_t resample_cache_size_;

  std::vector<AudioBuffer<float>> resample_cache_;

  LagrangeInterpolator interpolator_;
};

template <typename FunctionType>
class FunctionWave :public WaveForm {
 public:
  FunctionWave(float sample_rate, size_t count, size_t freqs_per_octave, size_t resample_cache_size)
      :WaveForm(make_single_waveform(count), sample_rate/count, freqs_per_octave, resample_cache_size) { }

  static AudioBuffer<float> make_single_waveform(size_t count) {
    FunctionType func;
    AudioBuffer<float> result(1, static_cast<int>(count));
    func(result.getWritePointer(0), count);
    return result;
  }
};

struct SineWaveFunc {
  void operator()(float *buffer, size_t buffer_size) {
    for (size_t i = 0; i < buffer_size; i++) {
      buffer[i] = sin(juce::MathConstants<float>::twoPi * i / buffer_size);
    }
  }
};
using SineWave = FunctionWave<SineWaveFunc>;

struct SawWaveFunc {
  void operator()(float *buffer, size_t buffer_size) {
    for (size_t i = 0; i < buffer_size; i++) {
      // x is in [-1, 1)
//      float x = 2.0f * (i - buffer_size / 2) / buffer_size;
//      buffer[i] = x;
      buffer[i] = (float(i) / buffer_size)*2-1;
    }
  }
};
using SawWave = FunctionWave<SawWaveFunc>;
