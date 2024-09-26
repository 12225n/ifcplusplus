#include "GLDIfc2OsgNodePlugin.h"

#include "CIM2OSG/Plugin/IPlugin.hpp"

#include <ifcpp/model/BuildingModel.h>
#include <ifcpp/geometry/GeometrySettings.h>
#include <ifcpp/geometry/GeometryConverter.h>
#include <ifcpp/reader/ReaderSTEP.h>
#include <ifcpp/geometry/ConverterOSG.h>

#include <osg/Group>
#include <osg/ref_ptr>
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
{
	GLDIfc2OsgNodePlugin::IfcFileInfo* tmpIfcFileInfo = NULL;
	for (std::unordered_map<std::string, GLDIfc2OsgNodePlugin::IfcFileInfo*>::iterator tmpIter = m_ifcFilePath2IfcFileInfoMap.begin();
																							tmpIter != m_ifcFilePath2IfcFileInfoMap.end(); ++tmpIter)
	{
		tmpIfcFileInfo = tmpIter->second;

		delete tmpIfcFileInfo;
		tmpIfcFileInfo = NULL;

		tmpIter->second = NULL;
	}

	m_ifcFilePath2IfcFileInfoMap.clear();
}

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

osg::Node* GLDIfc2OsgNodePlugin::run(const char* tmpCIMLevelDir, ExternalData* externalData) // override
{
	std::string tmpIfcFilePath = std::string(tmpCIMLevelDir) + '/' + externalData->url();

	IfcFileInfo* tmpIfcFileInfo = NULL;
	std::shared_ptr<GeometryConverter> tmpGeometryConvertor = NULL;

	std::unordered_map<std::string, IfcFileInfo*>::iterator tmpIter = m_ifcFilePath2IfcFileInfoMap.find(tmpIfcFilePath);
	if (tmpIter == m_ifcFilePath2IfcFileInfoMap.end())
	{
		std::shared_ptr<BuildingModel> tmpBuildingModel = std::make_shared<BuildingModel>();

		std::shared_ptr<GeometrySettings> tmpGeometrySettings = std::make_shared<GeometrySettings>();

		// reset the IFC model
		tmpGeometryConvertor = std::make_shared<GeometryConverter>(tmpBuildingModel, tmpGeometrySettings);
		tmpGeometryConvertor->clearMessagesCallback();
		tmpGeometryConvertor->resetModel();
		tmpGeometryConvertor->getGeomSettings()->setNumVerticesPerCircle(16);
		tmpGeometryConvertor->getGeomSettings()->setMinNumVerticesPerArc(4);

		std::stringstream err;

		// load file to IFC model
		shared_ptr<ReaderSTEP> step_reader(new ReaderSTEP());
		step_reader->setMessageCallBack(std::bind(&GLDIfc2OsgNodePlugin::OnIfcPPMessage, this, std::placeholders::_1));
		step_reader->loadModelFromFile(tmpIfcFilePath, tmpGeometryConvertor->getBuildingModel());

		tmpIfcFileInfo = new IfcFileInfo;

		tmpIfcFileInfo->m_filePath = tmpIfcFilePath;
		tmpIfcFileInfo->m_geometryConvertor = tmpGeometryConvertor;
		tmpIfcFileInfo->m_buildingModel = tmpBuildingModel;

		m_ifcFilePath2IfcFileInfoMap.insert(std::make_pair(tmpIfcFilePath, tmpIfcFileInfo));

		std::unordered_map<int, std::shared_ptr<BuildingEntity>>& tmpId2BuildingEntityMap = tmpBuildingModel->getMapIfcEntities();

		std::string tmpGlobalId;
		std::shared_ptr<BuildingEntity> tmpBuildingEntity;
		std::shared_ptr<IfcRoot> tmpIfcRoot;
		for (std::unordered_map<int, std::shared_ptr<BuildingEntity>>::iterator tmpIter = tmpId2BuildingEntityMap.begin();
			tmpIter != tmpId2BuildingEntityMap.end(); ++tmpIter)
		{
			tmpBuildingEntity = tmpIter->second;

			tmpIfcRoot = std::dynamic_pointer_cast<IfcRoot>(tmpBuildingEntity);
			if (tmpIfcRoot != NULL)
			{
				tmpGlobalId = tmpIfcRoot->m_GlobalId->m_value;

				tmpIfcFileInfo->m_globalId2IdMap.insert(std::make_pair(tmpGlobalId, tmpIter->first));
			}
		}

		// convert IFC geometric representations into Carve geometry
		tmpGeometryConvertor->setCsgEps(1.5e-08);
		tmpGeometryConvertor->convertGeometry();
	}
	else if (tmpIter != m_ifcFilePath2IfcFileInfoMap.end())
	{
		tmpIfcFileInfo = tmpIter->second;
		tmpGeometryConvertor = tmpIfcFileInfo->m_geometryConvertor;
	}

	// first remove previously loaded geometry from scenegraph
	osg::ref_ptr<osg::Group> modelNode = new osg::Group;

	std::shared_ptr<BuildingModel> tmpBuildingModel = tmpGeometryConvertor->getBuildingModel();

	std::unordered_map<std::string, std::shared_ptr<ProductShapeData>> tmpGlobalId2ProductShapeDataMap = tmpGeometryConvertor->getShapeInputData();

	std::shared_ptr<ProductShapeData> tmpProductShapeData = NULL;
	
	std::unordered_map<std::string, std::shared_ptr<ProductShapeData>>::iterator tmpIter2 = tmpGlobalId2ProductShapeDataMap.find("0hHTNQdJP9n9lfUe_inZJs");
	if (tmpIter2 == tmpGlobalId2ProductShapeDataMap.end())
	{
		qDebug() << "GLDIfc2OsgNodePlugin, GLDIfc2OsgNodePlugin::Run, tmpIfcFilePath : " << tmpIfcFilePath.c_str()
															<< ", Could Not Find The Entity GlobalId : " << "0hHTNQdJP9n9lfUe_inZJs";

		return NULL;
	}

	tmpProductShapeData = tmpIter2->second;

	// convert Carve geometry to OSG
	shared_ptr<ConverterOSG> converter_osg(new ConverterOSG(tmpGeometryConvertor->getGeomSettings()));
	converter_osg->setMessageTarget(tmpGeometryConvertor.get());

	std::unordered_map<std::string, std::shared_ptr<ProductShapeData>> tmpShapeInputDatas;
	tmpShapeInputDatas.insert(std::make_pair(tmpIter2->first, tmpProductShapeData));

	converter_osg->convertToOSG(tmpShapeInputDatas, modelNode);

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

	tmpGeometryConvertor->clearIfcRepresentationsInModel(true, true, false);
	tmpGeometryConvertor->clearInputCache();

	return modelNode.release();
}

