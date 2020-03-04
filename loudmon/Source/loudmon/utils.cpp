#include "utils.h"

#include <cmath>

#include <iomanip>
#include <sstream>

std::string compact_value_text(float f) {
  std::stringstream ss;
  if (std::abs(f) < 1000) {
    ss << std::fixed << std::setprecision(1) << f;
    return ss.str();
  } else {
    ss << std::fixed << std::setprecision(1) << f/1000 << "k";
    return ss.str();
  }
}

