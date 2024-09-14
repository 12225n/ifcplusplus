#include "GLDIfc2OsgNodePlugin.h"

#include "CIM2OSG/Plugin/IPlugin.hpp"

#include <ifcpp/model/BuildingModel.h>
#include <ifcpp/geometry/GeometrySettings.h>
#include <ifcpp/geometry/GeometryConverter.h>
#include <ifcpp/reader/ReaderSTEP.h>
#include <ifcpp/geometry/ConverterOSG.h>

#include <osg/Group>
#include <osgUtil/Optimizer>

#include <QtCore/QString>
#include <QtCore/QDebug>

//#include <shared_ptr.h>

IPlugin* CreatePlugin()
{
	static GLDIfc2OsgNodePlugin m_gldIfc2OsgNodePlugin;

	return &m_gldIfc2OsgNodePlugin;
}

GLDIfc2OsgNodePlugin::~GLDIfc2OsgNodePlugin() // override
{}

ExternalDataType::ExternalDataType GLDIfc2OsgNodePlugin::modelType() // override
{
	return ExternalDataType::ExternalDataType::IFC;
}

const char* GLDIfc2OsgNodePlugin::comments() // override
{
	return "GLDIfc2OsgNodePlugin";
}

int GLDIfc2OsgNodePlugin::version() // override
{
	return 1.0;
}

osg::Node* GLDIfc2OsgNodePlugin::run(const char* directory, ExternalData* externalData) // override
{
	std::string tmpIfcFilePath;

	// first remove previously loaded geometry from scenegraph
	osg::ref_ptr<osg::Group> modelNode = new osg::Group; // m_system->getViewController()->getModelNode();
	//SceneGraphUtils::clearAllChildNodes(modelNode);
	//m_system->clearSelection();

	//BuildingModel* tmpBuildingModel = new BuildingModel;
	std::shared_ptr<BuildingModel> tmpBuildingModel = std::make_shared<BuildingModel>();

	//GeometrySettings* tmpGeometrySettings = new GeometrySettings();
	std::shared_ptr<GeometrySettings> tmpGeometrySettings = std::make_shared<GeometrySettings>();

	//GeometryConverter* tmpGeometryConverter = new GeometryConverter(tmpBuildingModel, tmpGeometrySettings);

	// reset the IFC model
	std::shared_ptr<GeometryConverter> geometry_converter = std::make_shared<GeometryConverter>(tmpBuildingModel, tmpGeometrySettings); // m_system->m_geometry_converter;
	geometry_converter->clearMessagesCallback();
	geometry_converter->resetModel();
	geometry_converter->getGeomSettings()->setNumVerticesPerCircle(16);
	geometry_converter->getGeomSettings()->setMinNumVerticesPerArc(4);
	std::stringstream err;

	// load file to IFC model
	shared_ptr<ReaderSTEP> step_reader(new ReaderSTEP());
	step_reader->setMessageCallBack(std::bind(&GLDIfc2OsgNodePlugin::OnIfcPPMessage, this, std::placeholders::_1));
	step_reader->loadModelFromFile(/*path_str*/ tmpIfcFilePath, geometry_converter->getBuildingModel());

	// convert IFC geometric representations into Carve geometry
	geometry_converter->setCsgEps(1.5e-08);
	geometry_converter->convertGeometry();

	// convert Carve geometry to OSG
	shared_ptr<ConverterOSG> converter_osg(new ConverterOSG(geometry_converter->getGeomSettings()));
	converter_osg->setMessageTarget(geometry_converter.get());
	converter_osg->convertToOSG(geometry_converter->getShapeInputData(), modelNode);

	if (modelNode)
	{
		bool optimize = true;
		if (optimize)
		{
			osgUtil::Optimizer opt;
			opt.optimize(modelNode);
		}

		// if model bounding sphere is far from origin, move to origin
		const osg::BoundingSphere& bsphere = modelNode->getBound();
		if (bsphere.center().length() > 10000)
		{
			if (bsphere.center().length() / bsphere.radius() > 100)
			{
				osg::MatrixTransform* mt = new osg::MatrixTransform();
				mt->setMatrix(osg::Matrix::translate(-bsphere.center() * 0.98));

				int num_children = modelNode->getNumChildren();
				for (int i = 0; i < num_children; ++i)
				{
					osg::Node* node = modelNode->getChild(i);
					if (!node)
					{
						continue;
					}
					mt->addChild(node);
				}
				SceneGraphUtils::removeChildren(modelNode);
				modelNode->addChild(mt);
			}
		}
	}

	geometry_converter->clearIfcRepresentationsInModel(true, true, false);
	geometry_converter->clearInputCache();

	return NULL;
}

