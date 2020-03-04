#include "log_slider.h"

#include "utils.h"

String LogSliderInternal::getTextFromValue(double value) {
  return compact_value_text(value);
}
