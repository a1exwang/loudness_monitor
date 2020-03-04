#pragma once

#include <list>
#include <JuceHeader.h>

class DebugOutputComponent :public Component, public juce::Timer {
 public:
  DebugOutputComponent();
  ~DebugOutputComponent() override;
  void print_line(const std::string& s);
  void timerCallback() override;
  void paint (Graphics& g) override;
 private:
  Font mono_font;
  size_t max_line_count = 16;
  std::list<std::string> lines;
  std::mutex lines_lock;
};

class DebugOutputWindow :public DocumentWindow {
 public:
  DebugOutputWindow(const String& name, Colour backgroundColour, int buttonsNeeded) :DocumentWindow(name, backgroundColour, buttonsNeeded) {
    setContentOwned(&comp, true);
  }

 private:
  DebugOutputComponent comp;
};

void log(int level, const std::string& s);
