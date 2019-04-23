#pragma once

#include "JuceHeader.h"

namespace vcp {

class AudioPlugin : public ReferenceCountedObject
{
public:
    AudioPlugin() { }
    ~AudioPlugin() { }
};

typedef ReferenceCountedObjectPtr<AudioPlugin> AudioPluginPtr;

}
