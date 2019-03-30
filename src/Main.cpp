
#include "JuceHeader.h"
#include "MainComponent.h"

class VersicapApplication  : public JUCEApplication
{
public:
    VersicapApplication() {}

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return false; }

    void initialise (const String& commandLine) override
    {
        look.setColour (Slider::backgroundColourId, kv::LookAndFeel_KV1::widgetBackgroundColor.darker());
        LookAndFeel::setDefaultLookAndFeel (&look);
        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const String& commandLine) override
    {
    }

    class MainWindow    : public DocumentWindow
    {
    public:
        MainWindow (String name) 
            : DocumentWindow (name, Desktop::getInstance().getDefaultLookAndFeel()
                                        .findColour (ResizableWindow::backgroundColourId),
                                         DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setBackgroundColour (kv::LookAndFeel_KV1::widgetBackgroundColor.darker());
            setContentOwned (new MainComponent(), true);

            setResizable (false, false);
            constrain.setMinimumSize (440, 300);
            constrain.setMaximumSize (440, 300);
            setConstrainer (&constrain);
            centreWithSize (getWidth(), getHeight());

            setVisible (true);
        }

        ~MainWindow()
        {
            setConstrainer (nullptr);
        }
        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        void maximiseButtonPressed() override
        {
            return;
        }

    private:
        ComponentBoundsConstrainer constrain;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    kv::LookAndFeel_KV1 look;
};

START_JUCE_APPLICATION (VersicapApplication)
