#pragma once

#include <list>
#include <string>
#include <JuceHeader.h>
#include "PluginProcessor.h"

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

class MainComponent : public AudioProcessorEditor, public juce::Timer {
 public:
  MainComponent(NewProjectAudioProcessor& p);
  ~MainComponent() override {
    dw.deleteAndZero();
    dw = nullptr;
  }

  void visibilityChanged() override {
    if (dw) {
      dw->setVisible(isVisible());
    }
  }

  void paint(Graphics& g) override {
    g.fillAll(Colour::fromRGB(0, 0, 0));
    g.drawText("hello world", 10, 10, 50, 50, Justification::left);
  }

  void timerCallback() override {
    while (dequeue()) {}
  }

  void set_rms(const std::vector<float>& values) {
    std::stringstream ss;
    ss << "RMS: ";
    for (float value : values) {
      ss << std::fixed << std::setprecision(2) << std::setfill('0') << value << "dB ";
    }
    enqueue([this, text = ss.str()]() {
      rms_text.setText(text, juce::NotificationType::dontSendNotification);
      repaint();
    });
  }

  void set_latency_ms(float ms, float max_expected) {
    std::stringstream ss;
    ss << "Latency: " << std::fixed << std::setprecision(2) << std::setfill('0') << ms << "/" << max_expected << "ms";
    enqueue([this, text = ss.str()]() {
      latency_text.setText(text, juce::NotificationType::dontSendNotification);
      repaint();
    });
  }

  void repaint_safe() {
    enqueue([this]() {
      repaint();
    });
  }

 protected:
  void enqueue(std::function<void()> action) {
    std::unique_lock<std::mutex> _(queue_lock);
    queued_actions.emplace_back(std::move(action));
  }
  bool dequeue() {
    std::unique_lock<std::mutex> _(queue_lock);
    if (queued_actions.empty()) {
      return false;
    }
    queued_actions.front()();
    queued_actions.pop_front();
    return true;
  }

 private:
  Label rms_text, latency_text;
  std::list<std::function<void()>> queued_actions;
  std::mutex queue_lock;
  Component::SafePointer<DebugOutputWindow> dw;
  //==============================================================================
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

void log(int level, const std::string& s);
