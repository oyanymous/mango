# API documentation with Doxygen, fetched at a pinned version so every machine generates the same output.
# Enabled with -DMANGO_BUILD_DOCS=ON; build with `cmake --build --preset <preset> --target docs`.
# Output: <build dir>/docs/html/index.html. Warnings (e.g. undocumented API) go to <build dir>/docs/warnings.log.
#
# Comment style: see "Documentation" in CLAUDE.md.

include(FetchContent)

set(MANGO_DOXYGEN_VERSION 1.18.0)
set(mango_doxygen_base_url
    "https://github.com/doxygen/doxygen/releases/download/Release_1_18_0")

if (CMAKE_HOST_WIN32)
    set(mango_doxygen_archive "doxygen-${MANGO_DOXYGEN_VERSION}.windows.x64.bin.zip")
    set(mango_doxygen_sha256 e84f54cfd49ef06b0b16536056dbec0c496323de28abcce53a4269463de35eaf)
elseif (CMAKE_HOST_APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(mango_doxygen_archive "doxygen-${MANGO_DOXYGEN_VERSION}-mac-arm.zip")
    set(mango_doxygen_sha256 31d1c74467a9f6f456b4e6a4eff0c03e93d1ffa92da5fdf2d53a2f14ebaabbd7)
elseif (CMAKE_HOST_APPLE)
    set(mango_doxygen_archive "doxygen-${MANGO_DOXYGEN_VERSION}-mac-intel.zip")
    set(mango_doxygen_sha256 8045d72f6f900fd430042339c1493d6eefcc85a7f990361fb57685682f16a2f9)
else ()
    set(mango_doxygen_archive "doxygen-${MANGO_DOXYGEN_VERSION}.linux.bin.tar.gz")
    set(mango_doxygen_sha256 14fa81bdc34171edb5f1f02b1d60e74802f0439b77fa44e592565d517d72df90)
endif ()

FetchContent_Declare(
    doxygen
    URL "${mango_doxygen_base_url}/${mango_doxygen_archive}"
    URL_HASH SHA256=${mango_doxygen_sha256}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR do-not-add # a prebuilt binary, not a CMake project
)
FetchContent_MakeAvailable(doxygen)

# The archives put the executable in different places per platform.
find_program(DOXYGEN_EXECUTABLE
    NAMES doxygen
    HINTS "${doxygen_SOURCE_DIR}"
    PATH_SUFFIXES bin Doxygen.app/Contents/Resources
    NO_DEFAULT_PATH
    REQUIRED
)
find_package(Doxygen ${MANGO_DOXYGEN_VERSION} EXACT REQUIRED)

set(DOXYGEN_PROJECT_NAME "Mango")
set(DOXYGEN_PROJECT_BRIEF "Cross-platform renderer and rendering hardware interface (RHI)")
set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/docs/Mainpage.md")
set(DOXYGEN_WARN_LOGFILE "${CMAKE_BINARY_DIR}/docs/warnings.log")
# Comments are written only where they add something the signature does not say, so undocumented members are
# extracted silently and partly documented parameter lists are fine. Wrong @param names are still reported
# (WARN_IF_DOC_ERROR).
set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
set(DOXYGEN_WARN_IF_INCOMPLETE_DOC NO)
set(DOXYGEN_EXTRACT_ALL YES)
set(DOXYGEN_EXTRACT_PRIVATE NO)
set(DOXYGEN_EXTRACT_STATIC YES)
set(DOXYGEN_EXTRACT_ANON_NSPACES NO)
set(DOXYGEN_JAVADOC_AUTOBRIEF YES) # the first sentence of a comment is its brief description
set(DOXYGEN_SORT_MEMBER_DOCS NO)   # keep declaration order
set(DOXYGEN_FULL_PATH_NAMES NO)
set(DOXYGEN_GENERATE_TREEVIEW YES)
set(DOXYGEN_HTML_DYNAMIC_SECTIONS YES)
set(DOXYGEN_SOURCE_BROWSER YES)
set(DOXYGEN_RECURSIVE YES)
set(DOXYGEN_FILE_PATTERNS *.h *.cpp *.mm *.md)
set(DOXYGEN_EXTENSION_MAPPING mm=Objective-C)
set(DOXYGEN_ENABLE_PREPROCESSING YES)
set(DOXYGEN_MACRO_EXPANSION YES)
set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
# Document the Vulkan backend as built; hide the flag-operator macro's generated operators.
set(DOXYGEN_PREDEFINED "MANGO_RHI_VULKAN=1" "MANGO_RHI_METAL=1" "MANGO_VULKAN_VALIDATION=1" "MANGO_RHI_FLAGS(Type)=")

doxygen_add_docs(docs
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/docs/Mainpage.md"
    COMMENT "Generating API documentation with Doxygen ${MANGO_DOXYGEN_VERSION}"
)
