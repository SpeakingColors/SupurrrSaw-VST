// Studio 87 SupurrSaw: the editor is the real mockup page (BinaryData web/index.html) shown in a web view.
#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

namespace s87 {

class SupurrEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SupurrEditor (SupurrProcessor&);
    ~SupurrEditor() override;
    void resized() override;
    void paint (juce::Graphics&) override {}

private:
    void timerCallback() override;
    std::optional<juce::WebBrowserComponent::Resource> serve (const juce::String& path);
    static juce::File dbFile (const juce::String& path);
    static juce::String safeName (const juce::String&);

    SupurrProcessor& proc;
    std::unique_ptr<juce::WebBrowserComponent> web;
    std::unique_ptr<juce::ResizableBorderComponent> edges;   // all four sides and four corners, aspect ratio kept by the constrainer
    juce::String lastTrips;
    int tripsSeen = 0;
    double scopeAcc = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SupurrEditor)
};

} // namespace s87
