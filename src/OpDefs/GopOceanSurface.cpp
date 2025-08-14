#include "OpDefs/GopOceanSurface.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <cmath>
#include <cstdio>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>

GopOceanSurface::GopOceanSurface(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
: GeometryOpDef(network, opInfo)
{

}

void GopOceanSurface::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if(outputRequested(0))
    {
        geo::Geometry geo = context.cloneInputGeo(0);
        
        const ga::Offset pointCount = geo.getNumPoints();

        tbb::parallel_for(tbb::blocked_range<ga::Offset>(0, pointCount), [this, &geo](tbb::blocked_range<ga::Offset> range){
            for(ga::Offset i=range.begin(); i!=range.end(); ++i)
            {
                bt::Vector3 pos = geo.getPointPos(i);

                pos = getSurfacePos(pos);

                geo.setPointPos(i, pos);
            }
        });

        setOutputGeometry(0, geo);
    }

}

enzo::bt::Vector3 GopOceanSurface::getSurfacePos(const enzo::bt::Vector3 pos)
{
    using enzo::bt::Vector3;

    const int waveNum = 30;
    const int waveDirNum = 20;

    const float speed = 1.0f;
    const float frame = 0.0f;
    const float t = speed * frame;

    float x = 0.0f;
    float y = 0.0f;

    float baseWaveLength = 2.0f;
    float baseAmp = 0.005f;

    const float offsetForEachDir = 20.8f;

    float offset = 17.2f;

    Vector3 outDisplacement(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < waveDirNum; ++i) {
        float angle = (float(i) / float(waveDirNum)) * 2.0f * float(M_PI);
        Vector3 dir(std::cos(angle), 0.0f, std::sin(angle));

        float dirOffset = 20.0f;
        float offset = (float(i) * dirOffset) * 0.1f + offsetForEachDir;

        float a = pos.dot(dir);

        for (int j = 0; j < waveNum; ++j) {
            float waveLength = baseWaveLength / (float(j) + 1.0f);
            float amplitude = baseAmp / (float(j) + 1.0f);
            offset += (float(j) + 826.0f) * 0.001f;

            float k = 2.0f * float(M_PI) / waveLength;
            float theta = k * a + t + offset;

            x = amplitude * std::sin(theta);
            y -= amplitude * std::cos(theta);
            outDisplacement += Vector3(x * dir.x(), 0.0f, x * dir.z());
        }
    }

    // big waves
    for (int i = 0; i < 5; ++i)
    {
        float bigWaveAngle = 0.75f * 2.0f * float(M_PI);
        float bigWaveStretch = 0.01f;
        Vector3 bigWaveDir(std::cos(bigWaveAngle) * bigWaveStretch, 1.0f, std::sin(bigWaveAngle) * bigWaveStretch);

        int bigWaveNum = 3;
        Vector3 bigWaveMaskPos = pos;
        bigWaveMaskPos += Vector3(100.0f * float(238 + i), 100.0f * float(238 + i), 100.0f * float(238 + i));
        // change frequency of noise
        bigWaveMaskPos = Vector3(bigWaveMaskPos.x() * 0.03f, bigWaveMaskPos.y() * 0.03f, bigWaveMaskPos.z() * 0.03f);
        float bigWaveMask = 1.0f;
        // bigWaveMask = 1;

        float angleVariation = 0.1f;
        float angle = (0.5f + float(i) * angleVariation) * 2.0f * float(M_PI);
        Vector3 dir(std::cos(angle), 0.0f, std::sin(angle));

        float dirOffset = 20.0f;
        offset = 6072.0f * 0.01f * dirOffset;

        float a = pos.dot(dir);

        baseWaveLength = 30.0f;
        baseAmp = 0.2f;

        for (int j = 0; j < bigWaveNum; ++j) {
            float waveLength = baseWaveLength / (float(j) + 1.0f);
            float amplitude = baseAmp / (float(j) + 1.0f);
            offset += (float(j) + 826.0f) * 0.005f;

            float k = 2.0f * float(M_PI) / waveLength;
            float theta = k * a + t + offset;

            x = amplitude * std::sin(theta);
            y -= amplitude * std::cos(theta) * bigWaveMask;
            outDisplacement += Vector3(x * dir.x(), 0.0f, x * dir.z());
        }
    }
    outDisplacement = Vector3(outDisplacement.x(), y, outDisplacement.z());

    return Vector3(pos.x() + outDisplacement.x(),
                   pos.y() + outDisplacement.y(),
                   pos.z() + outDisplacement.z());
}


enzo::prm::Template GopOceanSurface::parameterList[] =
{
    enzo::prm::Terminator
};
