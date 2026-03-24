// Common/Logger/Logger.cpp — macOS port
//
// WINDOWS ORIGINAL used a Named Pipe:
//   CreateFileA(R"(\\.\pipe\LoggerInformationPipe)", GENERIC_WRITE, ...)
//   WaitNamedPipeA(...)
//   WriteFile(hPipe, buffer, len, &bytesWritten, nullptr)
//
// MACOS EQUIVALENT uses a UNIX Domain Socket:
//   socket(AF_UNIX, SOCK_STREAM, 0)
//   connect(fd, (sockaddr*)&addr, sizeof(addr))
//   write(fd, buffer, len)
//
// WHY UNIX DOMAIN SOCKETS?
//   Windows Named Pipes and UNIX Domain Sockets are the same concept:
//   both provide local IPC (inter-process communication) using a path
//   in the filesystem namespace (Windows: \\.\pipe\Name, macOS: /tmp/name).
//   Both are stream-oriented (SOCK_STREAM = byte-stream, like a pipe).
//   Both work transparently between processes on the same machine.
//
// Key API differences:
//   Windows                          macOS
//   ─────────────────────────────────────────────────────────────
//   CreateFileA("\\.\pipe\...")      socket(AF_UNIX, SOCK_STREAM, 0)
//   WaitNamedPipeA(path, timeout)    retry connect() in a loop
//   WriteFile(handle, buf, len, ...)  write(fd, buf, len)
//   CloseHandle(handle)              close(fd)
//   INVALID_HANDLE_VALUE             -1 (invalid fd)
//   HANDLE hPipe                     int g_logSocket
//
// The socket path "/tmp/CobaltLoggerPipe" replaces "\\.\pipe\LoggerInformationPipe"

#include "Logger.hpp"
#include <sys/socket.h>     // socket, connect
#include <sys/un.h>         // sockaddr_un — UNIX domain socket address
#include <unistd.h>         // write, close
#include <cstdio>
#include <cstdarg>
#include <cstring>

namespace Logger {
    std::ofstream       logFile;
    std::filesystem::path dllDir;
}

// ---------------------------------------------------------------------------
// g_logSocket — replaces HANDLE hPipe = INVALID_HANDLE_VALUE
// A UNIX domain socket file descriptor. -1 means not connected (same as
// INVALID_HANDLE_VALUE meaning the pipe isn't open).
// ---------------------------------------------------------------------------
static int g_logSocket = -1;

// The socket path replaces the named pipe path.
// Windows: \\.\pipe\LoggerInformationPipe
// macOS:   /tmp/CobaltLoggerPipe
static const char* SOCKET_PATH = "/tmp/CobaltLoggerPipe";

// ---------------------------------------------------------------------------
// Logger::Init() — background connection thread
//
// Windows original:
//   std::thread([] {
//       while (true) {
//           hPipe = CreateFileA(R"(\\.\pipe\...)", GENERIC_WRITE, ...);
//           if (hPipe != INVALID_HANDLE_VALUE) break;
//           if (error == ERROR_PIPE_BUSY)
//               WaitNamedPipeA(path, 5000);
//           else break;
//       }
//   }).detach();
//
// macOS: same pattern — retry connect() in a loop until a server is listening.
// The external "logger server" would be a separate process that calls
// bind() + listen() + accept() on the same socket path.
// ---------------------------------------------------------------------------
void Logger::Init() {
    std::thread([] {
        for (int attempt = 0; attempt < 10; attempt++) {
            // Create a UNIX domain socket (replaces CreateFileA for a named pipe)
            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd < 0) break;

            // Set up the address — path-based, just like a named pipe path
            struct sockaddr_un addr{};
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

            // connect() replaces WaitNamedPipeA + CreateFileA open sequence
            if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                g_logSocket = fd;
                return; // connected successfully
            }

            close(fd);
            // Wait 1 second before retrying (WaitNamedPipeA used a 5s timeout)
            sleep(1);
        }
        // Failed to connect — Logger::printf will fall back to stderr
    }).detach();
}

// ---------------------------------------------------------------------------
// Logger::printf() — sends a formatted message
//
// Windows original:
//   WriteFile(hPipe, buffer, (DWORD)strlen(buffer), &bytesWritten, nullptr)
//
// macOS:
//   write(g_logSocket, buffer, strlen(buffer))
//
// write() is a POSIX syscall — no DWORD, no BOOL, just ssize_t return.
// If the socket isn't connected, fall back to stderr (same as the Windows
// version silently returning when hPipe == INVALID_HANDLE_VALUE).
// ---------------------------------------------------------------------------
void Logger::printf(const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer) - 2, fmt, args);
    va_end(args);

    // Append newline (strcat_s on Windows, strncat on macOS/POSIX)
    size_t len = strlen(buffer);
    if (len < sizeof(buffer) - 2) {
        buffer[len]     = '\n';
        buffer[len + 1] = '\0';
        len++;
    }

    if (g_logSocket >= 0) {
        // write() replaces WriteFile() — same semantics, no DWORD needed
        ssize_t written = write(g_logSocket, buffer, len);
        if (written <= 0) {
            // Socket closed — fall through to stderr
            close(g_logSocket);
            g_logSocket = -1;
        }
    } else {
        // No pipe/socket connected — print to stderr
        // (Windows version silently returned; stderr is more useful for debugging)
        ::fprintf(stderr, "[Cobalt] %s", buffer);
    }
}
