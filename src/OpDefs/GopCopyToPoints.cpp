#include "OpDefs/GopCopyToPoints.h"
#include "Engine/Operator/Primitive.h"
#include "Engine/Operator/Transform.h"
#include "Engine/Types.h"
#include <memory>
#include <string>
#include <unordered_map>

GopCopyToPoints::GopCopyToPoints(enzo::nt::NetworkManager *network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo) {}

void GopCopyToPoints::cookOp(enzo::op::Context context) {
    using namespace enzo;

    if (outputRequested(0)) {
        // TODO: don't need to clone the input packets, should really just get a const reference to
        // them Actually should implement a copy on write class for them, only copying modified
        // attributes I made an issue for it here https://github.com/ParkerBritt/enzo/issues/42
        NodePacket prototypePacket = context.cloneInputPacket(0);
        NodePacket pointPacket = context.cloneInputPacket(1);
        NodePacket outputPacket;

        // Iterate through every primitve on the point packet, getting their transforms
        // POINT_PRIORITY means it will iterate over points, using the P attribute if possible,
        // otherwise it will fall back to using primitive transforms (transform attribute) if the
        // primitive has any
        for (Transform transform : pointPacket.getTransforms(TransformClass::POINT_PRIORITY)) {

            // Iterate through every prototype primitive, copying it onto the point
            for (auto &prim : prototypePacket.getPrimitives()) {

                // Skip primitives that cannot be transformed
                if (prim->transformType() == TransformClass::NONE)
                    continue;

                // Copy and transform primitive
                std::shared_ptr<geo::Primitive> newPrim = prim->clone();
                newPrim->applyTransform(transform, TransformClass::POINT_PRIORITY);

                // Add back to packet
                outputPacket.attemptMerge(newPrim);
            }
        }

        setOutputPacket(0, outputPacket);
    }
}

enzo::prm::Template GopCopyToPoints::parameterList[] = {enzo::prm::Terminator};
