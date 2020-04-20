#include "plot.h"
#include "utils.h"
#include <numeric>


typedef std::tuple<float, float> Pt;
float dot(const Pt &p1, const Pt &p2) {
  return std::get<0>(p1) * std::get<0>(p2) + std::get<1>(p1) * std::get<1>(p2);
}
Pt operator+(Pt p1,Pt p2) {
  return std::make_tuple<float, float>(std::get<0>(p1) + std::get<0>(p2), std::get<1>(p1) + std::get<1>(p2));
}
Pt operator-(Pt p1,Pt p2) {
  return std::make_tuple<float, float>(std::get<0>(p1) - std::get<0>(p2), std::get<1>(p1) - std::get<1>(p2));
}

std::ostream &operator<<(std::ostream &os, const Pt &point) {
  os << "(" << std::get<0>(point) << ", " << std::get<1>(point) << ")";
  return os;
}


void PlotComponent::paint_coordinates(Graphics &g) {
  g.setColour(Colour(0xffcc9922));

  g.drawLine(app_x_min, app_y_max, app_x_max, app_y_max);
  g.drawLine(app_x_min, app_y_max, app_x_min, app_y_min);
  size_t n = 5;
  g.setFont(font_height);
  // draw x axis and y axis in one loop
  for (size_t i = 0; i <= n; i++) {
    auto x = app_x_min + (app_x_max-app_x_min)*i/n;
    auto y = app_y_min + (app_y_max-app_y_min)*i/n;
    auto [original_x, original_y] = map_point_reverse(x, y);
    // x axis
    g.fillEllipse(x-dot_radius/2, app_y_max-dot_radius/2, dot_radius, dot_radius);
    g.drawSingleLineText(compact_value_text(original_x), static_cast<int>(x), static_cast<int>(app_y_max+pad/2+font_height/2), Justification::horizontallyCentred);

    // y axis
    g.fillEllipse(app_x_min-dot_radius/2, y-dot_radius/2, dot_radius, dot_radius);
    g.drawSingleLineText(compact_value_text(original_y), static_cast<int>(app_x_min-pad/2), static_cast<int>(y+font_height/2), Justification::right);
  }
}
void PlotComponent::plot_lines(Graphics &g) {
  g.saveState();
  for (auto &[name, line] : values_) {
    if (!line.empty()) {
      g.setColour(juce::Colour(line_colors_[name]));
      float last_x, last_y;
      std::tie(last_x, last_y) = line.front();
      for (size_t i = 1; i < line.size(); i++) {
        auto [x, y] = line[i];
        auto [x1, y1] = map_point(last_x, last_y);
        auto [x2, y2] = map_point(x, y);

        if (get_clip_point(x1, y1, x2, y2)) {
          g.drawLine(x1, y1, x2, y2);
        }
        std::tie(last_x, last_y) = line[i];
      }
    }
  }
  g.restoreState();
}
void PlotComponent::paint_legends(Graphics &g) {
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

bool PlotComponent::get_clip_point(float &x1, float &y1, float &x2, float &y2) const {

  // make sure x1 <= x2
  if (x1 > x2) {
    std::swap(x1, x2);
    std::swap(y1, y2);
  }

  auto get_y = [=](float x) {
    return (y2 - y1) / (x2 - x1) * (x-x1) + y1;
  };
  auto get_x = [=](float y) {
    return (x2 - x1) / (y2 - y1) * (y-y1) + x1;
  };

  auto in_middle_of = [](const Pt &target, Pt p1, Pt p2) {
    return dot(std::move(p1) - target, std::move(p2) - target) < 0;
  };

  std::tuple<float, float> p1{x1, y1}, p2{x2, y2};

  if (!(x1 >= app_x_min && x1 < app_x_max && y1 >= app_y_min && y1 < app_y_max) &&
      !(x2 >= app_x_min && x2 < app_x_max && y2 >= app_y_min && y2 < app_y_max)) {
    return false;
  }

  // One single point cannot make a line
  if (x1 == x2 && y1 == y2) {
    return false;
  }

  auto filter_inf_x = [&](float &x) {
    if (std::isinf(x)) {
      if (x > 0) {
        x = app_x_max;
      } else {
        x = app_x_min;
      }
    }
  };
  auto filter_inf_y = [&](float &y) {
    if (std::isinf(y)) {
      if (y > 0) {
        y = app_x_max;
      } else {
        y = app_x_min;
      }
    }
  };
  filter_inf_x(x1);
  filter_inf_x(x2);
  filter_inf_y(y1);
  filter_inf_y(y2);
//
//  if (std::isinf(y1) && std::isinf(y2)) {
//    if (y1 * y2 > 0 || std::isinf(x1) || std::isinf(x2)) {
//      return false;
//    }
//    if (y1 < y2) {
//      y1 = app_y_min;
//      y2 = app_y_max;
//    } else {
//      y1 = app_y_max;
//      y2 = app_y_min;
//    }
//    return true;
//  } else if (std::isinf(y1) || std::isinf(y2)) {
//    if (std::isinf(y2)) {
//      x2 = x1;
//      y2 = y2 > 0 ? app_y_max : app_y_min;
//    } else {
//      x1 = x2;
//      y1 = y1 > 0 ? app_y_max : app_y_min;
//    }
//    return true;
//  }
  // neither y1 nor y2 is INF
  // TODO

  if (x1 != x2) {
    std::tuple<float, float>
        pleft{app_x_min, get_y(app_x_min)},
        pright{app_x_max, get_y(app_x_max)};

    if (in_middle_of(pleft, p1, p2) && std::get<1>(pleft) >= app_y_min && std::get<1>(pleft) < app_y_max) {
      std::tie(x1, y1) = pleft;
      return true;
    } else if (in_middle_of(pright, p1, p2) && std::get<1>(pright) >= app_y_min && std::get<1>(pright) < app_y_max) {
      std::tie(x2, y2) = pright;
      return true;
    }
  }
  if (y1 != y2) {
    Pt ptop{get_x(app_y_min), app_y_min},
        pbottom{get_x(app_y_max), app_y_max};

    if (in_middle_of(ptop, p1, p2) && std::get<0>(ptop) >= app_x_min && std::get<0>(ptop) < app_x_max) {
      if (y1 < y2) {
        std::tie(x1, y1) = ptop;
      } else {
        std::tie(x2, y2) = ptop;
      }
      return true;
    } else if (in_middle_of(pbottom, p1, p2) && std::get<0>(pbottom) >= app_x_min && std::get<0>(pbottom) < app_x_max) {
      if (y1 < y2) {
        std::tie(x1, y1) = pbottom;
      } else {
        std::tie(x2, y2) = pbottom;
      }
      return true;
    }
  }

  return true;
}
