/**
 * @file
 * @brief sg::RangeAllocator.
 */

#include "RangeAllocator.h"

#include <iterator>

namespace sg {

std::optional<std::uint32_t> RangeAllocator::allocate(std::uint32_t size) {
    if (size == 0) {
        return std::nullopt;
    }
    for (auto it = m_free.begin(); it != m_free.end(); ++it) {
        if (it->second < size) {
            continue;
        }
        const std::uint32_t offset = it->first;
        const std::uint32_t remaining = it->second - size;
        m_free.erase(it);
        if (remaining > 0) {
            m_free.emplace(offset + size, remaining);
        }
        m_used += size;
        return offset;
    }
    return std::nullopt;
}

void RangeAllocator::free(std::uint32_t offset, std::uint32_t size) {
    if (size == 0) {
        return;
    }
    m_used -= size;
    auto next = m_free.lower_bound(offset);
    // Merge with the following free range.
    if (next != m_free.end() && offset + size == next->first) {
        size += next->second;
        next = m_free.erase(next);
    }
    // Merge with the preceding free range.
    if (next != m_free.begin()) {
        auto previous = std::prev(next);
        if (previous->first + previous->second == offset) {
            previous->second += size;
            return;
        }
    }
    m_free.emplace(offset, size);
}

void RangeAllocator::grow(std::uint32_t capacity) {
    if (capacity <= m_capacity) {
        return;
    }
    const std::uint32_t old_capacity = m_capacity;
    m_capacity = capacity;
    m_used += capacity - old_capacity; // free() subtracts it again
    free(old_capacity, capacity - old_capacity);
}

} // namespace sg
