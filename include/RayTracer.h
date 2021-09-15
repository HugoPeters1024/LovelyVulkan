#pragma once
#include "precomp.h"
#include "AppContext.h"
#include "AppExt.h"

namespace lv {

class RayTracer;

template<>
struct app_extensions<RayTracer> {
    std::vector<const char*> operator()() const { 
        return {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        }; 
    }
};

struct RayTracerFrame {
};

struct RayTracerInfo {
};


class RayTracer : public AppExt<RayTracerFrame> {
public:
    RayTracer(AppContext& ctx, RayTracerInfo info);
    ~RayTracer();


    RayTracerFrame* buildFrame(FrameContext& frame) override;
    void destroyFrame(RayTracerFrame* frame) override;

private:
    RayTracerInfo info;

    void createBottomLevelACStructure();
};


}

