#include "FrameManager.h"

namespace lv {

FrameManager::FrameManager(AppContext& ctx) : AppExt(ctx) {
}

FrameManager::~FrameManager() {
    for(auto& frame : frameContexts) {
        for(auto& ext : extensions) {
            ext->cleanupFrameContext(frame);
        }
    }

    for(auto& fence : inFlightFences) {
        vkDestroyFence(ctx.vkDevice, fence, nullptr);
    }
}

void FrameManager::init(const std::vector<AppExt*>& extensions) {
    assert(frameContexts.empty() && "Already initialized");
    this->extensions = extensions;
    logger::debug("Frame Manager initializing with {} frames and {} extensions", nrFrames, extensions.size());

    // Create the frame contexts
    for(uint32_t i = 0; i<nrFrames; i++) {
        FrameContext frame(ctx);
        frame.idx = i;

        // Populate the command buffer
        auto allocInfo = vks::initializers::commandBufferAllocateInfo(ctx.vkCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
        vkCheck(vkAllocateCommandBuffers(ctx.vkDevice, &allocInfo, &frame.cmdBuffer));

        frame.frameFinished = VK_NULL_HANDLE;

        // Allow derivations to extend the context
        embellishFrameContext(frame);

        // Let the extensions expand the frames
        for(const auto& ext : extensions) {
            ext->embellishFrameContext(frame);
        }

        frameContexts.push_back(frame);
    }

    // Create the in flight fences
    auto fenceInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    inFlightFences.resize(nrFramesInFlight);
    for(auto& fence : inFlightFences) {
        vkCheck(vkCreateFence(ctx.vkDevice, &fenceInfo, nullptr, &fence));
    }
}

void FrameManager::submitFrame(FrameContext& frame) {
    auto submitInfo = vks::initializers::submitInfo(&frame.cmdBuffer);
    vkCheck(vkQueueSubmit(ctx.queues.graphics, 1, &submitInfo, frame.frameFinished));

}

void FrameManager::nextFrame(const std::function<void(FrameContext&)>& callback) {
    // make sure the flight spot is free and reset
    vkCheck(vkWaitForFences(ctx.vkDevice, 1, &inFlightFences[currentInFlight], VK_TRUE, UINT64_MAX));

    // progress to the next frame
    frameIdx = acquireNextFrameIdx();
    auto& frame = getCurrentFrame();

    // Make sure that this frame is also finished
    if (frame.frameFinished != VK_NULL_HANDLE) {
        vkCheck(vkWaitForFences(ctx.vkDevice, 1, &frame.frameFinished, VK_TRUE, UINT64_MAX));
    }
    vkCheck(vkResetFences(ctx.vkDevice, 1, &inFlightFences[currentInFlight]));

    // mark the frame in flight
    frame.frameFinished = inFlightFences[currentInFlight];

    vkCheck(vkResetCommandBuffer(frame.cmdBuffer, 0));
    auto beginInfo = vks::initializers::commandBufferBeginInfo();
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkCheck(vkBeginCommandBuffer(frame.cmdBuffer, &beginInfo));
    callback(frame);
    vkEndCommandBuffer(frame.cmdBuffer);

    submitFrame(frame);
    currentInFlight = (currentInFlight + 1) % nrFramesInFlight;
}

}
