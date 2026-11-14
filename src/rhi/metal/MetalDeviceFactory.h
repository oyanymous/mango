#pragma once

/**
 * @file
 * @brief rhi::metal::MetalDeviceFactory: the Metal backend's entry point.
 */

#include "IDeviceFactory.h"

/**
 * @brief The Metal implementation of the RHI.
 *
 * @details
 * Everything here is private to the RHI library; applications use the interfaces in namespace rhi.
 * Sources are Objective-C++ compiled with ARC. Headers that hold Metal objects are included only from `.mm`
 * files; headers included from C++ files (like this one) stay plain C++.
 */
namespace rhi::metal {

/**
 * @brief Creates Metal instances. Registered in RHIFactory for Backend::Metal.
 *
 * @note Being built step by step (docs/RHI_METAL_PLAN.md): createInstance() returns ErrorCode::Unsupported
 *       for now, and RHIFactory does not list Metal as available yet.
 */
class MetalDeviceFactory final : public IDeviceFactory {
public:
    Result<InstanceRef> createInstance(InstanceDesc desc) override;
};

} // namespace rhi::metal
