#pragma once

#include <cstdint>
#include <map>
#include <optional>

namespace sg {

class RangeAllocator {
public:
    explicit RangeAllocator(std::uint32_t capacity = 0) { grow(capacity); }

    std::optional<std::uint32_t> allocate(std::uint32_t size);

    void free(std::uint32_t offset, std::uint32_t size);

    void grow(std::uint32_t capacity);

    [[nodiscard]] std::uint32_t capacity() const { return m_capacity; }

    [[nodiscard]] std::uint32_t used() const { return m_used; }
private:
    std::map<std::uint32_t, std::uint32_t> m_free;
    std::uint32_t m_capacity = 0;
    std::uint32_t m_used = 0;
};

} // namespace sg
