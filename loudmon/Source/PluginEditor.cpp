/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <chrono>

static DebugOutputComponent* the_main_logger = nullptr;

#include <iostream>
void log(int level, const std::string &s) {
  if (the_main_logger) {
    std::stringstream ss;

    std::string time_str(29, '0');
    char* buf = &time_str[0];
    auto point = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(point);
    std::strftime(buf, 21, "%Y-%m-%dT%H:%M:%S.", std::localtime(&now_c));
    sprintf(buf + 20, "%09ld", point.time_since_epoch().count() % 1000000000);
    ss << "LOG(" << std::setw(2) << std::setfill('0') << level << ") " << time_str << " " << s;
    the_main_logger->print_line(ss.str());
  }
}

//==============================================================================

MainComponent::MainComponent(NewProjectAudioProcessor& p) : AudioProcessorEditor(p) {

  Desktop::getInstance().setGlobalScaleFactor(2);

  setResizable(true, true);
  setSize(400, 400);

  int h = 0;
  addAndMakeVisible(rms_text);
  rms_text.setFont (Font (16.0f, Font::bold));
  rms_text.setText("RMS", NotificationType::dontSendNotification);
  rms_text.setColour(Label::textColourId, Colours::orange);
  rms_text.setJustificationType (Justification::left);
  rms_text.setBounds(10, h+10, 300 - 20, 100 - 20);
  h += 100;

  addAndMakeVisible(latency_text);
  latency_text.setFont (Font (16.0f, Font::bold));
  latency_text.setText("Latency", NotificationType::dontSendNotification);
  latency_text.setColour(Label::textColourId, Colours::orange);
  latency_text.setJustificationType (Justification::left);
  latency_text.setBounds(10, h+10, 300 - 20, 100 - 20);
  h += 100;

  dw = new DebugOutputWindow("Debug Window", Colour(0), true);
  dw->setSize(1024, 400);
  dw->setResizable(true, true);

  startTimer(50);
}


inline DebugOutputComponent::DebugOutputComponent() :mono_font("Noto Sans Mono", 20, Font::plain) {
  if (the_main_logger == nullptr) {
    the_main_logger = this;
  }
  setSize(400, 200);
  startTimer(50);
}

DebugOutputComponent::~DebugOutputComponent() {
  the_main_logger = nullptr;
}
void DebugOutputComponent::print_line(const std::string &s) {
  std::unique_lock<std::mutex> _(lines_lock);
  lines.push_back(s);
  if (lines.size() > max_line_count) {
    lines.pop_front();
  }
}
void DebugOutputComponent::timerCallback() {
  repaint();
}
void DebugOutputComponent::paint(Graphics &g) {
  std::stringstream ss;
  {
    std::unique_lock<std::mutex> _(lines_lock);
    ss << "output: " << std::endl;
    for (auto &line : lines) {
      ss << line << std::endl;
    }
  }

  g.fillAll (Colours::black);
  g.setFont(mono_font);
  g.setColour(juce::Colour::fromRGB(248, 248, 248));
  g.drawMultiLineText(ss.str(), 10, 10, getWidth() - 20);
}
