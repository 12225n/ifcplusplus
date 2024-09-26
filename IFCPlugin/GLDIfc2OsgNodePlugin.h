#ifndef GLDIfc2OsgNodePlugin_H
#define GLDIfc2OsgNodePlugin_H

#ifdef GLDIfc2OsgNodePlugin_Exports
#define GLDIfc2OsgNodePlugin_Export __declspec(dllexport)
#else
#define GLDIfc2OsgNodePlugin_Export __declspec(dllimport)
#endif

#include "CIM2OSG/Plugin/IPlugin.hpp"

#include <ifcpp/model/StatusCallback.h>
#include <ifcpp/model/BuildingModel.h>
#include <ifcpp/geometry/GeometryConverter.h>

extern "C" GLDIfc2OsgNodePlugin_Export IPlugin* CreatePlugin();

class GLDIfc2OsgNodePlugin_Export GLDIfc2OsgNodePlugin : public IPlugin
{
public :

    struct IfcFileInfo
    {
        std::string m_filePath;

        std::shared_ptr<GeometryConverter> m_geometryConvertor;

        std::shared_ptr<BuildingModel> m_buildingModel;

        std::unordered_map<std::string, int> m_globalId2IdMap;
    };

    std::unordered_map<std::string, IfcFileInfo*> m_ifcFilePath2IfcFileInfoMap;

    std::mutex m_onIfcPPMessageMutex;

    ~GLDIfc2OsgNodePlugin() override;

    ExternalDataType::ExternalDataType modelType() override;

    const char* comments() override;

    int version() override;

    osg::Node* run(const char* directory, ExternalData* externalData) override;

    /*static*/ void OnIfcPPMessage(/*void* obj_ptr,*/ std::shared_ptr<StatusCallback::Message> t);
};

#endif // GLDIfc2OsgNodePlugin_H