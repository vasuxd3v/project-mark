// Engine/Overlay/Overlay.mm — macOS port
// File is .mm (Objective-C++) because Metal and AppKit require it.
//
// ─── RENDERING BACKEND COMPARISON ────────────────────────────────────────────
//
//   Concept              Windows (DirectX 11)          macOS (Metal)
//   ──────────────────────────────────────────────────────────────────────────
//   Graphics API         DirectX 11 (D3D11)            Metal
//   Device type          ID3D11Device*                  id<MTLDevice>
//   Command queue        ID3D11DeviceContext*            id<MTLCommandQueue>
//   Render target        IDXGISwapChain / back buffer   CAMetalLayer / MTKView
//   Shader language      HLSL                           Metal Shading Language (MSL)
//   Present hook         Hook IDXGISwapChain::Present   Hook CAMetalLayer/MTKView present
//   ImGui backend        imgui_impl_dx11.h              imgui_impl_metal.h
//   Window backend       imgui_impl_win32.h             imgui_impl_osx.h
//
// KEY CONCEPT — Why Metal instead of OpenGL?
//   Apple deprecated OpenGL in 2018 (macOS 10.14 Mojave) and will eventually
//   remove it. Metal is Apple's first-party, modern GPU API — the only supported
//   path for production-quality rendering on modern macOS/iOS.
//   DirectX 11 is Windows-only (Microsoft's proprietary GPU API).
//   Both are low-level, shader-based APIs designed for the same job.
//
// ImGui has official backends for both:
//   backends/imgui_impl_dx11.cpp    (Windows DX11)
//   backends/imgui_impl_metal.mm    (macOS Metal)
//   backends/imgui_impl_win32.cpp   (Windows window events)
//   backends/imgui_impl_osx.mm      (macOS window events)
//
// The ImGui core (imgui.cpp, imgui_draw.cpp, etc.) is IDENTICAL on both
// platforms. Only the backend files change.

#import <Metal/Metal.h>         // MTLDevice, MTLCommandQueue, etc.
#import <MetalKit/MetalKit.h>   // MTKView
#import <AppKit/AppKit.h>       // NSWindow, NSView
#import <QuartzCore/CAMetalLayer.h>  // CAMetalLayer (swap chain equivalent)
#include "Overlay.hpp"
#include "imgui.h"
#include "backends/imgui_impl_metal.h"   // replaces imgui_impl_dx11.h
#include "backends/imgui_impl_osx.h"     // replaces imgui_impl_win32.h

// Metal equivalents of DirectX 11 objects
static id<MTLDevice>        g_device       = nil;   // = ID3D11Device*
static id<MTLCommandQueue>  g_commandQueue = nil;   // = ID3D11DeviceContext*
static MTKView*             g_mtkView      = nil;   // = IDXGISwapChain surface

// ─────────────────────────────────────────────────────────────────────────────
// COverlay::Initialize()
//
// Windows equivalent:
//   D3D11CreateDeviceAndSwapChain(...)
//   ImGui_ImplWin32_Init(hwnd)
//   ImGui_ImplDX11_Init(device, context)
//
// macOS equivalent:
//   MTLCreateSystemDefaultDevice()  — creates GPU device (like D3D11CreateDevice)
//   [device newCommandQueue]         — creates command queue (like DeviceContext)
//   MTKView(frame, device)          — creates Metal view (like SwapChain surface)
//   ImGui_ImplMetal_Init(device)
//   ImGui_ImplOSX_Init(view)
// ─────────────────────────────────────────────────────────────────────────────
void COverlay::Initialize() {
    // Create GPU device — replaces D3D11CreateDevice
    // MTLCreateSystemDefaultDevice() returns the best GPU available
    g_device = MTLCreateSystemDefaultDevice();
    if (!g_device) return;

    // Create command queue — replaces ID3D11DeviceContext
    // All GPU commands are encoded into command buffers submitted to this queue
    g_commandQueue = [g_device newCommandQueue];

    // Find the target NSWindow (equivalent of finding the game's HWND)
    // On Windows: FindWindowA("UnrealWindow", "Roblox")
    // On macOS:   iterate NSApp.windows looking for the Roblox window
    NSWindow* targetWindow = nil;
    for (NSWindow* win in [NSApp windows]) {
        if ([win.title containsString:@"Roblox"]) {
            targetWindow = win;
            break;
        }
    }
    if (!targetWindow) return;

    // Create MTKView and attach it as a sublayer on the target view
    // Windows equivalent: creating a child window or D3D overlay
    NSRect frame = targetWindow.contentView.bounds;
    g_mtkView = [[MTKView alloc] initWithFrame:frame device:g_device];
    g_mtkView.clearColor = MTLClearColorMake(0, 0, 0, 0); // transparent
    [targetWindow.contentView addSubview:g_mtkView];

    // ImGui initialization — same pattern as DX11, different backend
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // ImGui_ImplOSX_Init replaces ImGui_ImplWin32_Init
    ImGui_ImplOSX_Init(g_mtkView);

    // ImGui_ImplMetal_Init replaces ImGui_ImplDX11_Init(device, context)
    ImGui_ImplMetal_Init(g_device);
}

// ─────────────────────────────────────────────────────────────────────────────
// COverlay::Render()
//
// Windows: called from hooked IDXGISwapChain::Present
//   ImGui_ImplDX11_NewFrame()
//   ImGui_ImplWin32_NewFrame()
//   ImGui::NewFrame()
//   // ... draw ImGui windows ...
//   ImGui::Render()
//   ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData())
//
// macOS: called from MTKView delegate's drawInMTKView:
//   ImGui_ImplMetal_NewFrame(renderPassDescriptor)
//   ImGui_ImplOSX_NewFrame(view)
//   ImGui::NewFrame()
//   // ... draw ImGui windows ...
//   ImGui::Render()
//   ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderEncoder)
// ─────────────────────────────────────────────────────────────────────────────
void COverlay::Render() {
    @autoreleasepool {
        id<CAMetalDrawable> drawable = [g_mtkView currentDrawable];
        MTLRenderPassDescriptor* rpd = g_mtkView.currentRenderPassDescriptor;
        if (!drawable || !rpd) return;

        id<MTLCommandBuffer> commandBuffer = [g_commandQueue commandBuffer];
        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:rpd];

        // Begin frame — replaces ImGui_ImplDX11_NewFrame() + ImGui_ImplWin32_NewFrame()
        ImGui_ImplMetal_NewFrame(rpd);       // replaces ImGui_ImplDX11_NewFrame
        ImGui_ImplOSX_NewFrame(g_mtkView);   // replaces ImGui_ImplWin32_NewFrame
        ImGui::NewFrame();

        // Draw your ImGui windows here — IDENTICAL to Windows version

        // Render — replaces ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData())
        ImGui::Render();
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, encoder);

        [encoder endEncoding];
        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
    }
}

void COverlay::Shutdown() {
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplOSX_Shutdown();
    ImGui::DestroyContext();
}