void GLDIfc2OsgNodePlugin::OnIfcPPMessage(/*void* obj_ptr,*/ shared_ptr<StatusCallback::Message> tmpStatusCallbackMessage)
{
	//GLDIfc2OsgNodePlugin* tmpThiz = static_cast<GLDIfc2OsgNodePlugin*>(obj_ptr);
	//if (tmpThiz == NULL)
	//{
	//	return;
	//}

	std::lock_guard<std::mutex> lock(this/*tmpThiz*/->m_onIfcPPMessageMutex);

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
		
		qInfo() << "GLDIfc2OsgNodePlugin, GLDIfc2OsgNodePlugin::OnIfcPPMessage, GeneralMessage : " << qt_str;
	}
	else if (tmpStatusCallbackMessage->m_message_type == StatusCallback::MESSAGE_TYPE_WARNING)
	{
		QString qt_str = QString::fromStdWString(message_str);

		qWarning() << "GLDIfc2OsgNodePlugin, GLDIfc2OsgNodePlugin::OnIfcPPMessage, WarningMessage : " << qt_str;
	}
	else if (tmpStatusCallbackMessage->m_message_type == StatusCallback::MESSAGE_TYPE_ERROR)
	{
		QString qt_str = QString::fromStdWString(message_str);

		qCritical() << "GLDIfc2OsgNodePlugin, GLDIfc2OsgNodePlugin::OnIfcPPMessage, ErrMessage : " << qt_str;
	}
	else if (tmpStatusCallbackMessage->m_message_type == StatusCallback::MESSAGE_TYPE_PROGRESS_VALUE)
	{
		qDebug() << "GLDIfc2OsgNodePlugin, GLDIfc2OsgNodePlugin::OnIfcPPMessage, ProgressValue : " << tmpStatusCallbackMessage->m_progress_value
																			<< ", ProgressType : " << tmpStatusCallbackMessage->m_progress_type
																			<< ", ProgressText : " << tmpStatusCallbackMessage->m_progress_text;
	}
	else if (tmpStatusCallbackMessage->m_message_type == StatusCallback::MESSAGE_TYPE_CLEAR_MESSAGES)
	{
		qDebug() << "GLDIfc2OsgNodePlugin, GLDIfc2OsgNodePlugin::OnIfcPPMessage, ClearMessage";
	}
}