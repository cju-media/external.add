cmake_minimum_required(VERSION 3.10)
project(simple_add)

# Instructions:
# Run cmake with -DMAX_SDK_PATH=/path/to/max-sdk
# Example: cmake -DMAX_SDK_PATH=../../max-sdk-7.3.3 ..

if(NOT DEFINED MAX_SDK_PATH)
    message(WARNING "MAX_SDK_PATH is not defined. Using default assumption or failing.")
    # You might want to default to a standard location or error out.
    # set(MAX_SDK_PATH "/path/to/default/sdk")
    message(FATAL_ERROR "Please specify MAX_SDK_PATH.")
endif()

# Include directories
set(MAX_INCLUDES "${MAX_SDK_PATH}/c74support/max-includes")
set(MSP_INCLUDES "${MAX_SDK_PATH}/c74support/msp-includes")
set(JIT_INCLUDES "${MAX_SDK_PATH}/c74support/jit-includes")

include_directories(${MAX_INCLUDES} ${MSP_INCLUDES} ${JIT_INCLUDES})

# Source files
set(SOURCES src/simple_add.c)

# Create the library
add_library(simple_add MODULE ${SOURCES})

# Platform specific configuration
if(APPLE)
    add_definitions(-DMAC_VERSION)

    # Bundle settings
    set_target_properties(simple_add PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "mxo"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.simple_add"
        MACOSX_BUNDLE_BUNDLE_NAME "simple_add"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_INFO_STRING "simple_add external"
    )

    # Linker flags for Mac (lookup symbols at runtime)
    set_target_properties(simple_add PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")

    # Code Signing
    set(CODE_SIGN_IDENTITY "-" CACHE STRING "Code signing identity for macOS (default: ad-hoc)")

    add_custom_command(TARGET simple_add POST_BUILD
        COMMAND xattr -cr "$<TARGET_FILE_DIR:simple_add>/../.."
        COMMAND codesign --force --deep -s "${CODE_SIGN_IDENTITY}" "$<TARGET_FILE_DIR:simple_add>/../.."
        COMMENT "Signing bundle with identity: ${CODE_SIGN_IDENTITY}"
    )

elseif(WIN32)
    add_definitions(-DWIN_VERSION -DEXT_WIN_VERSION -DMAXAPI_USE_MSCRT)

    # Windows suffix
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set_target_properties(simple_add PROPERTIES SUFFIX ".mxe64")
    else()
        set_target_properties(simple_add PROPERTIES SUFFIX ".mxe")
    endif()

    # Link against MaxAPI.lib
    # Assuming the lib is in the include directory for simplicity, or strictly in libs folder if exists
    # SDK structure usually has libs in specific folders, but older SDKs had .lib in includes.
    # Modern SDK (7+) has 'c74support/max-includes/x64/MaxAPI.lib' or similar.

    # Attempt to find library
    find_library(MAX_API_LIB NAMES MaxAPI PATHS
        "${MAX_INCLUDES}/x64"
        "${MAX_INCLUDES}"
        NO_DEFAULT_PATH
    )

    if(MAX_API_LIB)
        target_link_libraries(simple_add ${MAX_API_LIB})
    else()
        message(WARNING "MaxAPI.lib not found in likely locations. Linking might fail.")
    endif()

endif()
