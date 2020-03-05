#include "filter_ui.h"

#include "utils.h"

FilterTransferFunctionComponent::FilterTransferFunctionComponent(double sample_rate, size_t input_channels, size_t fft_order)
    :sample_rate_(sample_rate),
     input_channels_(input_channels),
     fft_order_(fft_order), fft_size_(1ul << fft_order),
     forward_fft(fft_order),
     spectrum_(1, fft_size_*2),
     frequency_slider_("Frequency"),
     quality_slider_("Q"),
     filters_(input_channels_) {

  set_parameters(sqrt(20*20000), 1);

  float freq_min = 20, freq_max = sample_rate/2;
  frequency_slider_.setOnValueChange(std::bind(&FilterTransferFunctionComponent::filter_parameter_changed, this));
  frequency_slider_.setRangeLogarithm(freq_min, freq_max);
  addAndMakeVisible(frequency_slider_);

  quality_slider_.setOnValueChange(std::bind(&FilterTransferFunctionComponent::filter_parameter_changed, this));
  quality_slider_.setRangeLogarithm(0.05, 20);
  addAndMakeVisible(quality_slider_);

  addAndMakeVisible(plot_);
}


void FilterTransferFunctionComponent::paint(Graphics &g) {
  g.fillAll();
}
void FilterTransferFunctionComponent::set_parameters(double frequency, float quality) {
  frequency_ = frequency;
  quality_ = quality;

  IIRCoefficients coeffs;
  coeffs = IIRCoefficients::makeLowPass(sample_rate_, frequency_, quality_);

  {
    std::unique_lock<spinlock> _(filters_lock_);
    for (auto &filter : filters_) {
      filter.get<0>() = dsp::IIR::Coefficients<float>::makeHighPass(sample_rate_, std::min(frequency_*0.9, sample_rate_/2), quality_);
      filter.get<1>()= dsp::IIR::Coefficients<float>::makeLowPass(sample_rate_, std::min(frequency_*1.1, sample_rate_/2), quality_);
    }
  }

  filter_for_display_.get<0>() = dsp::IIR::Coefficients<float>::makeHighPass(sample_rate_, frequency_*0.9, quality_);
  filter_for_display_.get<1>() = dsp::IIR::Coefficients<float>::makeLowPass(sample_rate_, std::min(frequency_*1.1, sample_rate_/2), quality_);
  spectrum_.clear();
  spectrum_.setSample(0, 0, 1);
  dsp::AudioBlock<float> audio_block(spectrum_);
  dsp::ProcessContextReplacing<float> context_replacing(audio_block);
  filter_for_display_.process(context_replacing);
  forward_fft.performFrequencyOnlyForwardTransform(spectrum_.getArrayOfWritePointers()[0]);
  for (size_t i = 0; i < spectrum_.getNumSamples(); i++) {
    spectrum_.setSample(0, i, 10 * log10f(spectrum_.getSample(0, i)));
  }
  plot_.set_value_range(20, 20000, -50, 10, true, false);
  plot_.clear();
  std::vector<std::tuple<double, double>> values;
  for (size_t i = 0; i < fft_size_/2; i++) {
    auto y = spectrum_.getSample(0, i);
    values.emplace_back(double(i)/fft_size_ * sample_rate_, y);
  }
  plot_.add_new_values("spectrum", values);
}
