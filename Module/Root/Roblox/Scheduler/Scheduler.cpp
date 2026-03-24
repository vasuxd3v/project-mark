// Roblox/Scheduler/Scheduler.cpp — macOS port
//
// ONE function changes: IsValidPointer()
//   Windows: VirtualQuery() + MEMORY_BASIC_INFORMATION
//   macOS:   vm_region_64() + vm_region_basic_info_data_64_t
//
// All other functions are identical (pure pointer arithmetic / Luau VM calls).
//
// ─── IsValidPointer — the core change ────────────────────────────────────────
//
// WINDOWS:
//   MEMORY_BASIC_INFORMATION mbi;
//   if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) return false;
//   return (mbi.State == MEM_COMMIT) &&
//          ((mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | ...)) != 0) &&
//          (start >= regionStart && end <= regionEnd);
//
// MACOS:
//   vm_region_basic_info_data_64_t info;
//   kern_return_t kr = vm_region_64(mach_task_self(), &addr, &vmsize,
//                         VM_REGION_BASIC_INFO_64, (vm_region_info_t)&info,
//                         &count, &object);
//   return (kr == KERN_SUCCESS) &&
//          (info.protection & VM_PROT_READ) &&
//          (ptr + size <= addr + vmsize);
//
// Concept mapping:
//   VirtualQuery(ptr, ...)         → vm_region_64(mach_task_self(), &addr, ...)
//   mbi.State == MEM_COMMIT        → kr == KERN_SUCCESS (if it succeeds, region exists)
//   mbi.Protect & PAGE_READONLY    → info.protection & VM_PROT_READ
//   mbi.Protect & PAGE_READWRITE   → info.protection & (VM_PROT_READ | VM_PROT_WRITE)
//   mbi.Protect & PAGE_EXECUTE_*   → info.protection & VM_PROT_EXECUTE
//   mbi.BaseAddress + RegionSize   → addr (modified by vm_region_64) + vmsize
//   GetCurrentProcess()            → mach_task_self()
//
// NOTE on vm_region_64's addr parameter:
//   Unlike VirtualQuery (which does NOT modify the input pointer), vm_region_64
//   modifies `addr` in-place to the START of the region containing `ptr`.
//   This means addr after the call is analogous to mbi.BaseAddress.

#pragma once
#include "Scheduling.hpp"
#include "Includes.hpp"
#include "Darwin/MemoryRegion.hpp"  // Darwin::IsReadable wraps vm_region_64
#include <mach/mach.h>
#include <mach/vm_map.h>

std::vector<uintptr_t> Jobs;
uintptr_t Address = 0;

// IDENTICAL to Windows version — pure Luau VM struct access, no OS APIs
void CScheduler::SetProtoCaps(Proto* Proto, uintptr_t* Capabilities)
{
    Proto->userdata = Capabilities;
    for (int i = 0; i < Proto->sizep; ++i)
        SetProtoCaps(Proto->p[i], Capabilities);
}

// IDENTICAL to Windows version
void CScheduler::SetThreadCaps(lua_State* L, int Identity, uintptr_t Capabilities)
{
    L->userdata->Identity     = Identity;
    L->userdata->Capabilities = Capabilities;
}

// IDENTICAL — pure pointer arithmetic using REBASE offsets
uintptr_t CScheduler::DataModel()
{
    uintptr_t FakeDM = *reinterpret_cast<uintptr_t*>(Offsets::External::TaskScheduler::FakeDMPointer);
    uintptr_t DM     = *reinterpret_cast<uintptr_t*>(FakeDM + Offsets::External::TaskScheduler::FakeDMtoDM);
    return DM;
}

// IDENTICAL
uintptr_t CScheduler::ScriptContext(uintptr_t DataModel)
{
    uintptr_t Children = *reinterpret_cast<uintptr_t*>(DataModel + Offsets::External::DataModel::Children);
    uintptr_t SC       = *reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(Children) + Offsets::External::TaskScheduler::ScriptContext);
    return SC;
}

