project(GLDIfc2OsgNodePlugin)

ADD_DEFINITIONS(-DIFCQUERY_STATIC_LIB)

set(CMAKE_DEBUG_POSTFIX _d)

set(G_TmpIfcPlusPlusInstallPath D:/ifcplusplus/Install_DebugAndRelease)
set(G_TmpQtHomePath D:/Qt/6.7.1/msvc2019_64)

find_package(OpenSceneGraph COMPONENTS osg osgViewer osgGA osgDB osgManipulator osgSim osgParticle osgText osgUtil osgTerrain)
find_package(Qt6 COMPONENTS Core)

set(G_TmpOpenCim2Dir D:/open-cim2)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/OpenCIM)
include_directories(${G_TmpOpenCim2Dir}/OpenCIM)
include_directories(${CMAKE_SOURCE_DIR}/OpenCIM/LibCIMKernel/Include)
include_directories(${G_TmpOpenCim2Dir}/OpenCIM/LibCIMKernel/Include)
include_directories(${OSG_INCLUDE_DIR})

#[[
include_directories(${G_TmpIfcPlusPlusInstallPath}/include/)
# include_directories(${G_TmpIfcPlusPlusInstallPath}/include/ifcpp/)
# include_directories(${G_TmpIfcPlusPlusInstallPath}/include/ifcpp/IFC4X3)
include_directories(${G_TmpIfcPlusPlusInstallPath}/include/ifcpp/IFC4X3/include)
# include_directories(${G_TmpIfcPlusPlusInstallPath}/include/ifcpp/external/include/Carve/src/include)
# include_directories(${G_TmpIfcPlusPlusInstallPath}/include/external/include/Carve/src/include)
include_directories(${G_TmpIfcPlusPlusInstallPath}/include/external/Carve/src/include)
include_directories(${G_TmpIfcPlusPlusInstallPath}/include/external/glm)
include_directories(${G_TmpIfcPlusPlusInstallPath}/include/external)
]]

include_directories(
	${IFCPP_SOURCE_DIR}/IfcPlusPlus/src
	${IFCPP_SOURCE_DIR}/IfcPlusPlus/src/ifcpp/IFC4X3/include
	${IFCPP_SOURCE_DIR}/IfcPlusPlus/src/external
	${IFCPP_SOURCE_DIR}/IfcPlusPlus/src/external/glm
	${IFCPP_SOURCE_DIR}/IfcPlusPlus/src/external/Carve/include
	${IFCPP_SOURCE_DIR}/IfcPlusPlus/src/external/Carve/src
	${IFCPP_SOURCE_DIR}/IfcPlusPlus/src/external/Carve/src/include
	${IFCPP_SOURCE_DIR}/IfcPlusPlus/src/external/Carve/src/common
	${IFCPP_SOURCE_DIR}/IfcPlusPlus/src/external/Carve/build/src
	${IFCPP_SOURCE_DIR}/IfcPlusPlus/src/external/glm
)

include_directories(${G_TmpQtHomePath}/include)

# link_directories(${G_TmpIfcPlusPlusInstallPath}/bin)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/Debug)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/Release/lib)

file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(GLDIfc2OsgNodePlugin SHARED ${SOURCE_FILES})

set(G_TmpIfcPlusPlusLib
			debug IfcPlusPlusd.lib
			optimized IfcPlusPlus.lib)

target_link_libraries(GLDIfc2OsgNodePlugin ${OPENSCENEGRAPH_LIBRARIES} ${G_TmpIfcPlusPlusLib} Qt6::Core)