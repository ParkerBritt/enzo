#include "Gui/Viewport/GLMesh.h"
#include <GL/gl.h>
#include <iostream>
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Operator/Geometry.h"
#include "Engine/Types.h"
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include "icecream.hpp"



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
    // glBufferData(GL_ARRAY_BUFFER, vertexPosData.size()*sizeof(GLfloat), vertexPosData.data(), GL_STATIC_DRAW);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData.size()*sizeof(GLint), indexData.data(), GL_STATIC_DRAW);


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
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // disable vertex attrib array
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &faceIndexBuffer);
    glGenBuffers(1, &lineIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceIndexBuffer);

}

void GLMesh::setPosBuffer(enzo::geo::Geometry& geometry)
{
    bind();
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    const size_t numPrims = geometry.getNumPrims();

    vertices.resize(geometry.getNumVerts());
    geometry.computePrimStartVertices();

    tbb::parallel_for(tbb::blocked_range<size_t>(0, numPrims), [&](tbb::blocked_range<size_t> range)
    {
        for (int primOffset=range.begin(); primOffset<range.end(); ++primOffset)
        {
            const enzo::ga::Offset primStartVert = geometry.getPrimStartVertex(primOffset);
            const unsigned int faceVertCnt = geometry.getPrimVertCount(primOffset);

            enzo::bt::Vector3 Normal;

            // compute normal
            if(faceVertCnt>=3)
            {
                const unsigned v1 = primStartVert;
                const unsigned v2 = primStartVert + 1;
                const unsigned v3 = primStartVert + 2;

                const enzo::bt::Vector3 pos1 = geometry.getPosFromVert(v1);
                const enzo::bt::Vector3 pos2 = geometry.getPosFromVert(v2);
                const enzo::bt::Vector3 pos3 = geometry.getPosFromVert(v3);

                enzo::bt::Vector3 tang1 = (pos2-pos1);
                enzo::bt::Vector3 tang2 = (pos3-pos1);

                tang1.normalize();
                tang2.normalize();

                Normal = tang1.cross(tang2);
            }

            for(int i=0; i< faceVertCnt; ++i)
            {
                const unsigned int vertexCount = primStartVert+i;
                enzo::bt::Vector3 p = geometry.getPosFromVert(vertexCount);

                vertices[vertexCount] ={
                    { p.x(),
                      p.y(),
                      p.z()},
                    { Normal.x(),
                      Normal.y(),
                      Normal.z()}
                };
            }

        }
    });

    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    unbind();
}

void GLMesh::setIndexBuffer(enzo::geo::Geometry& geometry)
{
    bind();
    faceIndexData.clear();
    lineIndexData.clear();


    // create triangle fan from potentially ngon inputs
    for(enzo::ga::Offset primOffset=0; primOffset<geometry.getNumPrims(); ++primOffset)
    {
        int primVertexCount = geometry.getPrimVertCount(primOffset);
        const enzo::ga::Offset startVert = geometry.getPrimStartVertex(primOffset);
        const enzo::bt::boolT closed = geometry.isClosed(primOffset);

        if(!closed && primVertexCount>=2)
        {
            for(size_t i=0; i<primVertexCount-1; ++i)
            {
                lineIndexData.push_back(startVert+i);
                lineIndexData.push_back(startVert+i+1);
            }
        }
        else if(primVertexCount>=3)
        {
            for(size_t i=1; i<primVertexCount-1; ++i)
            {
                faceIndexData.push_back(startVert);
                faceIndexData.push_back(startVert+i);
                faceIndexData.push_back(startVert+i+1);

            }
        }

    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, faceIndexData.size()*sizeof(GLint), faceIndexData.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIndexData.size()*sizeof(GLint), lineIndexData.data(), GL_STATIC_DRAW);
    unbind();
}


void GLMesh::bind()
{
    glBindVertexArray(vao);
}

void GLMesh::unbind()
{
    glBindVertexArray(0);
}

void GLMesh::draw()
{
    bind();

    // wireframe
    // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceIndexBuffer);
    glDrawElements(GL_TRIANGLES, faceIndexData.size(), GL_UNSIGNED_INT, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineIndexBuffer);
    glDrawElements(GL_LINES, lineIndexData.size(), GL_UNSIGNED_INT, 0);
}
