cmake_minimum_required(VERSION 3.15)
project(PROCEDURAL_TREES)

if (MSVC)
    add_compile_options(/FS)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DGLEW_STATIC)

# Automatically find all source and header files
file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/environment/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/environment/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/environment
    ${CMAKE_CURRENT_SOURCE_DIR}/src/environment/tree
    ${CMAKE_CURRENT_SOURCE_DIR}/src/environment/sky
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/dep/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dep/glew/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dep/
)

# Specify full paths to libraries
set(GLFW3_LIB "${CMAKE_CURRENT_SOURCE_DIR}/dep/glfw/lib/glfw3.lib")
set(GLEW_LIB "${CMAKE_CURRENT_SOURCE_DIR}/dep/glew/lib/Release/Win32/glew32s.lib")

# Link libraries with full paths
target_link_libraries(${PROJECT_NAME}
    ${GLFW3_LIB}
    ${GLEW_LIB}
    opengl32
    User32
    Gdi32
    Shell32
    Ws2_32
    Winmm
)

# Set output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# For Windows subsystem (optional - removes console window)
# if(WIN32)
#     set_target_properties(${PROJECT_NAME} PROPERTIES
#         WIN32_EXECUTABLE TRUE
#     )
# endif()

# Copy assets/resources to build directory if needed
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets 
#      DESTINATION ${CMAKE_BINARY_DIR}/bin)

if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)  # Only for 32-bit MSVC builds
    # Disable SAFESEH
    target_link_options(${PROJECT_NAME} PRIVATE "/SAFESEH:NO")
endif()
