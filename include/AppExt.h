#pragma once
#include "precomp.h"

namespace lv {

class AppContext;
class FrameContext;
class IAppExt;

struct FrameExt : NoCopy {
public: 
    FrameExt() = default;
    virtual ~FrameExt() {}
};


class AppExt {
protected:
    AppContext& ctx;
    AppExt(AppContext& ctx) : ctx(ctx) { }
public:
    virtual ~AppExt() {}
    virtual void rebuild() { logger::error("Rebuilding logic is not implemented"); }

    virtual void embellishFrameContext(FrameContext& frame) {}
    virtual void cleanupFrameContext(FrameContext& frame) {}
};

}
