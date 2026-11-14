#pragma once

/**
 * @file
 * @brief rhi::Result, rhi::Error and rhi::ErrorCode: how RHI operations report failure.
 */

#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

/**
 * @brief The rendering hardware interface: a GPU abstraction the renderer is written against.
 *
 * @details
 * Public headers in `src/rhi` are backend-independent; backends (e.g. rhi::vulkan) implement the interfaces.
 * Start with rhi::RHIFactory.
 */
namespace rhi {

/**
 * @brief Why an RHI operation failed.
 */
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

/**
 * @brief Returns the enumerator's name, e.g. `"OutOfDate"`.
 *
 * @param code Error code to name.
 */
std::string_view toString(ErrorCode code);

/**
 * @brief A failure: a machine-readable code plus a human-readable message.
 */
struct Error {
    ErrorCode code = ErrorCode::Unknown; /**< What kind of failure. */
    std::string message;                 /**< Details for logs, e.g. which call failed and the native result. */
};

/**
 * @brief Either a value of type @p T or an Error. A stand-in for C++23 `std::expected`.
 *
 * @details
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
    /**
     * @brief A successful result holding @p value.
     *
     * @param value The value.
     */
    Result(T value) : m_storage(std::in_place_index<0>, std::move(value)) {}
    /**
     * @brief A failed result holding @p error.
     *
     * @param error The error.
     */
    Result(Error error) : m_storage(std::in_place_index<1>, std::move(error)) {}

    /**
     * @brief True if the result holds a value.
     */
    bool hasValue() const noexcept { return m_storage.index() == 0; }
    /**
     * @brief True if the result holds a value.
     */
    explicit operator bool() const noexcept { return hasValue(); }

    /**
     * @brief The value. Requires hasValue().
     */
    T& value() & {
        assert(hasValue());
        return std::get<0>(m_storage);
    }
    /**
     * @brief The value. Requires hasValue().
     */
    const T& value() const& {
        assert(hasValue());
        return std::get<0>(m_storage);
    }
    /**
     * @brief Moves the value out. Requires hasValue(). Use for move-only types: `std::move(result).value()`.
     */
    T&& value() && {
        assert(hasValue());
        return std::get<0>(std::move(m_storage));
    }

    /**
     * @brief Accesses the value's members. Requires hasValue().
     */
    T* operator->() { return &value(); }
    /**
     * @brief Accesses the value's members. Requires hasValue().
     */
    const T* operator->() const { return &value(); }
    /**
     * @brief The value. Requires hasValue().
     */
    T& operator*() & { return value(); }
    /**
     * @brief The value. Requires hasValue().
     */
    const T& operator*() const& { return value(); }

    /**
     * @brief The value, or @p fallback if the result holds an error.
     *
     * @param fallback Returned (converted to @p T) on error.
     */
    template <typename U>
    T valueOr(U&& fallback) const& {
        return hasValue() ? std::get<0>(m_storage) : static_cast<T>(std::forward<U>(fallback));
    }

    /**
     * @brief The error. Requires `!hasValue()`.
     */
    const Error& error() const {
        assert(!hasValue());
        return std::get<1>(m_storage);
    }
private:
    std::variant<T, Error> m_storage;
};

/**
 * @brief A result with no value: success, or an Error.
 *
 * @details
 * A default-constructed `Result<void>` (`return {};`) is success.
 */
template <>
class [[nodiscard]] Result<void> {
public:
    /**
     * @brief Success.
     */
    Result() = default;
    /**
     * @brief Failure holding @p error.
     *
     * @param error The error.
     */
    Result(Error error) : m_error(std::move(error)) {}

    /**
     * @brief True on success.
     */
    bool hasValue() const noexcept { return !m_error.has_value(); }
    /**
     * @brief True on success.
     */
    explicit operator bool() const noexcept { return hasValue(); }

    /**
     * @brief The error. Requires `!hasValue()`.
     */
    const Error& error() const {
        assert(!hasValue());
        return *m_error;
    }
private:
    std::optional<Error> m_error;
};

} // namespace rhi
