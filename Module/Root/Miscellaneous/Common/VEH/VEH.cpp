// Common/VEH/VEH.cpp — macOS port
//
// WINDOWS ORIGINAL:
//   SetUnhandledExceptionFilter(Alert)
//   Alert receives PEXCEPTION_POINTERS with ExceptionRecord + ContextRecord
//   ContextRecord holds CPU registers: Rax, Rbx, Rcx, Rdx, Rdi, Rsi, Rbp, Rsp, R8-R15
//   Thread safety via CRITICAL_SECTION (InitializeCriticalSection / EnterCriticalSection)
//   Crash dialog via MessageBox()
//   Returns EXCEPTION_CONTINUE_EXECUTION to suppress the crash
//
// MACOS EQUIVALENT:
//   sigaction() with SA_SIGINFO flag
//   Handler receives: int signo, siginfo_t* info, void* ucontext
//   siginfo_t::si_addr = fault address  (== ExceptionRecord->ExceptionAddress)
//   signo              = signal number  (== ExceptionRecord->ExceptionCode analogue)
//   ucontext_t::uc_mcontext holds CPU registers:
//     ARM64:  __darwin_arm_thread_state64  (__x[0-28], __sp, __lr, __pc, __cpsr)
//     x86_64: __darwin_x86_thread_state64  (__rax, __rbx, __rcx, ...)
//   Thread safety via pthread_mutex_t  (replaces CRITICAL_SECTION)
//   Crash output via fprintf(stderr, ...) + Logger::printf()
//
// Signal-to-exception mapping:
//   SIGSEGV  →  EXCEPTION_ACCESS_VIOLATION (0xC0000005) on Windows
//   SIGBUS   →  EXCEPTION_DATATYPE_MISALIGNMENT (0x80000002)
//   SIGABRT  →  abort() / __fastfail equivalent
//   SIGILL   →  EXCEPTION_ILLEGAL_INSTRUCTION (0xC000001D)
//   SIGFPE   →  EXCEPTION_FLT_DIVIDE_BY_ZERO / INT_DIVIDE_BY_ZERO

#include "VEH.hpp"
#include <signal.h>         // sigaction, siginfo_t, SA_SIGINFO
#include <ucontext.h>       // ucontext_t, mcontext_t
#include <pthread.h>        // pthread_mutex_t — replaces CRITICAL_SECTION
#include <sstream>

// ---------------------------------------------------------------------------
// g_crashMutex — replaces CRITICAL_SECTION from the Windows version.
//
// Windows:
//   CRITICAL_SECTION section;
//   InitializeCriticalSection(&section);
//   EnterCriticalSection(&section);
//   LeaveCriticalSection(&section);
//
// macOS:
//   pthread_mutex_t g_crashMutex = PTHREAD_MUTEX_INITIALIZER;
//   pthread_mutex_lock(&g_crashMutex);
//   pthread_mutex_unlock(&g_crashMutex);
//
// PTHREAD_MUTEX_INITIALIZER is a compile-time initializer (no init function
// needed), equivalent to InitializeCriticalSection but simpler to use.
// ---------------------------------------------------------------------------
static pthread_mutex_t g_crashMutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_handled = false;

