// Workspace/Framework/Modules/Input/Input.mm — macOS port
// This file is .mm (Objective-C++) because NSAlert requires ObjC.
// CGEvent APIs work in plain C++ too, but .mm allows both.
//
// ─── API MAPPING TABLE ───────────────────────────────────────────────────────
//
//   Windows API              macOS equivalent              Header
//   ─────────────────────────────────────────────────────────────────────────
//   GetForegroundWindow()    CGWindowListCopyWindowInfo()  CoreGraphics
//   FindWindowA("Roblox")    Check kCGWindowOwnerName      CoreGraphics
//   keybd_event()            CGEventCreateKeyboardEvent()  CoreGraphics
//   MapVirtualKeyA()         CGKeyCode (direct key code)   CoreGraphics
//   KEYEVENTF_SCANCODE       Not needed — CGKeyCode direct CoreGraphics
//   mouse_event(LEFT_DOWN)   CGEventCreateMouseEvent()     CoreGraphics
//   MOUSEEVENTF_LEFTDOWN     kCGEventLeftMouseDown         CoreGraphics
//   MOUSEEVENTF_RIGHTDOWN    kCGEventRightMouseDown        CoreGraphics
//   MOUSEEVENTF_WHEEL        kCGEventScrollWheel           CoreGraphics
//   MOUSEEVENTF_ABSOLUTE     CGEventSetLocation()          CoreGraphics
//   GetSystemMetrics(CX)     CGDisplayPixelsWide()         CoreGraphics
//   MessageBoxA()            NSAlert / NSApp               AppKit
//
// CGEventPost(kCGAnnotatedSessionEventTap, event) is the macOS equivalent of
// sending synthetic input events to the system. The "AnnotatedSession" tap
// ensures the event goes to the foreground application.
//
// NOTE ON PERMISSIONS:
//   On macOS, generating synthetic input events requires either:
//   a) The process is trusted (Accessibility permission in System Prefs), OR
//   b) It's an app with com.apple.security.automation.apple-events entitlement
//   This is analogous to Windows requiring UIPI trust for input injection.
//
// Link with: -framework CoreGraphics -framework ApplicationServices -framework AppKit

#import <AppKit/AppKit.h>                        // NSAlert — replaces MessageBoxA
#include <CoreGraphics/CoreGraphics.h>           // umbrella: CGEvent, CGDisplay, CGWindow, etc.
#include "Input.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// RobloxActive()
//
// Windows: GetForegroundWindow() == FindWindowA(NULL, "Roblox")
//   — Get the current foreground HWND, compare to the Roblox window HWND
//
// macOS: Query the window list and check if the frontmost window belongs
//   to a process named "RobloxPlayer" (or similar)
//
// CGWindowListCopyWindowInfo returns a CFArrayRef of window dictionaries.
// The frontmost window is at index 0 (windows are in z-order).
// kCGWindowOwnerName gives the process name for that window.
// ─────────────────────────────────────────────────────────────────────────────
static bool RobloxActive() {
    CFArrayRef windowList = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
        kCGNullWindowID
    );
    if (!windowList) return false;

    bool active = false;
    CFIndex count = CFArrayGetCount(windowList);
    if (count > 0) {
        // Index 0 = frontmost window (highest z-order)
        CFDictionaryRef topWindow = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, 0);
        CFStringRef ownerName = (CFStringRef)CFDictionaryGetValue(topWindow, kCGWindowOwnerName);
        if (ownerName) {
            char buf[256] = {};
            CFStringGetCString(ownerName, buf, sizeof(buf), kCFStringEncodingUTF8);
            // Check if it's RobloxPlayer (exact name depends on macOS Roblox build)
            active = (strstr(buf, "Roblox") != nullptr);
        }
    }

    CFRelease(windowList);
    return active;
}

