#include "OpDefs/GopTransform.hpp"
#include "Engine/Core/Types.h"
#include "Engine/Parameter/Template.h"
#include <Eigen/src/Core/Matrix.h>
#include <Eigen/src/Geometry/AngleAxis.h>
#include <Eigen/src/Geometry/Transform.h>

GopTransform::GopTransform(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo)
{
}

void GopTransform::cookOp(enzo::op::CookContext context)
{
    using namespace enzo;

    if (outputRequested(0))
    {
        NodePacket packet = context.cloneInputPacket(0);

        const floatT rotateX = context.evalParmFloat("rotate", 0);
        const floatT rotateY = context.evalParmFloat("rotate", 1);
        const floatT rotateZ = context.evalParmFloat("rotate", 2);

        // Compose translation then rotation into one homogeneous transform.
        Eigen::Affine3f transform = Eigen::Affine3f::Identity();
        transform.translate(Vector3(
            context.evalParmFloat("translate", 0),
            context.evalParmFloat("translate", 1),
            context.evalParmFloat("translate", 2)
        ));
        transform.rotate(Eigen::AngleAxisf(rotateX, Vector3(1, 0, 0)));
        transform.rotate(Eigen::AngleAxisf(rotateY, Vector3(0, 1, 0)));
        transform.rotate(Eigen::AngleAxisf(rotateZ, Vector3(0, 0, 1)));

        const Matrix4 matrix = transform.matrix();
        for (unsigned int p = 0; p < packet.size(); ++p)
        {
            packet.getPrimitive(p)->applyTransform(matrix, TransformClass::POINT);
        }

        setOutputPacket(0, packet);
    }
}

std::vector<enzo::prm::Template> GopTransform::parameterList()
{
    return {
        enzo::prm::Template(enzo::prm::Type::XYZ, enzo::prm::Name("translate", "Translate"), 3),
        enzo::prm::Template(enzo::prm::Type::XYZ, enzo::prm::Name("rotate", "Rotate"), 3)
    };
}
