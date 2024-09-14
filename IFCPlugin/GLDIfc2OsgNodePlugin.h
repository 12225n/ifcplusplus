#ifndef GLDIfc2OsgNodePlugin_H
#define GLDIfc2OsgNodePlugin_H

#include "CIM2OSG/Plugin/IPlugin.hpp"

#include <ifcpp/model/StatusCallback.h>

IPlugin* CreatePlugin();

class GLDIfc2OsgNodePlugin : public IPlugin
{
public :

    std::mutex m_onIfcPPMessageMutex;

    ~GLDIfc2OsgNodePlugin() override;

    ExternalDataType::ExternalDataType modelType() override;

    const char* comments() override;

    int version() override;

    osg::Node* run(const char* directory, ExternalData* externalData) override;

    static void OnIfcPPMessage(void* obj_ptr, std::shared_ptr<StatusCallback::Message> t);
};

#endif // GLDIfc2OsgNodePlugin_H