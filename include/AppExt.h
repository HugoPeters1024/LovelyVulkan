#pragma once
#include "precomp.h"
#include "Window.h"

namespace lv {

class AppContext;
class FrameContext;

class IAppExt {
private:
public:
    virtual void buildCore(AppContext& ctx) = 0;
    virtual void destroyCore(AppContext& ctx) = 0;
    virtual void rebuild() = 0;

    virtual void* buildDowncastedFrame(FrameContext& frame) = 0;
    virtual std::type_index frameType() const = 0;
};

template<typename Frame>
class AppExt : public IAppExt {
public:
    virtual Frame* buildFrame(FrameContext& frame) = 0;
    std::type_index frameType() const override { return typeid(Frame); }
    AppExt() {}

    inline void* buildDowncastedFrame(FrameContext& frame) override { return buildFrame(frame); }
    void rebuild() override {
        logger::error("Rebuilding logic is not implemented");
    }
};


}