// ─────────────────────────────────────────────────────────────────────────────
// keypress / keytap / keyrelease
//
// Windows: keybd_event(0, (BYTE)MapVirtualKeyA(key, MAPVK_VK_TO_VSC), KEYEVENTF_SCANCODE, 0)
//   — Uses scan codes (hardware key codes). MapVirtualKeyA converts VK_ codes to scan codes.
//
// macOS: CGEventCreateKeyboardEvent(source, keyCode, keyDown)
//   — Uses CGKeyCode directly. macOS key codes are different from Windows VK_ codes.
//     A mapping table (not shown here) would translate between them.
//
// CGEventPost(kCGAnnotatedSessionEventTap, event):
//   — Posts the event to the "AnnotatedSession" event tap, which delivers it to
//     the application at the front of the event stream.
//   — Windows equivalent: keybd_event() posts directly to the input queue.
// ─────────────────────────────────────────────────────────────────────────────
static int keypress(lua_State* L) {
    luaL_checktype(L, 1, LUA_TNUMBER);
    CGKeyCode key = (CGKeyCode)lua_tointeger(L, 1);

    if (!RobloxActive()) return 0;

    // CGEventCreateKeyboardEvent(source, virtualKey, keyDown)
    // source = nullptr means "default event source"
    // keyDown = true = key pressed, false = key released
    CGEventRef event = CGEventCreateKeyboardEvent(nullptr, key, true);  // key down
    CGEventPost(kCGAnnotatedSessionEventTap, event);
    CFRelease(event);
    return 0;
}

static int keytap(lua_State* L) {
    luaL_checktype(L, 1, LUA_TNUMBER);
    CGKeyCode key = (CGKeyCode)lua_tointeger(L, 1);

    if (!RobloxActive()) return 0;

    // Windows: keybd_event down then keybd_event up (two calls)
    // macOS:   same pattern — create key down, post it, create key up, post it
    CGEventRef down = CGEventCreateKeyboardEvent(nullptr, key, true);
    CGEventRef up   = CGEventCreateKeyboardEvent(nullptr, key, false);
    CGEventPost(kCGAnnotatedSessionEventTap, down);
    CGEventPost(kCGAnnotatedSessionEventTap, up);
    CFRelease(down);
    CFRelease(up);
    return 0;
}

static int keyrelease(lua_State* L) {
    luaL_checktype(L, 1, LUA_TNUMBER);
    CGKeyCode key = (CGKeyCode)lua_tointeger(L, 1);

    if (!RobloxActive()) return 0;

    CGEventRef event = CGEventCreateKeyboardEvent(nullptr, key, false);  // key up
    CGEventPost(kCGAnnotatedSessionEventTap, event);
    CFRelease(event);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mouse click functions
//
// Windows: mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
//
// macOS: CGEventCreateMouseEvent(source, eventType, mouseCursorPosition, button)
//   — CGEventGetLocation(CGEventCreate(nullptr)) gets the current cursor position
//   — kCGEventLeftMouseDown / kCGEventLeftMouseUp are the event types
//   — kCGMouseButtonLeft / kCGMouseButtonRight are the button constants
// ─────────────────────────────────────────────────────────────────────────────
static CGPoint getCurrentCursorPosition() {
    CGEventRef event = CGEventCreate(nullptr);
    CGPoint pos = CGEventGetLocation(event);
    CFRelease(event);
    return pos;
}

static int mouse1click(lua_State* L) {
    if (!RobloxActive()) return 0;
    CGPoint pos = getCurrentCursorPosition();
    // Windows: mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, ...)
    // macOS:   separate down + up events (same two-step approach)
    CGEventRef dn = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDown, pos, kCGMouseButtonLeft);
    CGEventRef up = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseUp,   pos, kCGMouseButtonLeft);
    CGEventPost(kCGAnnotatedSessionEventTap, dn);
    CGEventPost(kCGAnnotatedSessionEventTap, up);
    CFRelease(dn); CFRelease(up);
    return 0;
}

static int mouse1press(lua_State* L) {
    if (!RobloxActive()) return 0;
    CGPoint pos = getCurrentCursorPosition();
    CGEventRef ev = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDown, pos, kCGMouseButtonLeft);
    CGEventPost(kCGAnnotatedSessionEventTap, ev);
    CFRelease(ev);
    return 0;
}

static int mouse1release(lua_State* L) {
    if (!RobloxActive()) return 0;
    CGPoint pos = getCurrentCursorPosition();
    CGEventRef ev = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseUp, pos, kCGMouseButtonLeft);
    CGEventPost(kCGAnnotatedSessionEventTap, ev);
    CFRelease(ev);
    return 0;
}

