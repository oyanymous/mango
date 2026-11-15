/**
 * @file
 * @brief sg::Texture.
 */

#include "SceneTexture.h"

#include <algorithm>
#include <atomic>

namespace sg {

namespace {

/// Source of Texture::id(); textures may be created on any thread.
std::atomic<std::uint64_t> g_next_texture_id{1};

} // namespace

std::shared_ptr<Texture> Texture::fromRgba8(std::uint32_t width, std::uint32_t height, std::vector<std::uint8_t> rgba) {
    return std::shared_ptr<Texture>(new Texture(std::max(width, 1u), std::max(height, 1u), std::move(rgba)));
}

Texture::Texture(std::uint32_t width, std::uint32_t height, std::vector<std::uint8_t> rgba)
    : m_width(width), m_height(height), m_pixels(std::move(rgba)), m_id(g_next_texture_id++) {
    m_pixels.resize(static_cast<std::size_t>(width) * height * 4, 0);
}

} // namespace sg
