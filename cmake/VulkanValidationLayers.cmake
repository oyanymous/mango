# Builds the Khronos validation layer (VK_LAYER_KHRONOS_validation) from source at the same pinned SDK tag as
# Vulkan-Headers. It is a separate ExternalProject so it builds once (slow: it compiles SPIRV-Tools)
# and does not rebuild with the rest of the project. Its own dependencies are fetched by its UPDATE_DEPS
# script, which needs Python 3 on PATH.
#
# Defines:
#   mango_vulkan_validation_layers   target to depend on, so the layer is built before it is used
#   MANGO_VULKAN_LAYER_PATH          directory holding the layer's JSON manifest (for VK_LAYER_PATH)

include(ExternalProject)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Short directory names on purpose: SPIRV-Tools has very long object file names, and the compiler
# fails once a path passes Windows' 260-character limit.
set(MANGO_VVL_ROOT "${CMAKE_BINARY_DIR}/vvl")
set(MANGO_VVL_INSTALL_DIR "${MANGO_VVL_ROOT}/install")

string(LENGTH "${CMAKE_BINARY_DIR}" mango_binary_dir_length)
if (WIN32 AND mango_binary_dir_length GREATER 70)
    message(WARNING "The build directory path is ${mango_binary_dir_length} characters long. Building the validation "
                    "layer may exceed Windows' 260-character path limit; use a shorter build directory or "
                    "-DMANGO_VULKAN_VALIDATION=OFF.")
endif ()

if (WIN32)
    set(MANGO_VULKAN_LAYER_PATH "${MANGO_VVL_INSTALL_DIR}/bin")
    set(mango_vvl_manifest "${MANGO_VULKAN_LAYER_PATH}/VkLayer_khronos_validation.json")
else ()
    set(MANGO_VULKAN_LAYER_PATH "${MANGO_VVL_INSTALL_DIR}/share/vulkan/explicit_layer.d")
    set(mango_vvl_manifest "${MANGO_VULKAN_LAYER_PATH}/VkLayer_khronos_validation.json")
endif ()

set(mango_vvl_cmake_args
    -DCMAKE_BUILD_TYPE=Release # a Debug validation layer is far too slow to be useful
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DUPDATE_DEPS=ON
    -DUPDATE_DEPS_DIR=${MANGO_VVL_ROOT}/deps
    -DBUILD_TESTS=OFF
    -DPython3_EXECUTABLE=${Python3_EXECUTABLE}
)
if (CMAKE_C_COMPILER)
    list(APPEND mango_vvl_cmake_args "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
endif ()
# With MSVC + Ninja the layer build inherits the MSVC environment (INCLUDE, LIB, PATH) of the parent build.

ExternalProject_Add(mango_vulkan_validation_layers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-ValidationLayers.git
    GIT_TAG ${MANGO_VULKAN_SDK_TAG}
    GIT_SHALLOW TRUE
    PREFIX "${MANGO_VVL_ROOT}"
    SOURCE_DIR "${MANGO_VVL_ROOT}/src"
    STAMP_DIR "${MANGO_VVL_ROOT}/stamp" # outside SOURCE_DIR, which the git clone replaces
    TMP_DIR "${MANGO_VVL_ROOT}/tmp"
    BINARY_DIR "${MANGO_VVL_ROOT}/build"
    INSTALL_DIR "${MANGO_VVL_INSTALL_DIR}"
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS ${mango_vvl_cmake_args}
    UPDATE_DISCONNECTED TRUE # the tag is pinned; never re-fetch on rebuild
    BUILD_BYPRODUCTS "${mango_vvl_manifest}"
    USES_TERMINAL_BUILD TRUE
)
