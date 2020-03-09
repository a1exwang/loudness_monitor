#include <chrono>

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "loudmon/debug_output.h"


MainComponent::MainComponent(NewProjectAudioProcessor& p)
    : AudioProcessorEditor(p),
      main_info_(std::bind(&MainComponent::repaint_safe, this)),
      menu_bar_(this),
      oscilloscope_(256),
      keyboard_(keyboard_state_, juce::MidiKeyboardComponent::Orientation::horizontalKeyboard) {

  if (juce::SystemStats::getOperatingSystemType() == juce::SystemStats::OperatingSystemType::Linux) {
    Desktop::getInstance().setGlobalScaleFactor(2);
  } else {
    Desktop::getInstance().setGlobalScaleFactor(1);
  }
  setResizable(true, true);
  setSize(600, 800);
  setResizeLimits(300, 300, std::numeric_limits<int>::max(), std::numeric_limits<int>::max());

  addAndMakeVisible(menu_bar_);

  addAndMakeVisible(keyboard_);
  addAndMakeVisible(synth_control_);

  addAndMakeVisible(info_text);
  info_text.setFont (Font (16.0f, Font::bold));
  info_text.setText("Info", NotificationType::dontSendNotification);
  info_text.setColour(Label::textColourId, Colours::orange);
  info_text.setJustificationType (Justification::left);
  info_text.setBounds(10, 10, getWidth() - 20, 400 - 20);

  addAndMakeVisible(oscilloscope_);

  debug_window = new DebugOutputWindow("Debug Window", Colour(0), true);
  debug_window->setSize(1024, 400);
  debug_window->setResizable(true, true);

  startTimerHz(30);
  resize_children();
}

MainComponent::~MainComponent() {
  if (debug_window) {
    debug_window.deleteAndZero();
    debug_window = nullptr;
  }
}

void MainComponent::toggle_debug_window() {
  debug_window_visible_ = !debug_window_visible_;
  if (debug_window) {
    debug_window->setVisible(debug_window_visible_ && isVisible());
  }
}

void MainComponent::visibilityChanged() {
  if (debug_window) {
    debug_window->setVisible(isVisible() && debug_window_visible_);
  }
}

void MainComponent::paint(Graphics& g) {
  auto now = std::chrono::high_resolution_clock::now();
  main_info_.set_fps(1 / std::chrono::duration<float>(now-last_paint_time).count());
  last_paint_time = now;

  g.fillAll(Colour::fromRGB(0, 0, 0));
}

void MainComponent::enqueue(std::function<void()> action) {
  std::unique_lock<std::mutex> _(queue_lock);
  queued_actions.emplace_back(std::move(action));
}
bool MainComponent::dequeue() {
  std::function<void()> action;
  {
    std::unique_lock<std::mutex> _(queue_lock);
    if (queued_actions.empty()) {
      return false;
    }
    action = std::move(queued_actions.front());
    queued_actions.pop_front();
  }
  action();
  return true;
}

void MainComponent::resize_children() {
  auto area = getLocalBounds();
  menu_bar_.setBounds(area.removeFromTop (LookAndFeel::getDefaultLookAndFeel().getDefaultMenuBarHeight()));
  keyboard_.setBounds(area.removeFromBottom(50));
  auto control_area = area.removeFromLeft(static_cast<int>(area.getWidth()*(1.0f/3.0f)));
  synth_control_.setBounds(control_area);
  auto total_height = area.getHeight();
  info_text.setBounds(area.removeFromTop(total_height/4));
  oscilloscope_.setBounds(area.removeFromTop(total_height/4));

  if (filter && main_filter_enabled) {
    filter->setBounds(area);
  }
}
