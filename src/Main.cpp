
#ifndef VCP_STLIB

#include "gui/LookAndFeel.h"
#include "gui/MainWindow.h"

#include "Commands.h"
#include "PluginManager.h"
#include "Project.h"
#include "Settings.h"
#include "Versicap.h"

namespace vcp {

class Application : public JUCEApplication,
                    public AsyncUpdater
{
public:
    Application() { }

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }

    void initialise (const String& commandLine) override
    {
        versicap.reset (new Versicap());

        if (maybeLaunchSlave (commandLine))
            return;

        if (sendCommandLineToPreexistingInstance())
        {
            quit();
            return;
        }

        setupGlobals();
        triggerAsyncUpdate();
    }

    void shutdown() override
    {
        shutdownGui();
        versicap->saveSettings();
        versicap->saveRenderContext();
        versicap->shutdown();
        versicap.reset();
    }

    void systemRequestedQuit() override
    {
        if (versicap->hasProjectChanged())
        {
            const auto result = NativeMessageBox::showYesNoBox (AlertWindow::InfoIcon,
                "Versicap", "This project has changed. Would you like to save?",
                Versicap::getMainWindow());

            if (result == 1)
                versicap->getCommandManager().invokeDirectly (Commands::projectSave, false);
        }

        quit();
    }

    void anotherInstanceStarted (const String& commandLine) override
    {
        ignoreUnused (commandLine);
    }

    void handleAsyncUpdate() override
    {
        const auto file = versicap->getSettings().getLastProject();
        if (file.existsAsFile())
        {
            DBG("[VCP] loading last project: " << file.getFullPathName());
            versicap->loadProject (file);
        }

        versicap->launched();
    }

private:
    std::unique_ptr<Versicap> versicap;
    OwnedArray<kv::ChildProcessSlave>   slaves;

    void setupGlobals()
    {
        versicap->initialize();
        auto& plugins = versicap->getPluginManager();
        plugins.scanAudioPlugins ({ "AudioUnit", "VST", "VST3" });
    }

    void shutdownGui()
    {
        versicap->closePluginWindow();        
    }

    bool maybeLaunchSlave (const String& commandLine)
    {
        slaves.clearQuick (true);
        slaves.add (versicap->getPluginManager().createAudioPluginScannerSlave());
        StringArray processIds = { VCP_PLUGIN_SCANNER_PROCESS_ID };
        for (auto* slave : slaves)
        {
            for (const auto& pid : processIds)
            {
                if (slave->initialiseFromCommandLine (commandLine, pid))
                {
				   #if JUCE_MAC
                    Process::setDockIconVisible (false);
				   #endif
                    juce::shutdownJuce_GUI();
                    return true;
                }
            }
        }
        
        return false;
    }
};

}

START_JUCE_APPLICATION (vcp::Application)
#endif
