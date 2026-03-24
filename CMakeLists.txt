# CMakeLists.txt — project-mark macOS Port
# Replaces: Cobalt.sln + Module/binninsploit.vcxproj (Visual Studio solution)
#
# Build system:
#   Windows: .sln / .vcxproj  (MSBuild / Visual Studio 2022)
#   macOS:   CMakeLists.txt   (CMake → Xcode or Makefile or Ninja)
#
# Output:
#   Windows: Module.dll        (PE Dynamic Link Library)
#   macOS:   libCobalt.dylib   (Mach-O Dynamic Library)
#
# HOW TO BUILD:
#   brew install cmake cryptopp
#   cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64"
#   cmake --build build -j$(sysctl -n hw.logicalcpu)

cmake_minimum_required(VERSION 3.20)
project(Cobalt LANGUAGES CXX OBJCXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 20)

# Shorthand for the dependencies root (corrected path)
set(DEPS Module/Dependencies/Dependencies)

# ─── Collect Luau VM sources via glob (all .cpp in VM/src) ───────────────────
file(GLOB LUAU_VM_SOURCES      "${DEPS}/Luau/VM/src/*.cpp")
file(GLOB LUAU_COMPILER_SOURCES "${DEPS}/Luau/Compiler/src/*.cpp")
file(GLOB LUAU_AST_SOURCES     "${DEPS}/Luau/Ast/src/*.cpp")
file(GLOB LUAU_COMMON_SOURCES  "${DEPS}/Luau/Common/src/*.cpp")
file(GLOB CPR_SOURCES          "${DEPS}/cpr/src/*.cpp")
file(GLOB IMGUI_SOURCES        "${DEPS}/imgui/imgui*.cpp")

# ─── Build target: .dylib (replaces .dll) ────────────────────────────────────
add_library(Cobalt SHARED

    # Entry point — __attribute__((constructor)) replaces DllMain
    Module/Root/Main.mm

    # Darwin wrappers (macOS-specific, no Windows equivalent)
    Module/Root/Darwin/IOKitHWID.mm

    # Overlay — Metal backend replaces DirectX 11
    Module/Root/Engine/Overlay/Overlay.mm

    # Miscellaneous
    Module/Root/Miscellaneous/Common/Logger/Logger.cpp
    Module/Root/Miscellaneous/Common/VEH/VEH.cpp
    Module/Root/Miscellaneous/Communications/Communications.cpp

    # Roblox subsystems
    Module/Root/Roblox/Scheduler/Scheduler.cpp
    Module/Root/Roblox/Scheduler/TaskScheduler.cpp
    Module/Root/Roblox/Yieldworker/Yielder.cpp

    # Workspace
    Module/Root/Workspace/Execution/Execution.cpp
    Module/Root/Workspace/AutoExecute/Autoexec.cpp
    Module/Root/Workspace/Framework/Environment.cpp
    Module/Root/Workspace/Framework/Modules/Script/Script.cpp
    Module/Root/Workspace/Framework/Modules/HTTP/HTTP.cpp
    Module/Root/Workspace/Framework/Modules/FileLib/Filesystem.cpp
    Module/Root/Workspace/Framework/Modules/Input/Input.mm
    Module/Root/Workspace/Framework/Modules/Closures/Closures.cpp
    Module/Root/Workspace/TPService/Handler.cpp

    # Luau VM + Compiler + AST + Common (via glob — all .cpp files)
    ${LUAU_VM_SOURCES}
    ${LUAU_COMPILER_SOURCES}
    ${LUAU_AST_SOURCES}
    ${LUAU_COMMON_SOURCES}

    # ImGui core
    ${IMGUI_SOURCES}

    # ImGui Metal backends
    ${DEPS}/imgui/backends/imgui_impl_metal.mm
    ${DEPS}/imgui/backends/imgui_impl_osx.mm

    # cpr HTTP client
    ${CPR_SOURCES}

    # lz4 (source files — cross-platform C)
    ${DEPS}/lz4/include/lz4.c
    ${DEPS}/lz4/include/lz4_memory.c
)

