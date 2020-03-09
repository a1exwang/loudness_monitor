#pragma once


#include <JuceHeader.h>
#include "utils.h"
#include <iterator>
#include <list>


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

constexpr uint32_t ReservedColors[] = {
    0xff22ee22,
    0xff44cc44,
    0xff66aa66,
    0xffee8822,
    0xffee22ee,
    0xffcc44cc,
    0xffaa66aa,
    0xff2288ee,
};

class PlotComponent :public juce::Component {
 public:
  PlotComponent() {
    reset();
  }
  void add_new_value(const std::string& name, float x, float y) {
    std::unique_lock<spinlock> _(lock_);
    if (values_.find(name) == values_.end()) {
      assign_color(name);
    }
    values_[name].emplace_back(x, y);
  }

  void add_new_values(const std::string& name, std::vector<std::tuple<float, float>> new_values) {
    std::unique_lock<spinlock> _(lock_);
    if (values_.find(name) == values_.end()) {
      assign_color(name);
    }
    std::copy(new_values.begin(), new_values.end(), std::back_inserter(values_[name]));
  }
  void clear() {
    std::unique_lock<spinlock> _(lock_);
    values_.clear();
  }

  void set_value_range(float x_min, float x_max, float y_min, float y_max, bool x_axis_log, bool y_axis_log) {
    std::unique_lock<spinlock> _(lock_);
    x_min_ = x_min;
    x_max_ = x_max;
    y_min_ = y_min;
    y_max_ = y_max;
    x_axis_log_ = x_axis_log;
    y_axis_log_ = y_axis_log;
    reset();
  }

  // UI Callbacks in UI thread
  void resized() override {
    left = 0;
    top = 0;
    width = static_cast<float>(getWidth());
    height = static_cast<float>(getHeight());
    reset();
  }

  void paint(Graphics &g) override {
    paint_all(g);
  }

 private:
  void paint_all(Graphics &g) {
    paint_coordinates(g);
    paint_legends(g);
    plot_lines(g);
  }
  void assign_color(const std::string& name) {
    auto color = ReservedColors[line_colors_.size() % (sizeof(ReservedColors)/sizeof(ReservedColors[0]))];
    line_colors_[name] = color;
  }

  void reset() {
    app_x_min = left + pad+value_width;
    app_y_max = top + height-pad*2-value_height;
    app_x_max = left + width - pad, app_y_min = top + pad;
  }

  static float map_value_log(float value, float min, float max, float target_min, float target_max) {
    float k = min / (max - min);
    return scale_to<float>(linear_normalize<float>(
        log(linear_normalize<float>(value, min, max)+k), log(k), log(1+k)), target_min, target_max);
  }
  static float map_value_exp(float value, float min, float max, float target_min, float target_max) {
    float k = log(max/min);
    return scale_to<float>(linear_normalize<float>(
        exp(linear_normalize<float>(value, min, max)*k), 1, exp(k)), target_min, target_max);
  }
  static float map_value_linear(float value, float min, float max, float target_min, float target_max) {
    return scale_to<float>(linear_normalize<float>(value, min, max), target_min, target_max);
  }

  std::tuple<float, float> map_point(float x, float y) {
    return {
        x_axis_log_ ? map_value_log(x, x_min_, x_max_, app_x_min, app_x_max) : map_value_linear(x, x_min_, x_max_, app_x_min, app_x_max),
        y_axis_log_ ? map_value_log(y, y_min_, y_max_, app_y_max, app_y_min /* y is reversed */) : map_value_linear(y, y_min_, y_max_, app_y_max, app_y_min),
    };
  };
  std::tuple<float, float> map_point_reverse(float x, float y) {
    return {
        x_axis_log_ ? map_value_exp(x, app_x_min, app_x_max, x_min_, x_max_) : map_value_linear(x, app_x_min, app_x_max, x_min_, x_max_),
        y_axis_log_ ? map_value_exp(y, app_y_min, app_y_max, y_max_, y_min_ /* y is reversed */) : map_value_linear(y, app_y_min, app_y_max, y_max_, y_min_),
    };
  };
 private:
  void paint_coordinates(Graphics &g);
  void plot_lines(Graphics &g) {
    g.saveState();
    for (auto &[name, line] : values_) {
      if (!line.empty()) {
        g.setColour(juce::Colour(line_colors_[name]));
        float last_x, last_y;
        std::tie(last_x, last_y) = line.front();
        for (size_t i = 1; i < line.size(); i++) {
          float x, y;
          std::tie(x, y) = line[i];
          auto [x1, y1] = map_point(last_x, last_y);
          auto [x2, y2] = map_point(x, y);
          if (x1 >= app_x_min && x1 < app_x_max && x1 >= app_x_min && x1 < app_x_max &&
              y1 >= app_y_min && y1 < app_y_max && y2 >= app_y_min && y2 < app_y_max) {
            g.drawLine(x1, y1, x2, y2);
          }
          std::tie(last_x, last_y) = line[i];
        }
      }
    }
    g.restoreState();
  }
  void paint_legends(Graphics &g) {
    auto start_y = app_y_min + (app_y_max-app_y_min)*0.1f;
    size_t i = 0;
    g.saveState();
    for (auto &[name, color] : line_colors_) {
      g.setColour(juce::Colour(color));
      g.drawLine(app_x_min + (app_x_max-app_x_min)*0.85f,
                 start_y + i * legend_height,
                 app_x_min + (app_x_max-app_x_min)*0.9f,
                 start_y + i * legend_height);
      g.setFont(legend_height);
      g.drawSingleLineText(name, static_cast<int>(app_x_min + (app_x_max-app_x_min)*0.91f), static_cast<int>(start_y + i * legend_height));
    }
    g.restoreState();
  }

 private:
  float x_min_ = 1, x_max_ = 10, y_min_ = 0, y_max_ = 10;
  float pad = 10;
  float value_width = 50;
  float value_height = 30;
  float font_height = 16;
  float dot_radius = 4;
  float legend_height = 8;

  float left, top, width, height;
  float app_x_min, app_x_max, app_y_min, app_y_max;

  bool x_axis_log_ = false;
  bool y_axis_log_ = false;

  std::map<std::string, std::vector<std::tuple<float, float>>> values_;
  std::map<std::string, uint32_t> line_colors_;
  spinlock lock_;
};