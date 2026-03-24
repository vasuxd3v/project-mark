// Roblox/Bypassing/ThreadPool.hpp — macOS port
//
// WINDOWS ORIGINAL used the Windows Thread Pool API:
//   CreateThreadpoolWork(callback, data, nullptr) → PTP_WORK
//   SubmitThreadpoolWork(work)
//   CloseThreadpoolWork(work)         ← called inside callback to self-destruct
//   PTP_CALLBACK_INSTANCE             ← passed to callback but unused here
//   PTP_WORK                          ← opaque handle type
//
// MACOS EQUIVALENT uses Grand Central Dispatch (GCD):
//   dispatch_get_global_queue(priority, 0) → dispatch_queue_t
//   dispatch_async(queue, ^{ ... })          → submits block to thread pool
//
// WHY GCD?
//   GCD is Apple's high-level concurrency framework built into macOS/iOS.
//   It manages a thread pool automatically (same as the Windows thread pool).
//   dispatch_get_global_queue() returns a shared concurrent queue backed by
//   the system thread pool — equivalent to using the default thread pool in
//   CreateThreadpoolWork with no custom PTP_POOL.
//
//   dispatch_async() is non-blocking: it enqueues the block and returns
//   immediately, just like SubmitThreadpoolWork().
//
// The Objective-C block (^{ ... }) syntax is the macOS closure mechanism.
// In C++ files you can use it with -fblocks (Clang flag). In .mm files it
// works natively. This replaces the function pointer callback pattern of
// the Windows Thread Pool API.
//
// DISPATCH_QUEUE_PRIORITY_DEFAULT corresponds to the default Windows thread
// pool priority. GCD also has HIGH and LOW priority variants.

#pragma once
#include <functional>
#include <memory>
#include <thread>
#include <dispatch/dispatch.h>   // GCD — replaces Windows threadpool headers

template <typename T>
struct STraits;

template <typename R, typename... Args>
struct STraits<R(Args...)> {
    using RetType = R;
};

struct SInfo {
    std::function<void()> Task;
};

class CThreadPool {
public:
    template<typename T, typename... Args>
    bool Run(T Callback, Args&&... Arguments) {
        // Allocate work item on the heap (same as Windows version using make_unique)
        auto ThreadInfo = std::make_unique<SInfo>(SInfo{
            [Callback, Arguments...]() { Callback(Arguments...); }
        });
        SInfo* raw = ThreadInfo.release();

        // -----------------------------------------------------------------------
        // Windows original:
        //   auto Function = [](PTP_CALLBACK_INSTANCE, PVOID DataInfo, PTP_WORK Work) {
        //       auto Data = static_cast<SInfo*>(DataInfo);
        //       try { Data->Task(); } catch (...) {}
        //       CloseThreadpoolWork(Work);
        //       delete Data;
        //   };
        //   const auto Work = CreateThreadpoolWork(Function, raw, nullptr);
        //   if (!Work) return false;
        //   SubmitThreadpoolWork(Work);
        //
        // macOS equivalent:
        //   dispatch_async submits an Objective-C block to the global queue.
        //   The block captures `raw` and cleans up after itself — same as the
        //   Windows callback deleting Data and calling CloseThreadpoolWork.
        //
        // dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0):
        //   Returns the system-wide concurrent queue for normal-priority work.
        //   Equivalent to using the default Windows thread pool (no custom pool).
        // -----------------------------------------------------------------------
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            try {
                raw->Task();
            } catch (const std::exception&) {
                // Same silent catch as the Windows version
            }
            delete raw;  // CloseThreadpoolWork equivalent — cleanup after execution
        });

        return true;
    }
};

inline auto ThreadPool = std::make_unique<CThreadPool>();