static int mouse2click(lua_State* L) {
    if (!RobloxActive()) return 0;
    CGPoint pos = getCurrentCursorPosition();
    CGEventRef dn = CGEventCreateMouseEvent(nullptr, kCGEventRightMouseDown, pos, kCGMouseButtonRight);
    CGEventRef up = CGEventCreateMouseEvent(nullptr, kCGEventRightMouseUp,   pos, kCGMouseButtonRight);
    CGEventPost(kCGAnnotatedSessionEventTap, dn);
    CGEventPost(kCGAnnotatedSessionEventTap, up);
    CFRelease(dn); CFRelease(up);
    return 0;
}

static int mouse2press(lua_State* L) {
    if (!RobloxActive()) return 0;
    CGPoint pos = getCurrentCursorPosition();
    CGEventRef ev = CGEventCreateMouseEvent(nullptr, kCGEventRightMouseDown, pos, kCGMouseButtonRight);
    CGEventPost(kCGAnnotatedSessionEventTap, ev);
    CFRelease(ev);
    return 0;
}

static int mouse2release(lua_State* L) {
    if (!RobloxActive()) return 0;
    CGPoint pos = getCurrentCursorPosition();
    CGEventRef ev = CGEventCreateMouseEvent(nullptr, kCGEventRightMouseUp, pos, kCGMouseButtonRight);
    CGEventPost(kCGAnnotatedSessionEventTap, ev);
    CFRelease(ev);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// mousemoveabs — absolute mouse movement
//
// Windows: mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, scaledX, scaledY, 0, 0)
//   — Windows uses a normalized 0-65535 coordinate space. You must scale
//     pixel coordinates: scaledX = x * 65535 / screenWidth
//
// macOS: CGEventSetLocation(event, CGPoint{x, y})
//   — macOS uses actual pixel coordinates (no scaling needed!)
//   — CGDisplayPixelsWide(CGMainDisplayID()) = GetSystemMetrics(SM_CXSCREEN)
// ─────────────────────────────────────────────────────────────────────────────
static int mousemoveabs(lua_State* L) {
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);

    double x = lua_tonumber(L, 1);
    double y = lua_tonumber(L, 2);

    if (!RobloxActive()) return 0;

    // macOS uses direct pixel coordinates — no 65535-scaling needed
    // Windows had: x = (x + windowOffset) * (65535 / screenWidth)
    // macOS has:   CGPoint directly accepts pixel coordinates
    CGPoint pos = CGPointMake(x, y);

    // kCGEventMouseMoved = absolute move to position (replaces MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE)
    CGEventRef ev = CGEventCreateMouseEvent(nullptr, kCGEventMouseMoved, pos, kCGMouseButtonLeft);
    CGEventPost(kCGAnnotatedSessionEventTap, ev);
    CFRelease(ev);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// mousemoverel — relative mouse movement
//
// Windows: mouse_event(MOUSEEVENTF_MOVE, dx, dy, 0, 0)
//   — dx, dy are pixel deltas relative to current position
//
// macOS: Get current position, add delta, post kCGEventMouseMoved
//   — No direct "relative move" event; compute new absolute position instead
// ─────────────────────────────────────────────────────────────────────────────
static int mousemoverel(lua_State* L) {
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);

    double dx = lua_tonumber(L, 1);
    double dy = lua_tonumber(L, 2);

    if (!RobloxActive()) return 0;

    CGPoint current = getCurrentCursorPosition();
    CGPoint newPos  = CGPointMake(current.x + dx, current.y + dy);

    CGEventRef ev = CGEventCreateMouseEvent(nullptr, kCGEventMouseMoved, newPos, kCGMouseButtonLeft);
    CGEventPost(kCGAnnotatedSessionEventTap, ev);
    CFRelease(ev);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// mousescroll
//
// Windows: mouse_event(MOUSEEVENTF_WHEEL, 0, 0, amount, 0)
//   — amount in WHEEL_DELTA units (120 = one notch)
//
// macOS: CGEventCreateScrollWheelEvent2(source, unit, wheelCount, ...)
//   — kCGScrollEventUnitLine = scroll by lines (vs kCGScrollEventUnitPixel)
//   — wheelCount = number of scroll wheels (usually 1 for vertical)
// ─────────────────────────────────────────────────────────────────────────────
static int mousescroll(lua_State* L) {
    luaL_checktype(L, 1, LUA_TNUMBER);
    int32_t amt = (int32_t)lua_tointeger(L, 1);

    if (!RobloxActive()) return 0;

    // CGEventCreateScrollWheelEvent2 replaces mouse_event(MOUSEEVENTF_WHEEL)
    // Wheel 1 = vertical scroll, wheel 2 = horizontal scroll
    CGEventRef ev = CGEventCreateScrollWheelEvent(
        nullptr,                    // source
        kCGScrollEventUnitLine,     // unit (LINE ≈ Windows WHEEL_DELTA lines)
        1,                          // wheelCount
        amt                         // wheel1 delta (vertical)
    );
    CGEventPost(kCGAnnotatedSessionEventTap, ev);
    CFRelease(ev);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// messagebox
//
// Windows: MessageBoxA(nullptr, text, caption, type) → returns int (IDOK, IDCANCEL etc.)
//
// macOS: NSAlert with runModal → returns NSAlertFirstButtonReturn etc.
//
// NSAlert runs on the main thread. If called from a background thread
// (as Luau scripts typically run), you must dispatch to the main queue.
// This is analogous to SendMessage vs PostMessage on Windows.
// ─────────────────────────────────────────────────────────────────────────────
static int messagebox(lua_State* LS) {
    const auto text    = luaL_checkstring(LS, 1);
    const auto caption = luaL_checkstring(LS, 2);
    const auto type    = luaL_checkinteger(LS, 3);

    // Use the same yielder pattern as the Windows version (async execution)
    return Yielder->YielderExecution(LS,
        [text, caption, type]() -> auto {
            __block NSInteger result = 0;

            // NSAlert must run on the main thread
            // Windows equivalent: SendMessage(hwnd, ...) from background thread
            dispatch_sync(dispatch_get_main_queue(), ^{
                NSAlert* alert = [[NSAlert alloc] init];
                [alert setMessageText:[NSString stringWithUTF8String:caption]];
                [alert setInformativeText:[NSString stringWithUTF8String:text]];

                // Map Windows MB_* type flags to NSAlert buttons
                // MB_OK=0, MB_OKCANCEL=1, MB_YESNO=4
                if (type & 0x1) [alert addButtonWithTitle:@"Cancel"]; // MB_OKCANCEL
                if (type & 0x4) {                                       // MB_YESNO
                    [alert addButtonWithTitle:@"Yes"];
                    [alert addButtonWithTitle:@"No"];
                } else {
                    [alert addButtonWithTitle:@"OK"];
                }

                result = [alert runModal];
            });

            // Map NSAlertFirstButtonReturn (1000) back to Windows IDOK (1) style
            int windowsStyleResult = (int)(result - NSAlertFirstButtonReturn) + 1;

            return [windowsStyleResult](lua_State* L) -> int {
                lua_pushinteger(L, windowsStyleResult);
                return 1;
            };
        }
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// isrbxactive
// IDENTICAL logic — just calls our macOS RobloxActive() instead of Win32
// ─────────────────────────────────────────────────────────────────────────────
static int isrbxactive(lua_State* L) {
    lua_pushboolean(L, RobloxActive());
    return 1;
}

// ─────────────────────────────────────────────────────────────────────────────
// CInput::InitLib — IDENTICAL registrations
// ─────────────────────────────────────────────────────────────────────────────
void CInput::InitLib(lua_State* L) {
    declare__Global(L, "isrbxactive",  isrbxactive);
    declare__Global(L, "isgameactive", isrbxactive);
    declare__Global(L, "keypress",     keypress);
    declare__Global(L, "keytap",       keytap);
    declare__Global(L, "keyrelease",   keyrelease);
    declare__Global(L, "mouse1click",  mouse1click);
    declare__Global(L, "mouse1press",  mouse1press);
    declare__Global(L, "mouse1release",mouse1release);
    declare__Global(L, "mouse2click",  mouse2click);
    declare__Global(L, "mouse2press",  mouse2press);
    declare__Global(L, "mouse2release",mouse2release);
    declare__Global(L, "mousemoveabs", mousemoveabs);
    declare__Global(L, "mousemoverel", mousemoverel);
    declare__Global(L, "mousescroll",  mousescroll);
    declare__Global(L, "messagebox",   messagebox);
}
