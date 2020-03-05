#include "plot.h"
#include "utils.h"

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
    g.drawSingleLineText(compact_value_text(original_x), x, app_y_max+pad/2+font_height/2, Justification::horizontallyCentred);

    // y axis
    g.fillEllipse(app_x_min-dot_radius/2, y-dot_radius/2, dot_radius, dot_radius);
    g.drawSingleLineText(compact_value_text(original_y), app_x_min-pad/2, y+font_height/2, Justification::right);
  }
}
