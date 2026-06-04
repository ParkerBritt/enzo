#include "OpDefs/GopTransform.hpp"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Parameter/Template.h"
#include <Eigen/src/Core/Matrix.h>
#include <Eigen/src/Geometry/AngleAxis.h>
#include <Eigen/src/Geometry/Transform.h>
#include <cstddef>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

GopTransform::GopTransform(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo)
{
}

void GopTransform::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if (outputRequested(0))
    {
        NodePacket packet = context.cloneInputPacket(0);

        Eigen::Affine3d transform = Eigen::Affine3d::Identity();

        const floatT rotateX = context.evalFloatParm("rotate", 0);
        const floatT rotateY = context.evalFloatParm("rotate", 1);
        const floatT rotateZ = context.evalFloatParm("rotate", 2);
        transform.rotate(Eigen::AngleAxisd(rotateX, Eigen::Vector3d(1, 0, 0)));
        transform.rotate(Eigen::AngleAxisd(rotateY, Eigen::Vector3d(0, 1, 0)));
        transform.rotate(Eigen::AngleAxisd(rotateZ, Eigen::Vector3d(0, 0, 1)));

        const Eigen::Matrix3d R = transform.linear();
        const Eigen::Vector3d t = Vector3(
            context.evalFloatParm("translate", 0),
            context.evalFloatParm("translate", 1),
            context.evalFloatParm("translate", 2)
        );

        for (unsigned int p = 0; p < packet.size(); ++p)
        {
            auto geo = packet.getPrimitive(p);
            auto PAttr = geo->getAttribByName(attr::AttrOwner::POINT, "P", true);
            attr::AttributeHandleVector3 PAttrHandle(PAttr);
            const size_t N = PAttrHandle.getSize();

            tbb::parallel_for(
                tbb::blocked_range<size_t>(0, N),
                [&](tbb::blocked_range<size_t> range) {
                    for (size_t i = range.begin(); i < range.end(); ++i)
                    {
                        const enzo::Vector3 pointPos = (R * PAttrHandle[i]) + t;
                        PAttrHandle.setValue(i, pointPos);
                    }
                }
            );
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
