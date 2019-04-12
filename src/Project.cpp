
#include "PluginManager.h"
#include "Project.h"
#include "RenderContext.h"
#include "Tags.h"
#include "Types.h"

namespace vcp {

//=========================================================================
Layer::Layer() : kv::ObjectModel (Tags::layer) { }
Layer::Layer (const ValueTree& data) : kv::ObjectModel (data) { }

bool  Layer::isValid() const        { return objectData.hasType (Tags::layer); }
uint8 Layer::getVelocity() const    { return static_cast<uint8> ((int) getProperty (Tags::velocity)); }

//=========================================================================
Project::Project()
    : ObjectModel (Tags::project)
{
    setMissingProperties();
}

Project::~Project() {}

//=========================================================================
int Project::getSourceType() const
{
    const int type = SourceType::fromSlug (getProperty (Tags::source));
    jassert (type >= SourceType::Begin && type < SourceType::End);
    return type >= SourceType::Begin && type < SourceType::End 
        ? type : SourceType::MidiDevice;
}

//=========================================================================
void Project::getRenderContext (RenderContext& context) const
{
    context.source      = getSourceType();
    context.latency     = (int) getProperty (Tags::latencyComp, 0);
}

//=========================================================================
int Project::getNumLayers() const { return objectData.getChildWithName (Tags::layers).getNumChildren(); }
Layer Project::getLayer (int index) const { return Layer (objectData.getChildWithName (Tags::layers).getChild (index)); }

//=========================================================================
void Project::clearPlugin()
{
    auto plugin = objectData.getChildWithName (Tags::plugin);
    if (plugin.isValid())
    {
        plugin.removeAllChildren (nullptr);
        plugin.removeAllProperties (nullptr);
    }
}

bool Project::getPluginDescription (PluginManager& plugins, PluginDescription& desc) const
{
    auto plugin = objectData.getChildWithName (Tags::plugin);
    auto& list = plugins.getKnownPlugins();
    if (const auto* const type = list.getTypeForIdentifierString (plugin.getProperty (Tags::identifier)))
    {
        desc = *type;
        return true;
    }

    return false;
}

void Project::setPluginDescription (const PluginDescription& desc)
{
    auto plugin = objectData.getChildWithName (Tags::plugin);
    plugin.setProperty (Tags::name, desc.name, nullptr)
          .setProperty (Tags::format, desc.pluginFormatName, nullptr)
          .setProperty (Tags::fileOrId, desc.fileOrIdentifier, nullptr)
          .setProperty (Tags::identifier, desc.createIdentifierString(), nullptr);
}

void Project::updatePluginState (AudioProcessor& processor)
{
    auto plugin = objectData.getChildWithName (Tags::plugin);
    MemoryBlock mb;
    processor.getStateInformation (mb);
    
    if (mb.getSize() > 0)
    {
        MemoryOutputStream mo;
        {
            GZIPCompressorOutputStream gz (mo);
            gz.write (mb.getData(), mb.getSize());
        }

        plugin.setProperty (Tags::state, mo.getMemoryBlock(), nullptr);
    }
}

void Project::applyPluginState (AudioProcessor& processor) const
{
    const auto plugin = objectData.getChildWithName (Tags::plugin);
    if (const auto* state = plugin.getProperty(Tags::state).getBinaryData())
    {
        MemoryInputStream mi (*state, false);
        GZIPDecompressorInputStream gz (mi);
        MemoryBlock mb;
        gz.readIntoMemoryBlock (mb);
        processor.setStateInformation (mb.getData(), static_cast<int> (mb.getSize()));
    }
}

//=========================================================================
bool Project::writeToFile (const File& file) const
{
    TemporaryFile tempFile (file);

    {
        FileOutputStream fo (tempFile.getFile());
        {
            GZIPCompressorOutputStream go (fo);
            objectData.writeToStream (go);
        }
    }

    return tempFile.overwriteTargetFileWithTemporary();
}

bool Project::loadFile (const File& file)
{
    if (! file.existsAsFile())
        return false;
    
    FileInputStream fi (file);
    GZIPDecompressorInputStream gi (fi);
    auto newData = ValueTree::readFromStream (gi);
    if (newData.isValid() && newData.hasType (Tags::project))
        objectData = newData;

    return objectData == newData;
}

void Project::setMissingProperties()
{
    stabilizePropertyString (Tags::name, "");
    stabilizePropertyString (Tags::dataPath, "");
    
    RenderContext context;
    stabilizePropertyString (Tags::source,  SourceType::getSlug (SourceType::MidiDevice));
    stabilizePropertyPOD (Tags::latencyComp,    context.latency);
    stabilizePropertyPOD (Tags::noteStart,      context.keyStart);
    stabilizePropertyPOD (Tags::noteEnd,        context.keyEnd);
    stabilizePropertyPOD (Tags::noteStep,       context.keyStride);
    
    stabilizePropertyString (Tags::baseName,    "Sample");
    stabilizePropertyPOD (Tags::noteLength,     context.noteLength);
    stabilizePropertyPOD (Tags::tailLength,     context.tailLength);

    stabilizePropertyPOD (Tags::bitDepth,   context.bitDepth);

    auto layers = objectData.getOrCreateChildWithName (Tags::layers, nullptr);
    if (layers.getNumChildren() <= 0)
    {
        auto layer = layers.getOrCreateChildWithName (Tags::layer, nullptr);
        layer.setProperty (Tags::velocity, 127, nullptr);
    }

    auto plugin = objectData.getOrCreateChildWithName (Tags::plugin, nullptr);
}

}
