#pragma once
#include "precomp.h"
#include "Window.h"

namespace lv {

class AppContext;
class FrameContext;

class IAppExt {
public:
    virtual ~IAppExt() {}
    virtual void rebuild() = 0;

    virtual void* buildDowncastedFrame(FrameContext& frame) = 0;
    virtual void destroyDowncastedFrame(AppContext& ctx, void* frame) = 0;
    virtual std::type_index frameType() const = 0;

};

template<typename Frame>
class AppExt : public IAppExt {
protected:
    AppContext& ctx;
    AppExt(AppContext& ctx) : ctx(ctx) {}
public:
    virtual ~AppExt() {}
    void rebuild() override { logger::error("Rebuilding logic is not implemented"); }
    inline void* buildDowncastedFrame(FrameContext& frame) override { return buildFrame(frame); }
    inline void destroyDowncastedFrame(AppContext &ctx, void *frame) override {
      destroyFrame(ctx, reinterpret_cast<Frame *>(frame));
    };
    std::type_index frameType() const override { return typeid(Frame); }

    virtual Frame* buildFrame(FrameContext& frame) = 0;
    virtual void destroyFrame(AppContext& ctx, Frame* frame) = 0;

};


}
