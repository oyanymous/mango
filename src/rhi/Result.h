#pragma once

#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace rhi {

enum class ErrorCode {
    Unknown,
    InvalidArgument,
    Unsupported,
    BackendUnavailable,
    InitializationFailed,
    OutOfHostMemory,
    OutOfDeviceMemory,
    DeviceLost,
    SurfaceLost,
    OutOfDate,
    Timeout,
};

std::string_view toString(ErrorCode code);

struct Error {
    ErrorCode code = ErrorCode::Unknown;
    std::string message;
};

template <typename T>
class [[nodiscard]] Result {
    static_assert(!std::is_same_v<std::remove_cvref_t<T>, Error>, "Result<Error> is ambiguous");
    static_assert(!std::is_reference_v<T>, "Result<T&> is not supported");
public:
    Result(T value) : m_storage(std::in_place_index<0>, std::move(value)) {}

    Result(Error error) : m_storage(std::in_place_index<1>, std::move(error)) {}

    bool hasValue() const noexcept { return m_storage.index() == 0; }

    explicit operator bool() const noexcept { return hasValue(); }

    T& value() & {
        assert(hasValue());
        return std::get<0>(m_storage);
    }

    const T& value() const& {
        assert(hasValue());
        return std::get<0>(m_storage);
    }

    T&& value() && {
        assert(hasValue());
        return std::get<0>(std::move(m_storage));
    }

    T* operator->() { return &value(); }

    const T* operator->() const { return &value(); }

    T& operator*() & { return value(); }

    const T& operator*() const& { return value(); }

    template <typename U>
    T valueOr(U&& fallback) const& {
        return hasValue() ? std::get<0>(m_storage) : static_cast<T>(std::forward<U>(fallback));
    }

    const Error& error() const {
        assert(!hasValue());
        return std::get<1>(m_storage);
    }
private:
    std::variant<T, Error> m_storage;
};

template <>
class [[nodiscard]] Result<void> {
public:
    Result() = default;

    Result(Error error) : m_error(std::move(error)) {}

    bool hasValue() const noexcept { return !m_error.has_value(); }

    explicit operator bool() const noexcept { return hasValue(); }

    const Error& error() const {
        assert(!hasValue());
        return *m_error;
    }
private:
    std::optional<Error> m_error;
};

} // namespace rhi
