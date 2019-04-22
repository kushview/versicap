
#include "controllers/ProjectsController.h"
#include "Commands.h"
#include "Project.h"
#include "ProjectWatcher.h"

namespace vcp {

class ProjectDocument : public FileBasedDocument
{
public:
    ProjectDocument (Versicap& vc)
        : FileBasedDocument ("versicap", "*.versicap", "Open Project", "Save Project"),
          versicap (vc) { }
    ~ProjectDocument() = default;

protected:
    String getDocumentTitle() override
    {
        const auto title = versicap.getProject().getProperty (Tags::name).toString();
        return title.isNotEmpty() ? title : "Untitled Project";
    }

    Result loadDocument (const File& file) override
    {
        if (! versicap.loadProject (file))
            return Result::fail ("Could not open project");
        return Result::ok();
    }

    Result saveDocument (const File& file) override
    {
        if (! versicap.saveProject (file))
            return Result::fail ("Could not save project");
        return Result::ok();
    }

    File getLastDocumentOpened() override
    {
        return lastOpenedFile;
    }

    void setLastDocumentOpened (const File& file) override
    {
        lastOpenedFile = file;
    }

private:
    Versicap& versicap;
    File lastOpenedFile;
};

ProjectsController::ProjectsController (Versicap& vc)
        : Controller (vc) { }
ProjectsController::~ProjectsController() {}

void ProjectsController::initialize()
{
    document.reset (new ProjectDocument (versicap));
    versicap.getDeviceManager().addChangeListener (this);
    watcher.setProject (versicap.getProject());
    watcher.onProjectModified = std::bind (&ProjectDocument::changed, document.get());
}

void ProjectsController::shutdown()
{
    watcher.onProjectModified = nullptr;
    versicap.getDeviceManager().removeChangeListener (this);
    document.reset();
}

void ProjectsController::projectChanged()
{
    watcher.setProject (versicap.getProject());
    document->setChangedFlag (false);
}

void ProjectsController::save()
{
    const auto result = document->save (true, true);
}

void ProjectsController::saveAs()
{
    FileChooser chooser ("Save Project As...", File(), "*.versicap", true, false, nullptr);
    if (! chooser.browseForFileToSave (true))
        return;
    const auto result = document->saveAs (chooser.getResult(), false, false, false);
}

void ProjectsController::open()
{
    FileChooser chooser ("Open Project", File(), "*.versicap", true, false, nullptr);
    if (! chooser.browseForFileToOpen())
        return;
    auto result = document->loadFrom (chooser.getResult(), false);
    if (! result.wasOk())
    {
        AlertWindow::showNativeDialogBox ("Versicap", result.getErrorMessage(), false);
    }
    else
    {
        DBG("[VCP] project opened ok");
    }
}

void ProjectsController::create()
{

    AlertWindow window ("New Project", "Enter the project's name", AlertWindow::NoIcon);
    window.addTextEditor ("Name", "");
    window.addButton ("Ok", 1);
    window.addButton ("Cancel", 0);
    const int result = window.runModalLoop();
    if (result == 0)
        return;

    document->saveIfNeededAndUserAgrees();

    Project newProject;
    String name = window.getTextEditor("Name")->getText();
    newProject.setProperty (Tags::name, name);
    const auto dataPath = Versicap::getUserDataPath().getChildFile("Projects").getNonexistentChildFile (name,"");
    dataPath.createDirectory();
    newProject.setProperty (Tags::dataPath, dataPath.getFullPathName());
    const auto filename = dataPath.getChildFile (name + String (".versicap"));
    newProject.writeToFile (filename);
    document->setFile (filename);
    document->setChangedFlag (false);
    versicap.loadProject (filename);
}

void ProjectsController::changeListenerCallback (ChangeBroadcaster*)
{
    auto project = versicap.getProject();
    AudioDeviceManager::AudioDeviceSetup setup;
    versicap.getDeviceManager().getAudioDeviceSetup (setup);
    project.setProperty (Tags::sampleRate, setup.sampleRate)
           .setProperty (Tags::bufferSize, setup.bufferSize)
           .setProperty (Tags::audioInput, setup.inputDeviceName)
           .setProperty (Tags::audioOutput, setup.outputDeviceName);
}

void ProjectsController::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    switch (commandID)
    {
        case Commands::projectSave:
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier);
            result.setInfo ("Save Project", "Save the current project", "Project", 0);
            break;
        case Commands::projectSaveAs:
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Save Project As", "Save the current project", "Project", 0);
            break;
        case Commands::projectOpen:
            result.addDefaultKeypress ('o', ModifierKeys::commandModifier);
            result.setInfo ("Open Project", "Open an existing project", "Project", 0);
            break;
        case Commands::projectNew:
            result.addDefaultKeypress ('n', ModifierKeys::commandModifier);
            result.setInfo ("New Project", "Create a new project", "Project", 0);
            break;
    }
}

bool ProjectsController::perform (const ApplicationCommandTarget::InvocationInfo& info)
{
    bool handled = true;

    switch (info.commandID)
    {
        case Commands::projectSave:     save();     break;
        case Commands::projectSaveAs:   saveAs();   break;
        case Commands::projectNew:      create();   break;
        case Commands::projectOpen:     open();     break;
        default: handled = false;
            break;
    }

    return handled;
}

}
