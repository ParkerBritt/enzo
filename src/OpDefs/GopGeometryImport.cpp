#include "OpDefs/GopGeometryImport.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Types.h"
#include <cstdio>
#include <oneapi/tbb/parallel_for.h>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>

GopGeometryImport::GopGeometryImport(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
: GeometryOpDef(network, opInfo)
{

}

void GopGeometryImport::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if(outputRequested(0))
    {
        geo::Geometry geo;

        bt::String filePath = context.evalStringParm("filePath");
        boost::trim(filePath);
        if(filePath.size()<4)
        {
            throwError("file path too small");
            setOutputGeometry(0, geo);
            return;
        }

        std::string fileExt = filePath.substr(filePath.size()-4, filePath.size());

        if(fileExt!=".obj")
        {
            throwError("File path not accepted: " + fileExt);
            setOutputGeometry(0, geo);
            return;
        }


        // auto PAttr = geo.getAttribByName(ga::AttrOwner::POINT, "P");
        // ga::AttributeHandleVector3 PAttrHandle(PAttr);

        // auto pointAttr = geo.getAttribByName(ga::AttrOwner::VERTEX, "point");
        // ga::AttributeHandleInt pointAttrHandle(pointAttr);

        // auto vertexCountAttr = geo.getAttribByName(ga::AttrOwner::PRIMITIVE, "vertexCount");
        // ga::AttributeHandleInt vertexCountHandle(vertexCountAttr);

        std::ifstream file(filePath);
        if(!file.is_open())
        {
            std::cerr << "Failed to open file " << filePath << "\n";
            setOutputGeometry(0, geo);
            return;
        }

        std::string line;
        while(std::getline(file, line))
        {
            if(line.size()==0)
            {
                continue;
            }

            char firstChar = line[0];
            if(firstChar=='v')
            {
                std::vector<std::string> result;
                boost::split(result, line, isspace);
                if(result.size()!=4)
                {
                    continue;

                }
                const bt::Vector3 pointPos = {std::stod(result[1]), std::stod(result[2]), std::stod(result[3])};
                geo.addPoint(pointPos);
            }
            else if(firstChar=='f' || firstChar=='l')
            {
                bool closedFace = firstChar=='f';

                std::vector<std::string> result;
                boost::split(result, line, isspace);
                if(result.size()<3)
                {
                    continue;

                }

                ga::Offset numVerts = result.size()-1;

                std::vector<ga::Offset> verts;
                verts.reserve(numVerts);

                // set vertex attributes
                for(int i=1; i<numVerts+1; ++i)
                {
                    verts.push_back(std::stoi(result[i])-1);
                }

                geo.addFace(verts, closedFace);

            }
        }
        


        // scale
        const float scale = context.evalFloatParm("size");
        const ga::Offset numPoints = geo.getNumPoints();
        for(ga::Offset i=0; i<numPoints; ++i)
        {
            enzo::bt::Vector3 pointPos = geo.getPointPos(i);
            pointPos*=scale;
            geo.setPointPos(i, pointPos);
        }
        // ----

        // set output geometry
        setOutputGeometry(0, geo);
    }

}

enzo::prm::Template GopGeometryImport::parameterList[] =
{
    enzo::prm::Template(enzo::prm::Type::STRING, enzo::prm::Name("filePath", "File Path")),
    enzo::prm::Template(enzo::prm::Type::FLOAT, enzo::prm::Name("size", "Size"), enzo::prm::Default(1)),
    enzo::prm::Terminator
};
