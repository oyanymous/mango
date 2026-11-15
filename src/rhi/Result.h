#pragma once

#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

/**
 * The rendering hardware interface: a GPU abstraction the renderer is written against.
 *
 * Public headers in `src/rhi` are backend-independent; backends (e.g. rhi::vulkan) implement the interfaces.
 * Start with rhi::RHIFactory.
 */
namespace rhi {

/// Why an RHI operation failed.
enum class ErrorCode {
    Unknown,              /**< Unclassified backend error; see Error::message. */
    InvalidArgument,      /**< A descriptor or argument was invalid. */
    Unsupported,          /**< The backend or adapter lacks a required feature, extension or format. */
    BackendUnavailable,   /**< The backend is not compiled in, or its driver/loader is missing. */
    InitializationFailed, /**< The driver failed to create an object for an implementation-specific reason. */
    OutOfHostMemory,      /**< CPU memory allocation failed. */
    OutOfDeviceMemory,    /**< GPU memory allocation failed. */
    DeviceLost,           /**< The GPU device was lost (driver reset, hang, removal); recreate the device. */
    SurfaceLost,          /**< The window surface is no longer usable; recreate the surface. */
    OutOfDate,            /**< The swapchain no longer matches its surface; resize it. */
    Timeout,              /**< A wait did not finish in time. */
};

/// Returns the enumerator's name, e.g. `"OutOfDate"`.
std::string_view toString(ErrorCode code);

/// A failure: a machine-readable code plus a human-readable message.
struct Error {
    ErrorCode code = ErrorCode::Unknown;
    std::string message; /**< Details for logs, e.g. which call failed and the native result. */
};

/**
 * Either a value of type @p T or an Error. A stand-in for C++23 `std::expected`.
 *
 * Returned by every RHI operation that can fail. Check it before using the value:
 *
 * @code
 * rhi::Result<rhi::BufferRef> buffer = device->createBuffer(desc);
 * if (!buffer) {
 *     log(buffer.error().message);
 *     return;
 * }
 * (*buffer)->map({});
 * @endcode
 * Both alternatives convert implicitly, so a function returning `Result<T>` can `return value;` or
 * `return Error{...};`. Accessing the wrong alternative asserts in debug builds.
 *
 * @tparam T Value type. Not a reference and not Error.
 */
template <typename T>
class [[nodiscard]] Result {
    static_assert(!std::is_same_v<std::remove_cvref_t<T>, Error>, "Result<Error> is ambiguous");
    static_assert(!std::is_reference_v<T>, "Result<T&> is not supported");
public:
    /// A successful result holding @p value.
    Result(T value) : m_storage(std::in_place_index<0>, std::move(value)) {}
    /// A failed result holding @p error.
    Result(Error error) : m_storage(std::in_place_index<1>, std::move(error)) {}

    /// True if the result holds a value.
    bool hasValue() const noexcept { return m_storage.index() == 0; }
    /// True if the result holds a value.
    explicit operator bool() const noexcept { return hasValue(); }

    /// The value. Requires hasValue().
    T& value() & {
        assert(hasValue());
        return std::get<0>(m_storage);
    }
    /// The value. Requires hasValue().
    const T& value() const& {
        assert(hasValue());
        return std::get<0>(m_storage);
    }
    /// Moves the value out. Requires hasValue(). Use for move-only types: `std::move(result).value()`.
    T&& value() && {
        assert(hasValue());
        return std::get<0>(std::move(m_storage));
    }

    /// Accesses the value's members. Requires hasValue().
    T* operator->() { return &value(); }
    /// Accesses the value's members. Requires hasValue().
    const T* operator->() const { return &value(); }
    /// The value. Requires hasValue().
    T& operator*() & { return value(); }
    /// The value. Requires hasValue().
    const T& operator*() const& { return value(); }

    /**
     * The value, or @p fallback if the result holds an error.
     *
     * @param fallback Returned (converted to @p T) on error.
     */
    template <typename U>
    T valueOr(U&& fallback) const& {
        return hasValue() ? std::get<0>(m_storage) : static_cast<T>(std::forward<U>(fallback));
    }

    /// The error. Requires `!hasValue()`.
    const Error& error() const {
        assert(!hasValue());
        return std::get<1>(m_storage);
    }
private:
    std::variant<T, Error> m_storage;
};

/**
 * A result with no value: success, or an Error.
 *
 * A default-constructed `Result<void>` (`return {};`) is success.
 */
template <>
class [[nodiscard]] Result<void> {
public:
    /// Success.
    Result() = default;
    /// Failure holding @p error.
    Result(Error error) : m_error(std::move(error)) {}

    /// True on success.
    bool hasValue() const noexcept { return !m_error.has_value(); }
    /// True on success.
    explicit operator bool() const noexcept { return hasValue(); }

    /// The error. Requires `!hasValue()`.
    const Error& error() const {
        assert(!hasValue());
        return *m_error;
    }
private:
    std::optional<Error> m_error;
};

} // namespace rhi