// ---------------------------------------------------------------------------
// SignalHandler — replaces Alert(PEXCEPTION_POINTERS*)
//
// SA_SIGINFO gives us the 3-argument form:
//   signo  = signal number  (SIGSEGV=11, SIGBUS=10, SIGABRT=6, SIGILL=4)
//   info   = siginfo_t*     (contains si_addr = fault address)
//   ctx    = void*          (cast to ucontext_t* to access registers)
//
// Windows original read registers from:
//   exception_pointers->ContextRecord->Rax  etc.
//
// macOS reads them from:
//   uctx->uc_mcontext->__ss.__rax  (x86_64)
//   uctx->uc_mcontext->__ss.__x[0] (ARM64)
// ---------------------------------------------------------------------------
static void SignalHandler(int signo, siginfo_t* info, void* ctx) {
    // Thread-safety: only handle once, same logic as the Windows 'handled' bool
    pthread_mutex_lock(&g_crashMutex);
    if (g_handled) {
        pthread_mutex_unlock(&g_crashMutex);
        return;
    }
    g_handled = true;
    pthread_mutex_unlock(&g_crashMutex);

    ucontext_t* uctx = reinterpret_cast<ucontext_t*>(ctx);
    std::stringstream message;

    message << "An unexpected error has occurred!\n\n";
    message << "Signal: " << signo << " (" << strsignal(signo) << ")\n";
    message << "Fault address: " << info->si_addr << "\n\n";

#if defined(__aarch64__)
    // ARM64 (Apple Silicon) register dump
    // Windows had RAX-R15; ARM64 has X0-X28, SP, LR, PC
    auto& regs = uctx->uc_mcontext->__ss;
    for (int i = 0; i <= 28; i++)
        message << "X" << i << ": 0x" << std::hex << regs.__x[i] << "\n";
    message << "SP: 0x"  << std::hex << regs.__sp << "\n";
    message << "LR: 0x"  << std::hex << regs.__lr << "\n";
    message << "PC: 0x"  << std::hex << regs.__pc << "\n";

#elif defined(__x86_64__)
    // x86_64 (Intel Mac) register dump
    // Direct equivalent of the Windows register dump in the original VEH.cpp
    auto& regs = uctx->uc_mcontext->__ss;
    message << "RAX: 0x" << std::hex << regs.__rax << "\n";
    message << "RBX: 0x" << std::hex << regs.__rbx << "\n";
    message << "RCX: 0x" << std::hex << regs.__rcx << "\n";
    message << "RDX: 0x" << std::hex << regs.__rdx << "\n";
    message << "RDI: 0x" << std::hex << regs.__rdi << "\n";
    message << "RSI: 0x" << std::hex << regs.__rsi << "\n";
    message << "RBP: 0x" << std::hex << regs.__rbp << "\n";
    message << "RSP: 0x" << std::hex << regs.__rsp << "\n";
    message << "R8:  0x" << std::hex << regs.__r8  << "\n";
    message << "R9:  0x" << std::hex << regs.__r9  << "\n";
    message << "R10: 0x" << std::hex << regs.__r10 << "\n";
    message << "R11: 0x" << std::hex << regs.__r11 << "\n";
    message << "R12: 0x" << std::hex << regs.__r12 << "\n";
    message << "R13: 0x" << std::hex << regs.__r13 << "\n";
    message << "R14: 0x" << std::hex << regs.__r14 << "\n";
    message << "R15: 0x" << std::hex << regs.__r15 << "\n";
#endif

    message << "\nPlease report this to the developer\n";

    // Windows used: MessageBox(0, message.str().c_str(), "Cobalt Crash", MB_ICONERROR)
    // macOS: write to stderr (visible in Console.app / log stream / Xcode)
    fprintf(stderr, "[Cobalt Crash]\n%s\n", message.str().c_str());
    Logger::printf("[Cobalt Crash] Signal %d at %p\n%s",
                   signo, info->si_addr, message.str().c_str());

    // Windows returned EXCEPTION_CONTINUE_EXECUTION.
    // On macOS we can't "continue" a segfault — the best we can do is re-raise
    // with the default handler so the process terminates cleanly.
    // (SA_RESETHAND in Start() automatically restores the default handler.)
}

// ---------------------------------------------------------------------------
// CVEH::Start() — registers signal handlers
//
// Windows original:
//   SetUnhandledExceptionFilter(Alert);
//
// macOS: sigaction() with SA_SIGINFO gives us the crash address and context.
//   struct sigaction sa;
//   sa.sa_sigaction = handler;
//   sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
//
// SA_SIGINFO  — enables the 3-argument handler form (gives us siginfo_t)
// SA_RESETHAND — resets to default after first invocation (prevents re-entry)
//               equivalent of the 'handled' bool guard in the Windows version
// ---------------------------------------------------------------------------
auto CVEH::Start() -> bool {
    struct sigaction sa{};
    sa.sa_sigaction = SignalHandler;
    sa.sa_flags     = SA_SIGINFO | SA_RESETHAND;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV, &sa, nullptr);   // access violation   → EXCEPTION_ACCESS_VIOLATION
    sigaction(SIGBUS,  &sa, nullptr);   // misaligned access  → EXCEPTION_DATATYPE_MISALIGNMENT
    sigaction(SIGABRT, &sa, nullptr);   // abort              → abnormal termination
    sigaction(SIGILL,  &sa, nullptr);   // illegal instruction → EXCEPTION_ILLEGAL_INSTRUCTION
    sigaction(SIGFPE,  &sa, nullptr);   // FP exception        → EXCEPTION_FLT_*

    return true;
}
