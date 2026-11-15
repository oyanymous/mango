#pragma once

#include <memory>

#include "ISurface.h"
#include "Result.h"
#include "VulkanCommon.h"
#include "VulkanInstance.h"

namespace rhi::vulkan {

class VulkanSurface final : public ISurface {
public:
    static Result<SurfaceRef> create(std::shared_ptr<VulkanInstanceHandle> instance, SurfaceDesc desc);

    VulkanSurface(std::shared_ptr<VulkanInstanceHandle> instance, SurfaceDesc desc, vk::raii::SurfaceKHR surface);

    VulkanSurface(const VulkanSurface&) = delete;
    VulkanSurface& operator=(const VulkanSurface&) = delete;

    SurfaceDesc getDesc() const override;

    vk::SurfaceKHR handle() const { return *m_surface; }
private:
    std::shared_ptr<VulkanInstanceHandle> m_instance;
    SurfaceDesc m_desc;
    vk::raii::SurfaceKHR m_surface;
};

} // namespace rhi::vulkan
