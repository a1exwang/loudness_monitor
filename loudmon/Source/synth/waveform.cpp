#include "waveform.h"

#include <cassert>

WaveForm::WaveForm(AudioBuffer<float> data, float original_freq, size_t freqs_per_octave, size_t resample_cache_size)
    :original_samples_(std::move(data)),
     original_frequency_(original_freq),
     freqs_per_octave_(freqs_per_octave),
     resample_cache_size_(resample_cache_size) {
  setup_resample_cache();
}

size_t WaveForm::memory_size() const {
  size_t ret = 0;
  for (auto &cache : resample_cache_) {
    ret += cache.getNumSamples() * sizeof(float);
  }
  ret += sizeof(*this);
  return ret;
}
size_t WaveForm::get_index(float frequency) const {
  auto ratio = frequency / min_frequency_;
  if (ratio <= 1) {
    return 0;
  } else {
    auto index = size_t(round(log2(ratio) * freqs_per_octave_));
    return std::min(index, resample_cache_size_-1);
  }
}

float WaveForm::get_frequency(size_t index) const {
  return exp2(float(index)/freqs_per_octave_) * min_frequency_;
}

void WaveForm::setup_resample_cache() {
  for (size_t i = 0; i < resample_cache_size_; i++) {
    auto target_freq = get_frequency(i);
    double speed_ratio = target_freq / original_frequency_;
    auto target_sample_count = static_cast<int>(ceil(original_samples_.getNumSamples() / speed_ratio));
    auto &buf = resample_cache_.emplace_back(1, target_sample_count);
    interpolator_.reset();
    interpolator_.process(speed_ratio, original_samples_.getReadPointer(0), buf.getWritePointer(0), target_sample_count);
  }
}

std::tuple<const float *, size_t> WaveForm::get_original_waveform() const {
  return {original_samples_.getReadPointer(0), original_samples_.getNumSamples()};
}
std::unique_ptr<WaveFormVoice> WaveForm::get_voice() const {
  return std::make_unique<WaveFormVoice>(weak_from_this());
}

float WaveFormVoice::next_sample(float frequency) {
  auto parent = parent_.lock();
  if (!parent) {
    return 0;
  }
  auto index = parent->get_index(frequency);
  auto &cache = parent->resample_cache_[index];
  auto cache_index = static_cast<int>(round(cache.getNumSamples() * phase));
  phase += 1.0f / cache.getNumSamples();
  phase = phase - int(phase);
  return cache.getSample(0, cache_index);
}

void WaveFormVoice::fill_next_samples(float frequency, float *buffer, size_t count_to_fill) {
  auto parent = parent_.lock();
  if (!parent) {
    juce::FloatVectorOperations::fill(buffer, 0, int(count_to_fill));
    return;
  }

  auto index = parent->get_index(frequency);
  auto &cache = parent->resample_cache_[index];
  auto cache_index = static_cast<int>(round(cache.getNumSamples() * phase));
  auto cache_buf = cache.getReadPointer(0);
  for (size_t i = 0; i < count_to_fill; i++) {
    buffer[i] = cache_buf[cache_index];
    cache_index++;
    cache_index %= cache.getNumSamples();
  }
  phase = float(cache_index) / cache.getNumSamples();
}

void WaveFormVoice::fill_next_samples(float frequency, AudioBuffer<float> &buffer, size_t count_to_fill) {
  assert(buffer.getNumChannels() == 1);
  fill_next_samples(frequency, buffer.getWritePointer(0), count_to_fill);
}

void WaveFormVoice::fill_next_samples(float frequency, AudioBuffer<float> &buffer) {
  fill_next_samples(frequency, buffer, buffer.getNumSamples());
}

void WaveFormVoice::reset() {
  phase = 0;
}

