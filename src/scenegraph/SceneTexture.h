#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace sg {

class Texture {
public:
    static std::shared_ptr<Texture> fromRgba8(std::uint32_t width, std::uint32_t height,
                                              std::vector<std::uint8_t> rgba);

    [[nodiscard]] std::uint32_t width() const { return m_width; }

    [[nodiscard]] std::uint32_t height() const { return m_height; }

    [[nodiscard]] const std::vector<std::uint8_t>& pixels() const { return m_pixels; }

    [[nodiscard]] std::uint64_t id() const { return m_id; }
private:
    Texture(std::uint32_t width, std::uint32_t height, std::vector<std::uint8_t> rgba);
    std::uint32_t m_width;
    std::uint32_t m_height;
    std::vector<std::uint8_t> m_pixels;
    std::uint64_t m_id;
};

} // namespace sg