void GLDIfc2OsgNodePlugin::OnIfcPPMessage(void* obj_ptr, shared_ptr<StatusCallback::Message> tmpStatusCallbackMessage)
{
	GLDIfc2OsgNodePlugin* tmpThiz = static_cast<GLDIfc2OsgNodePlugin*>(obj_ptr);
	if (tmpThiz == NULL)
	{
		return;
	}

	std::lock_guard<std::mutex> lock(/*this*/tmpThiz->m_onIfcPPMessageMutex);

	std::string reporting_function_str(tmpStatusCallbackMessage->m_reporting_function);
	std::wstringstream strs_report;
	if (reporting_function_str.size() > 0)
	{
		strs_report << tmpStatusCallbackMessage->m_reporting_function << ", ";
	}

	strs_report << tmpStatusCallbackMessage->m_message_text.c_str();

	if (tmpStatusCallbackMessage->m_entity)
	{
		BuildingEntity* ent = dynamic_cast<BuildingEntity*>(tmpStatusCallbackMessage->m_entity);
		if (ent)
		{
			strs_report << ", IFC entity: #" << ent->m_tag << "=" << EntityFactory::getStringForClassID(tmpStatusCallbackMessage->m_entity->classID());
		}
	}

	std::wstring message_str = strs_report.str().c_str();

	if (tmpStatusCallbackMessage->m_message_type == StatusCallback::MESSAGE_TYPE_GENERAL_MESSAGE)
	{
		QString qt_str = QString::fromStdWString(message_str);
		//myself->txtOut(qt_str);
		
		qInfo() << "GLDIfc2OsgNodePlugin, GLDIfc2OsgNodePlugin::OnIfcPPMessage, GeneralMessage : " << qt_str;
	}
	else if (tmpStatusCallbackMessage->m_message_type == StatusCallback::MESSAGE_TYPE_WARNING)
	{
		QString qt_str = QString::fromStdWString(message_str);
		//myself->txtOutWarning(qt_str);

		qWarning() << "GLDIfc2OsgNodePlugin, GLDIfc2OsgNodePlugin::OnIfcPPMessage, WarningMessage : " << qt_str;
	}
	else if (tmpStatusCallbackMessage->m_message_type == StatusCallback::MESSAGE_TYPE_ERROR)
	{
		QString qt_str = QString::fromStdWString(message_str);
		//myself->txtOutError(qt_str);

		qCritical() << "GLDIfc2OsgNodePlugin, GLDIfc2OsgNodePlugin::OnIfcPPMessage, ErrMessage : " << qt_str;
	}
	else if (tmpStatusCallbackMessage->m_message_type == StatusCallback::MESSAGE_TYPE_PROGRESS_VALUE)
	{
		//myself->progressValue(m->m_progress_value, m->m_progress_type);

		qDebug() << "GLDIfc2OsgNodePlugin, GLDIfc2OsgNodePlugin::OnIfcPPMessage, ProgressValue : " << tmpStatusCallbackMessage->m_progress_value
																			<< ", ProgressType : " << tmpStatusCallbackMessage->m_progress_type
																			<< ", ProgressText : " << tmpStatusCallbackMessage->m_progress_text;;
	}
	else if (tmpStatusCallbackMessage->m_message_type == StatusCallback::MESSAGE_TYPE_CLEAR_MESSAGES)
	{
		//myself->clearTxtOut();

		qDebug() << "GLDIfc2OsgNodePlugin, GLDIfc2OsgNodePlugin::OnIfcPPMessage, ClearMessage";
	}
}