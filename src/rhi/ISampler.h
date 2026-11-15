#pragma once

#include <optional>
#include <string>

#include "RHIFwd.h"
#include "RHITypes.h"

namespace rhi {

enum class Filter {
    Nearest, /**< Nearest texel / mip level. */
    Linear,  /**< Linear interpolation. */
};

/// What happens to texture coordinates outside [0, 1].
enum class AddressMode {
    Repeat,         /**< Tile the texture. */
    MirroredRepeat, /**< Tile, mirroring every other tile. */
    ClampToEdge,    /**< Use the edge texels. */
};

/// Describes a sampler to create with IDevice::createSampler. The defaults give trilinear, repeating sampling.
struct SamplerDesc {
    Filter min_filter = Filter::Linear;          /**< Filter when the texture is minified. */
    Filter mag_filter = Filter::Linear;          /**< Filter when the texture is magnified. */
    Filter mipmap_filter = Filter::Linear;       /**< Filter between mip levels. */
    AddressMode address_u = AddressMode::Repeat; /**< Addressing along U (x). */
    AddressMode address_v = AddressMode::Repeat; /**< Addressing along V (y). */
    AddressMode address_w = AddressMode::Repeat; /**< Addressing along W (z). */
    float min_lod = 0.0f;                        /**< Lowest mip level used. */
    float max_lod = 1000.0f;                     /**< Highest mip level used. */
    /// Anisotropic filtering level; > 1 enables it (needs DeviceFeatures::sampler_anisotropy).
    float max_anisotropy = 1.0f;
    std::optional<CompareOp> compare; /**< Set for depth-comparison (shadow map) samplers. */
    std::string label;
};

/// Sampling state (filtering, addressing) for reading textures in shaders. Bound through bind groups.
class ISampler {
public:
    virtual ~ISampler() = default;

    virtual SamplerDesc getDesc() const = 0;
};

} // namespace rhi
