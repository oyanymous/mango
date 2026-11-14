#pragma once

/**
 * @file
 * @brief sg::RangeAllocator: first-fit allocation of ranges in a growable pool (vertices, node slots).
 */

#include <cstdint>
#include <map>
#include <optional>

namespace sg {

/**
 * @brief Hands out ranges of `[0, capacity())`; freed ranges are merged with free neighbours and reused.
 *
 * @details
 * Only bookkeeping: the caller owns the memory and grows it (and calls grow()) when allocate() fails.
 */
class RangeAllocator {
public:
    /**
     * @brief A pool of @p capacity units, all free.
     */
    explicit RangeAllocator(std::uint32_t capacity = 0) { grow(capacity); }

    /**
     * @brief The offset of a free range of @p size units (> 0), or nothing if no free range is big enough.
     */
    std::optional<std::uint32_t> allocate(std::uint32_t size);
    /**
     * @brief Returns a range from allocate() to the pool.
     */
    void free(std::uint32_t offset, std::uint32_t size);
    /**
     * @brief Enlarges the pool to @p capacity units (no effect if not larger); the new units are free.
     */
    void grow(std::uint32_t capacity);

    /** @brief Total units. */
    [[nodiscard]] std::uint32_t capacity() const { return m_capacity; }
    /** @brief Units currently allocated. */
    [[nodiscard]] std::uint32_t used() const { return m_used; }
private:
    std::map<std::uint32_t, std::uint32_t> m_free; // offset -> size, never adjacent
    std::uint32_t m_capacity = 0;
    std::uint32_t m_used = 0;
};

} // namespace sg
