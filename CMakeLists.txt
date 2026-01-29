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
set(SOURCES_STREAM src/jit.stream.c)

# Create the library for simple_add
add_library(simple_add MODULE ${SOURCES})

# Create the library for jit.stream
add_library(jit_stream MODULE ${SOURCES_STREAM})

# Platform specific configuration
if(APPLE)
    add_definitions(-DMAC_VERSION)

    # Bundle settings for simple_add
    set_target_properties(simple_add PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "mxo"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.simple_add"
        MACOSX_BUNDLE_BUNDLE_NAME "simple_add"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_INFO_STRING "simple_add external"
        LINK_FLAGS "-undefined dynamic_lookup"
    )

    # Bundle settings for jit.stream
    set_target_properties(jit_stream PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "mxo"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.jit_stream"
        MACOSX_BUNDLE_BUNDLE_NAME "jit.stream"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_INFO_STRING "jit.stream external"
        LINK_FLAGS "-undefined dynamic_lookup"
    )

    # Link against Jitter API Framework (standard system path or in SDK)
    # Usually finding JitterAPI.framework in SDK or /Library/Frameworks
    find_library(JITTER_API_FRAMEWORK NAMES JitterAPI PATHS
        "${MAX_SDK_PATH}/c74support/jit-includes"
        NO_DEFAULT_PATH
    )
    if (JITTER_API_FRAMEWORK)
        target_link_libraries(jit_stream ${JITTER_API_FRAMEWORK})
    endif()

    # Code Signing
    set(CODE_SIGN_IDENTITY "-" CACHE STRING "Code signing identity for macOS (default: ad-hoc)")

    add_custom_command(TARGET simple_add POST_BUILD
        COMMAND xattr -cr "$<TARGET_FILE_DIR:simple_add>/../.."
        COMMAND codesign --force --deep -s "${CODE_SIGN_IDENTITY}" "$<TARGET_FILE_DIR:simple_add>/../.."
        COMMENT "Signing bundle with identity: ${CODE_SIGN_IDENTITY}"
    )

    add_custom_command(TARGET jit_stream POST_BUILD
        COMMAND xattr -cr "$<TARGET_FILE_DIR:jit_stream>/../.."
        COMMAND codesign --force --deep -s "${CODE_SIGN_IDENTITY}" "$<TARGET_FILE_DIR:jit_stream>/../.."
        COMMENT "Signing bundle with identity: ${CODE_SIGN_IDENTITY}"
    )

elseif(WIN32)
    add_definitions(-DWIN_VERSION -DEXT_WIN_VERSION -DMAXAPI_USE_MSCRT)

    # Windows suffix
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set_target_properties(simple_add PROPERTIES SUFFIX ".mxe64")
        set_target_properties(jit_stream PROPERTIES SUFFIX ".mxe64")
    else()
        set_target_properties(simple_add PROPERTIES SUFFIX ".mxe")
        set_target_properties(jit_stream PROPERTIES SUFFIX ".mxe")
    endif()

    # Link against MaxAPI.lib
    # Attempt to find library
    find_library(MAX_API_LIB NAMES MaxAPI PATHS
        "${MAX_INCLUDES}/x64"
        "${MAX_INCLUDES}"
        NO_DEFAULT_PATH
    )

    find_library(JIT_API_LIB NAMES jitlib PATHS
        "${JIT_INCLUDES}/x64"
        "${JIT_INCLUDES}"
        NO_DEFAULT_PATH
    )

    if(MAX_API_LIB)
        target_link_libraries(simple_add ${MAX_API_LIB})
        target_link_libraries(jit_stream ${MAX_API_LIB})
    else()
        message(WARNING "MaxAPI.lib not found. Linking might fail.")
    endif()

    if(JIT_API_LIB)
        target_link_libraries(jit_stream ${JIT_API_LIB})
    else()
        message(WARNING "jitlib.lib not found. Linking jit_stream might fail.")
    endif()

endif()
