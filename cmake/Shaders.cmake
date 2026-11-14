# GLSL -> SPIR-V at build time with glslang, fetched and built from source at the same SDK tag as the Vulkan
# headers, so no installed glslc/glslangValidator is needed.
#
# Defines:
#   mango_add_shaders(<target> <shader>...)
#       Compiles each GLSL file (stage from its extension: .vert, .frag, .comp, ...) to a header
#       `<file name>.h` in the target's include path, holding `const uint32_t <file_name>_spv[]`
#       (e.g. triangle.vert -> triangle.vert.h with triangle_vert_spv).

include(FetchContent)

set(ENABLE_OPT OFF CACHE BOOL "" FORCE) # the optimizer needs SPIRV-Tools; unoptimized SPIR-V is fine here
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES ON CACHE BOOL "" FORCE)
set(GLSLANG_TESTS OFF CACHE BOOL "" FORCE)
set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG ${MANGO_VULKAN_SDK_TAG}
    GIT_SHALLOW TRUE
    SYSTEM
)
FetchContent_MakeAvailable(glslang)
if (MINGW)
    # glslang links its tools with -static on MinGW, which CLion's bundled MinGW cannot do (missing
    # __intrinsic_setjmpex); link the compiler against the runtime DLLs like the project's own executables.
    set_property(TARGET glslang-standalone PROPERTY LINK_OPTIONS "")
endif ()

function(mango_add_shaders target)
    set(out_dir "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    foreach (shader IN LISTS ARGN)
        get_filename_component(name "${shader}" NAME)
        string(MAKE_C_IDENTIFIER "${name}_spv" variable)
        set(output "${out_dir}/${name}.h")
        add_custom_command(
            OUTPUT "${output}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${out_dir}"
            COMMAND glslang-standalone -V --target-env vulkan1.2 --vn "${variable}" -o "${output}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/${shader}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${shader}" glslang-standalone
            COMMENT "Compiling ${shader} to SPIR-V"
            VERBATIM
        )
        target_sources(${target} PRIVATE "${output}")
    endforeach ()
    target_include_directories(${target} PRIVATE "${out_dir}")
endfunction()
