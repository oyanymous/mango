#include "MetalShader.h"

#include <exception>
#include <string>
#include <utility>

#include <spirv_msl.hpp>

#include "MetalPipeline.h"

namespace rhi::metal {

namespace {

constexpr std::uint32_t kSpirvMagic = 0x07230203;

std::optional<spv::ExecutionModel> executionModel(ShaderStage stage) {
    switch (stage) {
    case ShaderStage::Vertex:
        return spv::ExecutionModelVertex;
    case ShaderStage::Fragment:
        return spv::ExecutionModelFragment;
    case ShaderStage::Compute:
        return spv::ExecutionModelGLCompute;
    default:
        return std::nullopt;
    }
}

} // namespace

Result<ShaderRef> MetalShader::create(std::shared_ptr<MetalDeviceContext> context, ShaderDesc desc) {
    if (desc.spirv.empty() || desc.spirv[0] != kSpirvMagic) {
        return Error{ErrorCode::InvalidArgument, "createShader: spirv is empty or not SPIR-V"};
    }
    const std::optional<spv::ExecutionModel> model = executionModel(desc.stage);
    if (!model) {
        return Error{ErrorCode::InvalidArgument,
                     "createShader: stage must be exactly one of Vertex, Fragment, Compute"};
    }

    try {
        spirv_cross::CompilerMSL compiler(desc.spirv);
        bool found = false;
        for (const spirv_cross::EntryPoint& entry : compiler.get_entry_points_and_stages()) {
            found = found || (entry.name == desc.entry_point && entry.execution_model == *model);
        }
        if (!found) {
            return Error{ErrorCode::InvalidArgument,
                         "createShader: no entry point '" + desc.entry_point + "' for the shader's stage"};
        }
    } catch (const std::exception& error) {
        return Error{ErrorCode::InvalidArgument, std::string("createShader: malformed SPIR-V: ") + error.what()};
    }
    return ShaderRef(std::make_shared<MetalShader>(std::move(context), std::move(desc)));
}

MetalShader::MetalShader(std::shared_ptr<MetalDeviceContext> context, ShaderDesc desc)
    : m_desc(std::move(desc)), m_context(std::move(context)) {
}

ShaderReflection MetalShader::getReflection() const {
    return {.stage = m_desc.stage, .entry_point = m_desc.entry_point};
}

Result<MetalFunction> MetalShader::compile(const MetalPipelineLayout& layout) const {
    const spv::ExecutionModel model = *executionModel(m_desc.stage);
    MetalFunction result;
    std::string source;
    std::string entry_name;
    try {
        spirv_cross::CompilerMSL compiler(m_desc.spirv);
        compiler.set_entry_point(m_desc.entry_point, model);

        spirv_cross::CompilerMSL::Options options;
        options.platform = spirv_cross::CompilerMSL::Options::macOS;
        options.set_msl_version(3, 0);
        compiler.set_msl_options(options);
        spirv_cross::CompilerGLSL::Options common = compiler.get_common_options();

        common.vertex.flip_vert_y = model == spv::ExecutionModelVertex;
        compiler.set_common_options(common);

        for (std::size_t set = 0; set < layout.sets().size(); ++set) {
            for (const MetalBindingSlot& slot : layout.sets()[set]) {
                spirv_cross::MSLResourceBinding binding;
                binding.stage = model;
                binding.desc_set = static_cast<std::uint32_t>(set);
                binding.binding = slot.binding;
                binding.count = slot.count;
                switch (slot.type) {
                case BindingType::UniformBuffer:
                case BindingType::StorageBuffer:
                    binding.msl_buffer = slot.index;
                    break;
                case BindingType::SampledTexture:
                case BindingType::StorageTexture:
                    binding.msl_texture = slot.index;
                    break;
                case BindingType::Sampler:
                    binding.msl_sampler = slot.index;
                    break;
                }
                compiler.add_msl_resource_binding(binding);
            }
        }

        source = compiler.compile();
        entry_name = compiler.get_cleansed_entry_point_name(m_desc.entry_point, model);
        if (model == spv::ExecutionModelGLCompute) {
            for (std::uint32_t axis = 0; axis < 3; ++axis) {
                result.workgroup_size[axis] =
                    std::max(1u, compiler.get_execution_mode_argument(spv::ExecutionModeLocalSize, axis));
            }
        }
    } catch (const std::exception& error) {
        return Error{ErrorCode::InitializationFailed, std::string("SPIRV-Cross failed: ") + error.what()};
    }

    MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
    options.languageVersion = MTLLanguageVersion3_0;
    NSError* error = nil;
    id<MTLLibrary> library = [m_context->device newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()]
                                                             options:options
                                                               error:&error];
    if (library == nil) {
        return Error{ErrorCode::InitializationFailed,
                     std::string("Metal shader compilation failed: ") + error.localizedDescription.UTF8String};
    }
    library.label = toNSString(m_desc.label);
    result.function = [library newFunctionWithName:[NSString stringWithUTF8String:entry_name.c_str()]];
    if (result.function == nil) {
        return Error{ErrorCode::InitializationFailed, "Metal shader compilation lost entry point " + entry_name};
    }
    return result;
}

} // namespace rhi::metal
