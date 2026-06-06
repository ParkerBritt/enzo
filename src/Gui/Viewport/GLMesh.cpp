#include "Gui/Viewport/GLMesh.h"
#if defined(__linux__) || defined(_WIN32)
#include <GL/gl.h>
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#endif

#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/MeshUtils.h"
#include "Engine/Primitives/Mesh.h"
#include "icecream.hpp"
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <iostream>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

GLMesh::GLMesh()
{
    initializeOpenGLFunctions();
    init();
}

void GLMesh::init()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    initBuffers();

    // store data in the buffer
    // glBufferData(GL_ARRAY_BUFFER, vertexPosData.size()*sizeof(GLfloat), vertexPosData.data(),
    // GL_STATIC_DRAW); glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData.size()*sizeof(GLint),
    // indexData.data(), GL_STATIC_DRAW);

    // unbind vertex array
    glBindVertexArray(0);
}

void GLMesh::initBuffers()
{

    // create buffer of vertices
    glGenBuffers(1, &vertexBuffer);
    // set purpose
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    // gives the shader a way to read buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // read normal
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, normal)
    );
    // disable vertex attrib array
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &faceIndexBuffer);
    glGenBuffers(1, &lineIndexBuffer);
    glGenBuffers(1, &edgeIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceIndexBuffer);
}

void GLMesh::setPosBuffer(const enzo::NodePacket& packet)
{
    bind();
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    // compute total vertex count across all primitives
    size_t totalVerts = 0;
    for (size_t pi = 0; pi < packet.size(); ++pi)
    {
        auto prim = packet.getPrimitive(pi);
        if (prim->getType() != enzo::geo::PrimType::MESH) continue;
        totalVerts += std::static_pointer_cast<const enzo::geo::Mesh>(prim)->getNumVerts();
    }

    vertices.resize(totalVerts);

    size_t vertOffset = 0;
    for (size_t pi = 0; pi < packet.size(); ++pi)
    {
        auto prim = packet.getPrimitive(pi);
        if (prim->getType() != enzo::geo::PrimType::MESH) continue;
        auto geometry = std::static_pointer_cast<const enzo::geo::Mesh>(prim);
        const size_t numFaces = geometry->getNumFaces();
        geometry->computeFaceStartVertices();

        const size_t localVertOffset = vertOffset;
        auto faceNormals = geometry->getFaceNormal(true);

        // Storage views resolve each vertex position with two direct loads
        const std::span<const enzo::intT> vertexPoints = geometry->vertexPointSpan();
        const std::span<const enzo::Vector3> pointPositions = geometry->pointPosSpan();

        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, numFaces),
            [&](tbb::blocked_range<size_t> range) {
                for (int faceOffset = range.begin(); faceOffset < range.end(); ++faceOffset)
                {
                    const enzo::Offset faceStartVert = geometry->getFaceStartVertex(faceOffset);
                    const unsigned int faceVertCnt = geometry->getFaceVertCount(faceOffset);

                    enzo::Vector3 Normal(0, 0, 0);
                    if (faceVertCnt >= 3) Normal = faceNormals[faceOffset];

                    for (int i = 0; i < faceVertCnt; ++i)
                    {
                        const unsigned int vertexCount = faceStartVert + i;
                        const enzo::Vector3& p = pointPositions[vertexPoints[vertexCount]];

                        vertices[localVertOffset + vertexCount] = {
                            {p.x(), p.y(), p.z()},
                            {Normal.x(), Normal.y(), Normal.z()}
                        };
                    }
                }
            }
        );

        vertOffset += geometry->getNumVerts();
    }

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );
    unbind();
}

void GLMesh::setIndexBuffer(const enzo::NodePacket& packet)
{
    bind();
    faceIndexData.clear();
    lineIndexData.clear();
    edgeIndexData.clear();

    size_t vertOffset = 0;
    for (size_t pi = 0; pi < packet.size(); ++pi)
    {
        auto prim = packet.getPrimitive(pi);
        if (prim->getType() != enzo::geo::PrimType::MESH) continue;
        auto geometry = std::static_pointer_cast<const enzo::geo::Mesh>(prim);

        // Open faces draw as polylines, closed faces fill and get a wireframe outline.
        std::vector<enzo::Offset> fillFaceOffsets;
        for (enzo::Offset faceOffset = 0; faceOffset < geometry->getNumFaces(); ++faceOffset)
        {
            int faceVertexCount = geometry->getFaceVertCount(faceOffset);
            const enzo::Offset startVert = vertOffset + geometry->getFaceStartVertex(faceOffset);
            const enzo::boolT closed = geometry->isClosed(faceOffset);

            if (!closed && faceVertexCount >= 2)
            {
                for (size_t i = 0; i < faceVertexCount - 1; ++i)
                {
                    lineIndexData.push_back(startVert + i);
                    lineIndexData.push_back(startVert + i + 1);
                }
            }
            else if (faceVertexCount >= 3)
            {
                fillFaceOffsets.push_back(faceOffset);

                // polygon perimeter edges, for wireframe overlay
                for (size_t i = 0; i < faceVertexCount; ++i)
                {
                    edgeIndexData.push_back(startVert + i);
                    edgeIndexData.push_back(startVert + (i + 1) % faceVertexCount);
                }
            }
        }

        // Ear clip the fillable faces so concave boolean output tessellates right.
        // The util returns mesh local offsets, so shift them into this prim's block.
        for (const std::array<enzo::Offset, 3>& tri :
             enzo::utils::earClipTriangleIndices(*geometry, fillFaceOffsets))
        {
            faceIndexData.push_back(vertOffset + tri[0]);
            faceIndexData.push_back(vertOffset + tri[1]);
            faceIndexData.push_back(vertOffset + tri[2]);
        }

        vertOffset += geometry->getNumVerts();
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceIndexBuffer);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        faceIndexData.size() * sizeof(GLint),
        faceIndexData.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineIndexBuffer);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        lineIndexData.size() * sizeof(GLint),
        lineIndexData.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeIndexBuffer);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        edgeIndexData.size() * sizeof(GLint),
        edgeIndexData.data(),
        GL_STATIC_DRAW
    );
    unbind();
}

void GLMesh::bind() { glBindVertexArray(vao); }

void GLMesh::unbind() { glBindVertexArray(0); }

void GLMesh::draw()
{
    bind();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceIndexBuffer);
    glDrawElements(GL_TRIANGLES, faceIndexData.size(), GL_UNSIGNED_INT, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineIndexBuffer);
    glDrawElements(GL_LINES, lineIndexData.size(), GL_UNSIGNED_INT, 0);
}

void GLMesh::drawWireframe()
{
    bind();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeIndexBuffer);
    glDrawElements(GL_LINES, edgeIndexData.size(), GL_UNSIGNED_INT, 0);
}
