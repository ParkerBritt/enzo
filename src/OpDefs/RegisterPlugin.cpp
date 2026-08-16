#include "Engine/Network/NodeType.h"
#include "Engine/Network/NodeTypeTable.h"
#include "GopGeometryImport.h"
#include "GopHouse.h"
#include "GopOceanSurface.h"
#include "OpDefs/GopBoolean.h"
#include "OpDefs/GopCamera.h"
#include "OpDefs/GopCircle.h"
#include "OpDefs/GopCopyToPoints.h"
#include "OpDefs/GopCube.h"
#include "OpDefs/GopDelete.h"
#include "OpDefs/GopExtrude.h"
#include "OpDefs/GopGrid.h"
#include "OpDefs/GopMerge.h"
#include "OpDefs/GopPath.h"
#include "OpDefs/GopSineWave.h"
#include "OpDefs/GopSweep.h"
#include "OpDefs/GopTransform.hpp"
#include <boost/config.hpp>
#include <boost/dll.hpp>

extern "C" {
BOOST_SYMBOL_EXPORT void newNodeLibrary(enzo::nt::addNodeTypePtr addNodeType)
{
    addNodeType(
        enzo::nt::NodeType{
            "transform",
            "Transform",
            &GopTransform::ctor,
            GopTransform::parameterList(),
            1,
            1,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "merge",
            "Merge",
            &GopMerge::ctor,
            GopMerge::parameterList(),
            2,
            2,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "geometryImport",
            "Geometry Import",
            &GopGeometryImport::ctor,
            GopGeometryImport::parameterList(),
            0,
            0,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "grid",
            "Grid",
            &GopGrid::ctor,
            GopGrid::parameterList(),
            0,
            0,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "sineWave",
            "Sine Wave",
            &GopSineWave::ctor,
            GopSineWave::parameterList(),
            1,
            1,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "oceanSurface",
            "Ocean Surface",
            &GopOceanSurface::ctor,
            GopOceanSurface::parameterList(),
            1,
            1,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "path",
            "Path",
            &GopPath::ctor,
            GopPath::parameterList(),
            1,
            1,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "camera",
            "Camera",
            &GopCamera::ctor,
            GopCamera::parameterList(),
            0,
            0,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "copyToPoints",
            "Copy To Points",
            &GopCopyToPoints::ctor,
            GopCopyToPoints::parameterList(),
            2,
            2,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "delete",
            "Delete",
            &GopDelete::ctor,
            GopDelete::parameterList(),
            1,
            1,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "cube",
            "Cube",
            &GopCube::ctor,
            GopCube::parameterList(),
            0,
            0,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "extrude",
            "Extrude",
            &GopExtrude::ctor,
            GopExtrude::parameterList(),
            1,
            1,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "boolean",
            "Boolean",
            &GopBoolean::ctor,
            GopBoolean::parameterList(),
            2,
            2,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "circle",
            "Circle",
            &GopCircle::ctor,
            GopCircle::parameterList(),
            0,
            0,
            1,
        }
    );
    addNodeType(
        enzo::nt::NodeType{
            "sweep",
            "Sweep",
            &GopSweep::ctor,
            GopSweep::parameterList(),
            1,
            2,
            1,
        }
    );
}
}
