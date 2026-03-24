// Miscellaneous/Communications/Communications.cpp — macOS port
//
// This file has the FEWEST changes of any platform-specific file.
// BSD sockets (the macOS/POSIX socket API) predate Winsock and were its
// direct inspiration. The functions are literally the same:
//   socket(), bind(), listen(), accept(), recv(), send(), ntohl(), htons()
//
// The ONLY differences are:
//
//   REMOVED (Windows-only):
//     #include <winsock2.h>           → not needed, BSD sockets are built-in
//     #pragma comment(lib,"Ws2_32.lib") → not needed
//     WSAStartup(MAKEWORD(2,2), &wsaData) → not needed (macOS sockets need no init)
//     WSACleanup()                    → not needed
//     SOCKET type                     → use int (Unix file descriptor)
//     INVALID_SOCKET                  → use -1
//     SOCKET_ERROR                    → use -1 (return value check)
//     closesocket(fd)                 → use close(fd)
//
//   ADDED (POSIX):
//     #include <sys/socket.h>         → socket(), bind(), listen(), accept()
//     #include <netinet/in.h>         → sockaddr_in, INADDR_ANY
//     #include <arpa/inet.h>          → ntohl(), htons()
//     #include <unistd.h>             → close() — replaces closesocket()
//
// The key educational insight: Winsock2's API was designed to be source-
// compatible with BSD sockets. Microsoft added WSAStartup/WSACleanup as a
// Windows initialization layer, but every other function (socket, bind,
// listen, accept, recv, send, ntohl, htons) has the same name and same
// signature on both platforms.

#pragma once
#include "Communications.hpp"
#include <sys/socket.h>     // socket, bind, listen, accept, recv
#include <netinet/in.h>     // sockaddr_in, INADDR_ANY, IPPROTO_TCP
#include <arpa/inet.h>      // ntohl, htons
#include <unistd.h>         // close() — replaces closesocket()
#include <netdb.h>          // getaddrinfo, freeaddrinfo

inline void(*luaprintf)(std::int32_t, const char*, ...) =
    reinterpret_cast<void(*)(std::int32_t, const char*, ...)>(ROBLOX::Print);

#define PORT 2304

// ---------------------------------------------------------------------------
// recvAll — IDENTICAL to Windows version.
// recv() has the same signature on both platforms.
// The only difference: on Windows the socket is SOCKET type; here it's int.
// ---------------------------------------------------------------------------
static bool recvAll(int sock, char* buffer, int totalBytes) {
    int received = 0;
    while (received < totalBytes) {
        int result = recv(sock, buffer + received, totalBytes - received, 0);
        if (result <= 0) return false;
        received += result;
    }
    return true;
}

// ---------------------------------------------------------------------------
// ServerThread — TCP server on port 2304
//
// WINDOWS original flow:
//   WSAStartup(...)                   ← Windows-only init
//   getaddrinfo(nullptr, "2304", ...) ← same on macOS
//   socket(AF_INET, SOCK_STREAM, TCP) ← same on macOS
//   bind(ListenSocket, ...)           ← same on macOS
//   listen(ListenSocket, SOMAXCONN)   ← same on macOS
//   accept(ListenSocket, ...)         ← same on macOS
//   recv(ClientSocket, ...)           ← same on macOS
//   closesocket(ClientSocket)         ← macOS: close(ClientSocket)
//   WSACleanup()                      ← Windows-only cleanup, REMOVED
//
// macOS version: identical logic, just drop WSA calls and use close().
// ---------------------------------------------------------------------------
static void ServerThread() {
    // NO WSAStartup needed — BSD sockets are always available on macOS

    struct addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = AI_PASSIVE;

    struct addrinfo* result = nullptr;
    char portStr[8];
    snprintf(portStr, sizeof(portStr), "%d", PORT);

    if (getaddrinfo(nullptr, portStr, &hints, &result) != 0) {
        return; // NO WSACleanup needed
    }

    // socket() — identical signature on Windows and macOS
    int ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket < 0) {             // macOS: -1  (Windows: INVALID_SOCKET)
        freeaddrinfo(result);
        return;
    }

    // SO_REUSEADDR — allows rebinding quickly after restart (same on both platforms)
    int reuse = 1;
    setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // bind() — identical
    if (bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen) < 0) {  // macOS: < 0 (Windows: SOCKET_ERROR)
        close(ListenSocket);           // macOS: close()  (Windows: closesocket())
        freeaddrinfo(result);
        return;
    }

    freeaddrinfo(result);

    // listen() — identical
    if (listen(ListenSocket, SOMAXCONN) < 0) {
        close(ListenSocket);
        return;
    }

    luaprintf(1, "[COBALT->TCP] Listening on port %d...", PORT);

    while (true) {
        // accept() — identical. Returns int on macOS, SOCKET (uintptr_t) on Windows.
        int ClientSocket = accept(ListenSocket, nullptr, nullptr);
        if (ClientSocket < 0) continue;   // macOS: < 0 (Windows: INVALID_SOCKET)

        // Receive 4-byte big-endian script size (identical framing protocol)
        uint32_t scriptSizeNet = 0;
        if (!recvAll(ClientSocket, reinterpret_cast<char*>(&scriptSizeNet), 4)) {
            close(ClientSocket);
            continue;
        }

        // ntohl() — identical function, identical header path difference
        // Windows: <winsock2.h>  |  macOS: <arpa/inet.h>
        uint32_t scriptSize = ntohl(scriptSizeNet);
        if (scriptSize == 0 || scriptSize > 10 * 1024 * 1024) {
            close(ClientSocket);
            continue;
        }

        std::vector<char> buffer(scriptSize + 1, 0);
        if (!recvAll(ClientSocket, buffer.data(), scriptSize)) {
            close(ClientSocket);
            continue;
        }

        std::string receivedScript(buffer.data(), scriptSize);
        if (Globals::ExecutorThread) {
            Execution->SendScript(Globals::ExecutorThread, receivedScript);
        } else {
            luaprintf(3, "[COBALT->TCP] Failed to execute: no valid lua_State");
        }

        // close() replaces closesocket() — only difference in this entire function
        close(ClientSocket);
    }

    close(ListenSocket);
    // NO WSACleanup() needed
}

void CCommunications::Initialize() {
    ServerThread();
}
