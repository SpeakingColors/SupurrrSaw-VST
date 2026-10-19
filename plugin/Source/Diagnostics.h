// Studio 87 SupurrSaw: small size-capped log and the per-user data folders.
#pragma once
#include <juce_core/juce_core.h>

namespace s87 {

class Diagnostics
{
public:
    static constexpr juce::int64 kMaxBytes = 1024 * 1024;   // ~1 MB, then the older half is dropped

    static juce::File root()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("Studio 87").getChildFile ("SupurrSaw");
    }
    static juce::File folder (const juce::String& name)
    {
        auto f = root().getChildFile (name);
        f.createDirectory();
        return f;
    }
    static juce::File logFile() { return folder ("logs").getChildFile ("supurrsaw.log"); }

    static void log (const juce::String& text)
    {
        const juce::ScopedLock sl (lock());
        auto f = logFile();
        if (f.getSize() > kMaxBytes)
        {
            auto all = f.loadFileAsString();
            auto cut = all.substring (all.length() / 2);
            auto nl = cut.indexOfChar ('\n');
            f.replaceWithText (nl >= 0 ? cut.substring (nl + 1) : cut);
        }
        f.appendText (juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H:%M:%S") + "  " + text + "\n");
    }

    static juce::String report()
    {
        juce::String s;
        s << "Studio 87 SupurrSaw diagnostics\n"
          << "Version: " << JucePlugin_VersionString << "\n"
          << "System: " << juce::SystemStats::getOperatingSystemName() << "\n"
          << "CPU: " << juce::SystemStats::getCpuModel() << ", cores " << juce::SystemStats::getNumCpus() << "\n"
          << "JUCE: " << juce::SystemStats::getJUCEVersion() << "\n"
          << "Data folder: " << root().getFullPathName() << "\n\n"
          << "Log (latest lines):\n";
        auto lines = juce::StringArray::fromLines (logFile().loadFileAsString());
        for (int i = juce::jmax (0, lines.size() - 60); i < lines.size(); ++i) s << lines[i] << "\n";
        return s;
    }

private:
    static juce::CriticalSection& lock() { static juce::CriticalSection cs; return cs; }
};

} // namespace s87
