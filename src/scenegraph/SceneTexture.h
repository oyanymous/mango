#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace sg {

/**
 * An immutable RGBA8 image, shared by the nodes that draw it.
 *
 * The pixels stay on the CPU until a renderer first draws the texture; each renderer then keeps its own GPU copy
 * until the last `std::shared_ptr` to the texture is gone. Pixels use straight alpha; the renderer premultiplies them.
 */
class Texture {
public:
    /**
     * Creates a texture from tightly packed RGBA8 rows, top row first.
     *
     * @param width Width in pixels; must be > 0.
     * @param height Height in pixels; must be > 0.
     * @param rgba `width * height * 4` bytes; missing bytes read as transparent black.
     */
    static std::shared_ptr<Texture> fromRgba8(std::uint32_t width, std::uint32_t height,
                                              std::vector<std::uint8_t> rgba);

    /// Width in pixels.
    [[nodiscard]] std::uint32_t width() const { return m_width; }
    /// Height in pixels.
    [[nodiscard]] std::uint32_t height() const { return m_height; }
    /// The pixels, `width() * height() * 4` bytes.
    [[nodiscard]] const std::vector<std::uint8_t>& pixels() const { return m_pixels; }
    /// An identifier unique for the life of the process (renderers key their GPU copies by it).
    [[nodiscard]] std::uint64_t id() const { return m_id; }
private:
    Texture(std::uint32_t width, std::uint32_t height, std::vector<std::uint8_t> rgba);
    std::uint32_t m_width;
    std::uint32_t m_height;
    std::vector<std::uint8_t> m_pixels;
    std::uint64_t m_id;
};

} // namespace sg
