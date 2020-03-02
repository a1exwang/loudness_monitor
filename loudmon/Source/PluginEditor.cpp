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

MainContentComponent::MainContentComponent(NewProjectAudioProcessor& p) :AudioProcessorEditor(p)
{
	setResizable(true, true);
	setSize(640, 640);
	addAndMakeVisible(rms_text);
	rms_text.setFont (Font (16.0f, Font::bold));
	rms_text.setText("RMS", NotificationType::dontSendNotification);
	rms_text.setColour(Label::textColourId, Colours::orange);
	rms_text.setJustificationType (Justification::left);
	rms_text.setBounds(10, 10, 300, 100);

	//addAndMakeVisible(peak_text);
	dw = new DebugOutputWindow("Debug Window", Colour(0), true);
	dw->setResizable(true, true);
	dw->setVisible(true);
    
}


inline DebugOutputComponent::DebugOutputComponent() {
	if (the_main_logger == nullptr) {
		the_main_logger = this;
	}
	lines.push_back("123");
	lines.push_back("123");
	lines.push_back("123");
	setSize(320, 200);
	log(1, "line1");
	log(4, "line2");
}
