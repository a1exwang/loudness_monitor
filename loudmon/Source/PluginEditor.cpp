#include <chrono>

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "loudmon/debug_output.h"


MainComponent::MainComponent(NewProjectAudioProcessor& p)
    : AudioProcessorEditor(p),
      main_info_(std::bind(&MainComponent::repaint_safe, this)) {

  Desktop::getInstance().setGlobalScaleFactor(2);

  setResizable(true, true);
  setSize(400, 800);

  addAndMakeVisible(info_text);
  info_text.setFont (Font (16.0f, Font::bold));
  info_text.setText("Info", NotificationType::dontSendNotification);
  info_text.setColour(Label::textColourId, Colours::orange);
  info_text.setJustificationType (Justification::left);
  info_text.setBounds(10, 10, getWidth() - 20, 400 - 20);

//  dw = new DebugOutputWindow("Debug Window", Colour(0), true);
//  dw->setSize(1024, 400);
//  dw->setResizable(true, true);

  startTimerHz(30);
}

MainComponent::~MainComponent() {
  dw.deleteAndZero();
  dw = nullptr;
}

void MainComponent::visibilityChanged() {
  if (dw) {
    dw->setVisible(isVisible());
  }
}

void MainComponent::paint(Graphics& g) {
  auto now = std::chrono::high_resolution_clock::now();
  main_info_.set_fps(1 / std::chrono::duration<float>(now-last_paint_time).count());
  last_paint_time = now;

  g.fillAll(Colour::fromRGB(0, 0, 0));
}
