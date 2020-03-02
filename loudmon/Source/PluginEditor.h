/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <list>
#include <string>
#include <JuceHeader.h>
#include "PluginProcessor.h"

class DebugOutputComponent :public Component {
public:
    DebugOutputComponent();
	void print_line(const std::string& s) {
		lines.push_back(s);
        if (lines.size() > 10) {
            lines.pop_front();
        }
        repaint();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
        std::stringstream ss;
        ss << "output: " << std::endl;
        for (auto& line : lines) {
            ss << line << std::endl;
        }
        g.setFont(15);
        g.setColour(juce::Colour::fromRGB(248, 248, 248));
        g.drawMultiLineText(ss.str(), 10, 10, getWidth() - 20);
    }
private:
    std::list<std::string> lines;
};

class DebugOutputWindow :public DocumentWindow {
public:
    DebugOutputWindow(const String& name, Colour backgroundColour, int buttonsNeeded) :DocumentWindow(name, backgroundColour, buttonsNeeded) {
        setContentOwned(&comp, true);
    }

private:
    DebugOutputComponent comp;
};

//==============================================================================
class MainContentComponent   : public AudioProcessorEditor
{
public:
    //==============================================================================
    MainContentComponent(NewProjectAudioProcessor& p);
    ~MainContentComponent() {
        dw.deleteAndZero();
    }

    void paint(Graphics& g) {
        g.fillAll(Colour::fromRGB(0, 0, 0));
        g.drawText("hello world", 10, 10, 50, 50, Justification::left);
    }

    void resized() override
    {
    }
    Component::SafePointer<DebugOutputWindow> dw;

    void setRMS(std::vector<float> values) {
        std::stringstream ss;
        ss << "RMS: ";
        for (int i = 0; i < values.size(); i++) {
            ss << std::fixed << std::setprecision(2) << std::setfill('0') << values[i] << "dB ";
        }
        rms_text.setText(ss.str(), juce::NotificationType::dontSendNotification);
        repaint();
    }
    void setPeak(std::vector<float> values) {

    }

private:
    std::vector<float> rms, peak;
    Label rms_text, peak_text;
    //==============================================================================
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

void log(int level, const std::string& s);
