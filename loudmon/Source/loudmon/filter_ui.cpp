#include "filter_ui.h"

#include "utils.h"

FilterTransferFunctionComponent::FilterTransferFunctionComponent(double sample_rate, size_t input_channels, size_t fft_order)
    :sample_rate_(sample_rate),
     input_channels_(input_channels),
     fft_order_(fft_order), fft_size_(1ul << fft_order),
     forward_fft(fft_order),
     spectrum_(fft_size_*2, 0),
     filters_(input_channels),
     frequency_slider_("Frequency"),
     quality_slider_("Q") {

  set_parameters(sample_rate/2, 1);

  float freq_min = 20, freq_max = sample_rate/2;
  frequency_slider_.setOnValueChange(std::bind(&FilterTransferFunctionComponent::filter_parameter_changed, this));
  frequency_slider_.setRangeLogarithm(freq_min, freq_max);
  addAndMakeVisible(frequency_slider_);

  quality_slider_.setOnValueChange(std::bind(&FilterTransferFunctionComponent::filter_parameter_changed, this));
  quality_slider_.setRangeLogarithm(0.05, 20);
  addAndMakeVisible(quality_slider_);
}

template <typename T>
T clip(T value, T min, T max) {
  if (value > max) {
    return max;
  } else if (value < min) {
    return min;
  } else {
    return value;
  }
}

// linear normalize to [0, 1)
template <typename T>
T linear_normalize(T x, T min, T max) {
  return (x - min) / (max - min);
}

// linear scale from [0, 1) to [min, max)
template <typename T>
T scale_to(T x, T min, T max) {
  return x * (max - min) + min;
};

void FilterTransferFunctionComponent::paint(Graphics &g) {
  g.fillAll();

  if (getHeight() > slider_height_) {
    paint_transfer_function(g, 0, slider_height_, getWidth(), getHeight() - slider_height_);
  }
}
void FilterTransferFunctionComponent::set_parameters(double frequency, float quality) {
  frequency_ = frequency;
  quality_ = quality;

  std::fill(spectrum_.begin(), spectrum_.end(), 0);
  spectrum_[0] = 1;

  {
    std::unique_lock<spinlock> _(filters_lock_);
    for (auto &filter : filters_) {
      filter.setCoefficients(IIRCoefficients::makeLowPass(sample_rate_, frequency_, quality_));
    }
  }

  filter_for_display_.reset();
  filter_for_display_.setCoefficients(IIRCoefficients::makeLowPass(sample_rate_, frequency_, quality_));
  filter_for_display_.processSamples(spectrum_.data(), spectrum_.size());
  // clean impulse data, because we don't want it to affect future processing
  forward_fft.performFrequencyOnlyForwardTransform(spectrum_.data());
  for (float &value : spectrum_) {
    value = 10 * log10f(value);
  }
}
void FilterTransferFunctionComponent::paint_transfer_function(Graphics &g, float left, float top, float width, float height) {

  float pad = 10;
  float value_width = 50;
  float value_height = 30;
  float font_height = 16;
  float dot_radius = 4;
  float x_min = 0, x_max = static_cast<float>(sample_rate_)/2, y_min = -50, y_max = 10;
//  float left = 0, top = slider_height_, width = getWidth(), height = getHeight() - slider_height_;

  float ox = left + pad+value_width, oy = top + height-pad*2-value_height;
  float app_x_max = left + width - pad, app_y_min = top + pad;
  /* Unreadable code begins */
  bool use_log = true;
  auto map_point = [use_log, this, x_min, x_max, y_min, y_max, pad, value_width, value_height, ox, oy, app_x_max, app_y_min](float x, float y)
      -> std::tuple<float, float> {
    if (use_log) {
      auto result_x = scale_to<double>(linear_normalize<double>(log(linear_normalize<double>(x, x_min, x_max)+1), 0, log(2)), ox, app_x_max);
      auto result_y = scale_to<double>(linear_normalize<double>(log(1-linear_normalize<double>(y, y_min, y_max)+1), 0, log(2)), app_y_min, oy);
      return {result_x, result_y};
    } else {
      return std::make_tuple<float, float>(
          scale_to<float>(linear_normalize(x, x_min, x_max), ox, app_x_max),
          scale_to<float>(1-linear_normalize(y, y_min, y_max), app_y_min, oy));
    }
  };
  auto map_point_reverse = [use_log, this, x_min, x_max, y_min, y_max, pad, value_width, value_height, ox, oy, app_x_max, app_y_min](float x, float y)
      -> std::tuple<float, float> {
    if (use_log) {
      auto result_x = scale_to<double>(linear_normalize<double>(exp(linear_normalize(x,ox,app_x_max)), 1, exp(1)), x_min, x_max);
      auto result_y = scale_to<double>(1-linear_normalize<double>(exp(linear_normalize(y,app_y_min,oy)), 1, exp(1)), y_min, y_max);
      return {result_x, result_y};
    } else {
      return std::make_tuple<float, float>(
          scale_to<float>(linear_normalize(x, ox, app_x_max), x_min, x_max),
          scale_to<float>(1-linear_normalize(y, app_y_min, oy), y_min, y_max));
    }
  };

  g.setColour(Colour(0xffffff00));
  g.drawSingleLineText("Filter:", left, top+font_height+pad/2);

  g.setColour(Colour(0xffcc9922));

  g.drawLine(ox, oy, app_x_max, oy);
  g.drawLine(ox, oy, ox, app_y_min);
  size_t n = 5;
  g.setFont(font_height);
  // draw x axis and y axis in one loop
  for (size_t i = 0; i <= n; i++) {
//    auto original_x = x_min + (x_max-x_min)*i/n;
//    auto original_y = y_min + (y_max-y_min)*i/n;
//    auto [x, y] = map_point(original_x, original_y);
    auto x = ox + (app_x_max-ox)*i/n;
    auto y = app_y_min + (oy-app_y_min)*i/n;
    auto [original_x, original_y] = map_point_reverse(x, y);
    // x axis
    g.fillEllipse(x-dot_radius/2, oy-dot_radius/2, dot_radius, dot_radius);
    g.drawSingleLineText(compact_value_text(original_x), x, oy+pad/2+font_height/2, Justification::horizontallyCentred);

    // y axis
    g.fillEllipse(ox-dot_radius/2, y-dot_radius/2, dot_radius, dot_radius);
    g.drawSingleLineText(compact_value_text(original_y), ox-pad/2, y+font_height/2, Justification::right);
  }

  for (size_t i = 1; i < fft_size_/2; i++) {
    auto [x1, y1] = map_point(double(i)/fft_size_ * sample_rate_, spectrum_[i-1]);
    auto [x2, y2] = map_point(double(i+1)/fft_size_ * sample_rate_, spectrum_[i]);
    if (y1 >= app_y_min && y1 < oy && y2 >= app_y_min && y2 < oy) {
      g.drawLine(x1, y1, x2, y2);
    }
  }
  /* Unreadable code ends */
}
