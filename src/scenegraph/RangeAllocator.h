#pragma once

#include <cstdint>
#include <map>
#include <optional>

namespace sg {

/**
 * Hands out ranges of `[0, capacity())`; freed ranges are merged with free neighbours and reused.
 *
 * Only bookkeeping: the caller owns the memory and grows it (and calls grow()) when allocate() fails.
 */
class RangeAllocator {
public:
    /// A pool of @p capacity units, all free.
    explicit RangeAllocator(std::uint32_t capacity = 0) { grow(capacity); }

    /// The offset of a free range of @p size units (> 0), or nothing if no free range is big enough.
    std::optional<std::uint32_t> allocate(std::uint32_t size);
    /// Returns a range from allocate() to the pool.
    void free(std::uint32_t offset, std::uint32_t size);
    /// Enlarges the pool to @p capacity units (no effect if not larger); the new units are free.
    void grow(std::uint32_t capacity);

    /// Total units.
    [[nodiscard]] std::uint32_t capacity() const { return m_capacity; }
    /// Units currently allocated.
    [[nodiscard]] std::uint32_t used() const { return m_used; }
private:
    std::map<std::uint32_t, std::uint32_t> m_free; // offset -> size, never adjacent
    std::uint32_t m_capacity = 0;
    std::uint32_t m_used = 0;
};

} // namespace sg