// IDENTICAL — calling a function pointer resolved at runtime
lua_State* CScheduler::LuaState(uintptr_t ScriptContext)
{
    uintptr_t dataModel = Scheduler->DataModel();
    uintptr_t StateFn   = REBASE(0xD7BA30);
    using decrypt_state_t = int64_t(*)(int64_t, uint64_t*, uint64_t*);
    auto GetState = reinterpret_cast<decrypt_state_t>(StateFn);
    uint64_t a2 = 0, a3 = 0;
    int64_t  scriptContext = static_cast<int64_t>(ScriptContext);
    int64_t  state = GetState(scriptContext, &a2, &a3);
    return reinterpret_cast<lua_State*>(state);
}

// ─────────────────────────────────────────────────────────────────────────────
// IsValidPointer — THE CHANGED FUNCTION
//
// Windows used VirtualQuery from <memoryapi.h> (via Windows.h):
//   VirtualQuery(ptr, &mbi, sizeof(mbi))
//   Checks mbi.State == MEM_COMMIT, checks page protection flags
//
// macOS uses vm_region_64 from <mach/vm_map.h>:
//   vm_region_64(mach_task_self(), &addr, &size, VM_REGION_BASIC_INFO_64, ...)
//   Checks kern_return_t == KERN_SUCCESS, checks info.protection flags
//
// We use Darwin::IsReadable() from MemoryRegion.hpp which wraps this cleanly.
// ─────────────────────────────────────────────────────────────────────────────
bool CScheduler::IsValidPointer(void* ptr, size_t size) {
    if (!ptr) return false;

    // -----------------------------------------------------------------------
    // macOS vm_region_64 approach — direct equivalent of VirtualQuery
    // -----------------------------------------------------------------------
    vm_address_t address = (vm_address_t)ptr;
    vm_size_t    vmsize  = 0;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object;

    // vm_region_64 replaces VirtualQuery.
    // mach_task_self() replaces the implicit current-process handle.
    // KERN_SUCCESS replaces "return value > 0" check of VirtualQuery.
    kern_return_t kr = vm_region_64(
        mach_task_self(),          // task port (= current process)
        &address,                  // in: query ptr; out: region start (= mbi.BaseAddress)
        &vmsize,                   // out: region size (= mbi.RegionSize)
        VM_REGION_BASIC_INFO_64,   // flavor
        (vm_region_info_t)&info,   // out: protection info
        &count,
        &object
    );

    if (kr != KERN_SUCCESS) return false;  // region not mapped (= mbi.State != MEM_COMMIT)

    // VM_PROT_READ = readable (replaces PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ)
    if (!(info.protection & VM_PROT_READ)) return false;

    // Check that [ptr, ptr+size) falls within the region
    uintptr_t start      = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t end        = start + size;
    uintptr_t regionStart = static_cast<uintptr_t>(address);  // vm_region_64 updated address
    uintptr_t regionEnd   = regionStart + static_cast<uintptr_t>(vmsize);

    return (start >= regionStart && end <= regionEnd);
}

// IDENTICAL
int64_t CScheduler::GetPlaceID() {
    int64_t DataModel = CScheduler::DataModel();
    if (!DataModel) return 0;
    auto PlaceIdPtr = reinterpret_cast<int64_t*>(DataModel + Offsets::External::TaskScheduler::PlaceId);
    if (!PlaceIdPtr) return 0;
    return *PlaceIdPtr;
}

// IDENTICAL
bool CScheduler::GameIsLoaded(uintptr_t DataModel) {
    uintptr_t GM = *(uintptr_t*)(DataModel + Offsets::External::DataModel::GameLoaded);
    return GM == 31;
}

// IDENTICAL
void CScheduler::UpdateJobs() {
    Jobs.clear();
    uintptr_t JobsStart = *(uintptr_t*)(Address + Offsets::External::TaskScheduler::JobStart);
    uintptr_t JobsEnd   = *(uintptr_t*)(Address + Offsets::External::TaskScheduler::JobStart + sizeof(void*));
    for (auto i = JobsStart; i < JobsEnd; i += 0x10) {
        uintptr_t Job = *(uintptr_t*)i;
        if (!Job) continue;
        std::string* JobName = reinterpret_cast<std::string*>(Job + Offsets::External::TaskScheduler::JobEnd);
        if (JobName && JobName->length() > 0) Jobs.push_back(Job);
    }
}

// IDENTICAL
uintptr_t CScheduler::GetJobByName(const std::string& Name) {
    for (auto Job : Jobs)
        if (*(std::string*)(Job + Offsets::External::TaskScheduler::JobName) == Name)
            return Job;
    return 0;
}