# ─── Include directories ──────────────────────────────────────────────────────
target_include_directories(Cobalt PRIVATE
    # Project source roots
    Module/Root
    Module/Root/Miscellaneous
    Module/Root/Roblox
    Module/Root/Workspace
    Module/Root/Darwin

    # Engine — lobject.h has #include <Engine.hpp> and <Encryptions.hpp>
    Module/Root/Engine

    # Luau — root needed for "VM/src/lcommon.h" and "Compiler/include/..." style includes
    ${DEPS}/Luau
    ${DEPS}/Luau/VM/include
    ${DEPS}/Luau/VM/src
    ${DEPS}/Luau/Compiler/include
    ${DEPS}/Luau/Common/include
    ${DEPS}/Luau/Ast/include

    # Third-party
    ${DEPS}/imgui
    ${DEPS}/cpr/include
    ${DEPS}/nlohmann
    ${DEPS}/lz4/include
    ${DEPS}/zstd/include

    # All of DEPS (for includes like <Engine.hpp>, <Bytecode/Bytecode.hpp>)
    ${DEPS}
)

# ─── macOS Frameworks ─────────────────────────────────────────────────────────
# Replaces Windows .lib files (Ws2_32.lib, Psapi.lib, d3d11.lib, etc.)
#
#   Windows lib        macOS framework / library
#   ─────────────────────────────────────────────
#   Ws2_32.lib     →   (not needed — BSD sockets built-in)
#   Psapi.lib      →   IOKit framework
#   d3d11.lib      →   Metal framework
#   dxgi.lib       →   MetalKit + QuartzCore
#   user32.lib     →   AppKit framework
#   (no equiv)     →   CoreGraphics (CGEvent input)
target_link_libraries(Cobalt PRIVATE
    "-framework Foundation"
    "-framework IOKit"
    "-framework Metal"
    "-framework MetalKit"
    "-framework AppKit"
    "-framework CoreGraphics"
    "-framework ApplicationServices"
    "-framework QuartzCore"
    "-framework CoreFoundation"
    "-framework GameController"
    curl                    # pre-installed on macOS (/usr/lib/libcurl.dylib)
)

# ─── CryptoPP ─────────────────────────────────────────────────────────────────
# CryptoPP is cross-platform C++ — same source files compile on macOS.
# Install via: brew install cryptopp
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(CRYPTOPP libcryptopp QUIET)
endif()

# Try find_package (Homebrew installs a CryptoPP cmake config on newer versions)
if(NOT CRYPTOPP_FOUND)
    find_library(CRYPTOPP_LIB NAMES cryptopp crypto++ PATHS /opt/homebrew/lib /usr/local/lib)
    find_path(CRYPTOPP_INCLUDE NAMES cryptopp/md5.h PATHS /opt/homebrew/include /usr/local/include)
    if(CRYPTOPP_LIB AND CRYPTOPP_INCLUDE)
        set(CRYPTOPP_FOUND TRUE)
        target_include_directories(Cobalt PRIVATE ${CRYPTOPP_INCLUDE})
        target_link_libraries(Cobalt PRIVATE ${CRYPTOPP_LIB})
    endif()
endif()

# ─── zstd ────────────────────────────────────────────────────────────────────
# zstd is cross-platform. Install via: brew install zstd
find_library(ZSTD_LIB NAMES zstd PATHS /opt/homebrew/lib /usr/local/lib)
if(ZSTD_LIB)
    target_link_libraries(Cobalt PRIVATE ${ZSTD_LIB})
else()
    message(WARNING "zstd not found via brew. Run: brew install zstd")
endif()

# ─── Compiler flags ───────────────────────────────────────────────────────────
target_compile_options(Cobalt PRIVATE
    -fvisibility=hidden         # Only export __attribute__((visibility("default"))) symbols
    -fno-objc-arc               # Manual ObjC memory management
    -fblocks                    # Enable ^{} block syntax for GCD in C++ files
    -Wno-deprecated-declarations
    -Wno-unused-variable
)

# ─── Output ──────────────────────────────────────────────────────────────────
set_target_properties(Cobalt PROPERTIES
    OUTPUT_NAME "Cobalt"
    SUFFIX      ".dylib"
    PREFIX      "lib"
)

set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
