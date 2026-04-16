#include "Engine/Operator/OpInfo.h"
#include "Engine/Operator/OperatorTable.h"
#include "GopGeometryImport.h"
#include "GopHouse.h"
#include "GopOceanSurface.h"
#include "GopTestCube.h"
#include "OpDefs/GopCamera.h"
#include "OpDefs/GopCopyToPoints.h"
#include "OpDefs/GopGrid.h"
#include "OpDefs/GopMerge.h"
#include "OpDefs/GopPath.h"
#include "OpDefs/GopSineWave.h"
#include "OpDefs/GopTransform.hpp"
#include <boost/config.hpp>
#include <boost/dll.hpp>

extern "C" {
BOOST_SYMBOL_EXPORT void newSopOperator(enzo::op::addOperatorPtr addOperator) {
    addOperator(enzo::op::OpInfo{
        "transform",
        "Transform",
        &GopTransform::ctor,
        GopTransform::parameterList,
        1,
        1,
        1,
    });
    addOperator(enzo::op::OpInfo{
        "merge",
        "Merge",
        &GopMerge::ctor,
        GopMerge::parameterList,
        2,
        2,
        1,
    });
    // addOperator(
    //     enzo::op::OpInfo {
    //         "house",
    //         "House",
    //         &GOP_house::ctor,
    //         GOP_house::parameterList,
    //         0,
    //         0,
    //         1,
    //     }
    // );
    // addOperator(
    //     enzo::op::OpInfo {
    //         "testGeoCube",
    //         "Test Cube",
    //         &GopTestGeoCube::ctor,
    //         GopTestGeoCube::parameterList,
    //         0,
    //         0,
    //         1,
    //     }
    // );
    addOperator(enzo::op::OpInfo{
        "geometryImport",
        "Geometry Import",
        &GopGeometryImport::ctor,
        GopGeometryImport::parameterList,
        0,
        0,
        1,
    });
    addOperator(enzo::op::OpInfo{
        "grid",
        "Grid",
        &GopGrid::ctor,
        GopGrid::parameterList,
        0,
        0,
        1,
    });
    addOperator(enzo::op::OpInfo{
        "sineWave",
        "Sine Wave",
        &GopSineWave::ctor,
        GopSineWave::parameterList,
        1,
        1,
        1,
    });
    addOperator(enzo::op::OpInfo{
        "oceanSurface",
        "Ocean Surface",
        &GopOceanSurface::ctor,
        GopOceanSurface::parameterList,
        1,
        1,
        1,
    });
    addOperator(enzo::op::OpInfo{
        "path",
        "Path",
        &GopPath::ctor,
        GopPath::parameterList,
        1,
        1,
        1,
    });
    addOperator(enzo::op::OpInfo{
        "camera",
        "Camera",
        &GopCamera::ctor,
        GopCamera::parameterList,
        0,
        0,
        1,
    });
    addOperator(enzo::op::OpInfo{
        "copyToPoints",
        "Copy To Points",
        &GopCopyToPoints::ctor,
        GopCopyToPoints::parameterList,
        2,
        2,
        1,
    });
}
}
