#pragma once

/**
 * @file
 * @brief The one place that configures and includes Vulkan-Hpp (`vulkan/vulkan_raii.hpp`).
 *
 * Every Vulkan backend translation unit gets this header first (it is the RHIVulkan precompiled header), so all of
 * them see the same Vulkan-Hpp configuration. Include it instead of `vulkan.hpp` or `vulkan_raii.hpp`.
 *
 * - No exceptions: RAII create functions return `vk::ResultValue<T>`, which VulkanCommon maps to rhi::Result.
 *   Failures are expected results (e.g. an out-of-date swapchain), so Vulkan-Hpp must not assert on them either.
 * - Only `vk::raii` objects are used, and each carries its own dispatch table, so there is no default dispatcher.
 * - Struct constructors are off so Vulkan structs are aggregates and take designated initializers.
 * - Unused parts (unique handles, `vk::to_string`, `operator<=>`) are left out to keep compile times down.
 */

#define VULKAN_HPP_NO_EXCEPTIONS
// Vulkan-Hpp otherwise asserts on every non-success result; the caller checks the returned result instead.
#define VULKAN_HPP_ASSERT_ON_RESULT(expression)
#define VULKAN_HPP_NO_DEFAULT_DISPATCHER
#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_SMART_HANDLE
#define VULKAN_HPP_NO_TO_STRING
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR

#include <vulkan/vulkan_raii.hpp>
