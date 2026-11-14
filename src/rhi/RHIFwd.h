#pragma once

/// @file
/// Forward declarations of every RHI interface and the `*Ref` handle types.
///
/// Include this instead of the full interface headers when a header only stores or passes RHI objects;
/// it keeps builds fast and avoids include cycles.

#include <memory>
#include <vector>

#include "Backend.h"

namespace rhi {

class IInstance;
class IAdapter;
class IDevice;
class IQueue;
class ISurface;
class ISwapchain;
class IBuffer;
class ITexture;
class ITextureView;
class ISampler;
class IShader;
class IPipelineLayout;
class IPipeline;
class IBindGroupLayout;
class IBindGroup;
class IRenderPass;
class IRenderTarget;
class ICommandBuffer;
class IRenderEncoder;
class IComputeEncoder;
class ICopyEncoder;
class IFence;
class ISemaphore;
class IDeviceFactory;

/// @name Reference-counted handles
/// Whoever holds a Ref keeps the object alive ("retains" in the design diagrams). Objects that depend on
/// others retain them too, e.g. a texture view retains its texture.
/// @{
using InstanceRef = std::shared_ptr<IInstance>;               ///< Handle to an IInstance.
using AdapterRef = std::shared_ptr<IAdapter>;                 ///< Handle to an IAdapter.
using DeviceRef = std::shared_ptr<IDevice>;                   ///< Handle to an IDevice.
using QueueRef = std::shared_ptr<IQueue>;                     ///< Handle to an IQueue.
using SurfaceRef = std::shared_ptr<ISurface>;                 ///< Handle to an ISurface.
using SwapchainRef = std::shared_ptr<ISwapchain>;             ///< Handle to an ISwapchain.
using BufferRef = std::shared_ptr<IBuffer>;                   ///< Handle to an IBuffer.
using TextureRef = std::shared_ptr<ITexture>;                 ///< Handle to an ITexture.
using TextureViewRef = std::shared_ptr<ITextureView>;         ///< Handle to an ITextureView.
using SamplerRef = std::shared_ptr<ISampler>;                 ///< Handle to an ISampler.
using ShaderRef = std::shared_ptr<IShader>;                   ///< Handle to an IShader.
using PipelineLayoutRef = std::shared_ptr<IPipelineLayout>;   ///< Handle to an IPipelineLayout.
using PipelineRef = std::shared_ptr<IPipeline>;               ///< Handle to an IPipeline.
using BindGroupLayoutRef = std::shared_ptr<IBindGroupLayout>; ///< Handle to an IBindGroupLayout.
using BindGroupRef = std::shared_ptr<IBindGroup>;             ///< Handle to an IBindGroup.
using RenderPassRef = std::shared_ptr<IRenderPass>;           ///< Handle to an IRenderPass.
using RenderTargetRef = std::shared_ptr<IRenderTarget>;       ///< Handle to an IRenderTarget.
using CommandBufferRef = std::shared_ptr<ICommandBuffer>;     ///< Handle to an ICommandBuffer.
using FenceRef = std::shared_ptr<IFence>;                     ///< Handle to an IFence.
using SemaphoreRef = std::shared_ptr<ISemaphore>;             ///< Handle to an ISemaphore.
using DeviceFactoryRef = std::shared_ptr<IDeviceFactory>;     ///< Handle to an IDeviceFactory.
/// @}

/// @name Encoder handles
/// Encoders are scoped to one recording pass of their command buffer, so they are owned uniquely.
/// @{
using RenderEncoderRef = std::unique_ptr<IRenderEncoder>;   ///< Owning pointer to an IRenderEncoder.
using ComputeEncoderRef = std::unique_ptr<IComputeEncoder>; ///< Owning pointer to an IComputeEncoder.
using CopyEncoderRef = std::unique_ptr<ICopyEncoder>;       ///< Owning pointer to an ICopyEncoder.
/// @}

using BackendList = std::vector<Backend>;    ///< A list of backends.
using AdapterList = std::vector<AdapterRef>; ///< A list of adapters.

} // namespace rhi
