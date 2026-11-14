#pragma once

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

// Objects are reference counted: whoever holds a Ref keeps the object alive ("retains" in the diagrams).
using InstanceRef = std::shared_ptr<IInstance>;
using AdapterRef = std::shared_ptr<IAdapter>;
using DeviceRef = std::shared_ptr<IDevice>;
using QueueRef = std::shared_ptr<IQueue>;
using SurfaceRef = std::shared_ptr<ISurface>;
using SwapchainRef = std::shared_ptr<ISwapchain>;
using BufferRef = std::shared_ptr<IBuffer>;
using TextureRef = std::shared_ptr<ITexture>;
using TextureViewRef = std::shared_ptr<ITextureView>;
using SamplerRef = std::shared_ptr<ISampler>;
using ShaderRef = std::shared_ptr<IShader>;
using PipelineLayoutRef = std::shared_ptr<IPipelineLayout>;
using PipelineRef = std::shared_ptr<IPipeline>;
using BindGroupLayoutRef = std::shared_ptr<IBindGroupLayout>;
using BindGroupRef = std::shared_ptr<IBindGroup>;
using RenderPassRef = std::shared_ptr<IRenderPass>;
using RenderTargetRef = std::shared_ptr<IRenderTarget>;
using CommandBufferRef = std::shared_ptr<ICommandBuffer>;
using FenceRef = std::shared_ptr<IFence>;
using SemaphoreRef = std::shared_ptr<ISemaphore>;
using DeviceFactoryRef = std::shared_ptr<IDeviceFactory>;

// Encoders are scoped to one recording pass of their command buffer, so they are owned uniquely.
using RenderEncoderRef = std::unique_ptr<IRenderEncoder>;
using ComputeEncoderRef = std::unique_ptr<IComputeEncoder>;
using CopyEncoderRef = std::unique_ptr<ICopyEncoder>;

using BackendList = std::vector<Backend>;
using AdapterList = std::vector<AdapterRef>;

} // namespace rhi
