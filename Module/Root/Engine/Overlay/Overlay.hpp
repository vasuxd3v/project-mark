// Engine/Overlay/Overlay.hpp — macOS port
// Interface kept identical — same COverlay class name for all callers.
//
// Windows: DirectX 11 (ID3D11Device, IDXGISwapChain, hook Present)
//           imgui_impl_dx11.h + imgui_impl_win32.h
//
// macOS:   Metal (MTLDevice, CAMetalLayer, MTKView)
//           imgui_impl_metal.h + imgui_impl_osx.h
//
// Both ImGui backends expose the same BeginFrame / EndFrame / RenderDrawData
// pattern — only the init code and the render call differ.

#pragma once
#include "Includes.hpp"

class COverlay {
public:
    static void Initialize();
    static void Render();
    static void Shutdown();
};

inline auto Overlay = std::make_unique<COverlay>();
