#include "precomp.h"
#include "AppContext.h"
#include "AppExt.h"

namespace lv {

class FrameContext {
public:
    AppContext& ctx;
    uint32_t idx;
    VkCommandBuffer cmdBuffer;
    VkFence frameFinished;

    FrameContext(AppContext& ctx) : ctx(ctx) {}

    template<typename T, typename... Args>
    T& registerExtFrame(Args&&... args) {
        static_assert(std::is_base_of<FrameExt, T>::value, "Frame extensions must be derived from FrameExt");
        assert(extensionFrames.find(typeid(T)) == extensionFrames.end() && "Frame of that type already registered");
        extensionFrames.insert({typeid(T), new T(std::forward<Args>(args)...)});
        return *reinterpret_cast<T*>(extensionFrames[typeid(T)]);
    }

    template<typename T>
    T& getExtFrame() {
        static_assert(std::is_base_of<FrameExt, T>::value, "Frame extensions must be derived from FrameExt");
        return *reinterpret_cast<T*>(getExtFrame(typeid(T)));
    }

    void* getExtFrame(std::type_index typeName) {
        assert(extensionFrames.find(typeName) != extensionFrames.end() && "No extension with this type registered");
        return extensionFrames.at(typeName);
    }



private:
    std::unordered_map<std::type_index, FrameExt*> extensionFrames;
};

class FrameManager : public AppExt {
public:
    FrameManager(AppContext& ctx);
    ~FrameManager() override;

    void init(const std::vector<AppExt*>& extensions);
    void nextFrame(const std::function<void(FrameContext&)>& callback);
protected:
    FrameContext& getCurrentFrame() { return frameContexts[frameIdx]; }

    void setNrFrames(uint32_t nrFrames) { this->nrFrames = nrFrames; }
    void setFrameIdx(uint32_t frameIdx) { assert(frameIdx < frameContexts.size()); this->frameIdx = frameIdx; }
    virtual uint32_t acquireNextFrameIdx() { return (frameIdx + 1) % nrFrames; }
    virtual void submitFrame(FrameContext& frame);

    std::vector<FrameContext> frameContexts;
    uint32_t nrFramesInFlight = 1;
    uint32_t nrFrames = 1;
    uint32_t frameIdx = 0;
    uint32_t currentInFlight = 0;
    std::vector<VkFence> inFlightFences;

private:
    std::vector<AppExt*> extensions;

};

template<typename T>
using FrameSelector = std::function<T(FrameContext&)>;

}
