#include "RayTracer.h"

namespace lv {

RayTracer::RayTracer(AppContext& ctx, RayTracerInfo info) : AppExt<RayTracerFrame>(ctx), info(info) {
    createBottomLevelACStructure();
}

RayTracer::~RayTracer() {
}

RayTracerFrame* RayTracer::buildFrame(FrameContext& frame) {
    return new RayTracerFrame {};
}

void RayTracer::destroyFrame(RayTracerFrame* frame) {
}

void RayTracer::createBottomLevelACStructure() {
}



}
