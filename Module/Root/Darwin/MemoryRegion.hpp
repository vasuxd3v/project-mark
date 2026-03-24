// Darwin/MemoryRegion.hpp
// macOS equivalent of Windows VirtualQuery / MEMORY_BASIC_INFORMATION
//
// Windows approach:
//   MEMORY_BASIC_INFORMATION mbi;
//   VirtualQuery(ptr, &mbi, sizeof(mbi));
//   if (mbi.State == MEM_COMMIT && mbi.Protect & PAGE_READWRITE) { ... }
//
// macOS approach:
//   Uses the Mach kernel VM API from <mach/vm_map.h>:
//   vm_region_64(mach_task_self(), &addr, &vmsize, VM_REGION_BASIC_INFO_64,
//                (vm_region_info_t)&info, &count, &object)
//
// Key concepts:
//   - mach_task_self() = the "task port" for the current process.
//     On Windows, the implicit process handle is used by VirtualQuery.
//     On macOS, all Mach calls require an explicit task port. For your own
//     process, mach_task_self() is the equivalent.
//
//   - VM_PROT_READ / VM_PROT_WRITE / VM_PROT_EXECUTE replace Windows'
//     PAGE_READONLY / PAGE_READWRITE / PAGE_EXECUTE_READ etc.
//
//   - kern_return_t (vs BOOL/DWORD): Mach calls return kern_return_t.
//     KERN_SUCCESS (== 0) means success; any other value is an error code.
//
//   - There is no MEM_COMMIT / MEM_FREE distinction in the same form.
//     If vm_region_64 succeeds, the region is mapped (committed equivalent).
//     If it fails, the address is not mapped.

#pragma once
#include <cstdint>
#include <mach/mach.h>       // mach_task_self, kern_return_t
#include <mach/vm_map.h>     // vm_region_64, vm_region_basic_info_data_64_t
#include <mach/vm_prot.h>    // VM_PROT_READ, VM_PROT_WRITE, VM_PROT_EXECUTE

namespace Darwin {

    // -----------------------------------------------------------------------
    // RegionInfo — mirrors the shape of MEMORY_BASIC_INFORMATION so call
    // sites don't need heavy rewrites.
    //
    // Windows MEMORY_BASIC_INFORMATION fields mapped:
    //   BaseAddress  -> BaseAddress   (start of the region vm_region_64 found)
    //   RegionSize   -> RegionSize    (byte size of the region)
    //   State        -> IsCommitted   (true if vm_region_64 succeeded)
    //   Protect      -> Protection    (VM_PROT_* flags)
    // -----------------------------------------------------------------------
    struct RegionInfo {
        uintptr_t BaseAddress;   // Start of the VM region
        size_t    RegionSize;    // Size in bytes
        int       Protection;   // VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE
        bool      IsCommitted;  // true when vm_region_64 succeeds (= MEM_COMMIT)
    };

    // -----------------------------------------------------------------------
    // QueryRegion
    // Windows equivalent: VirtualQuery(ptr, &mbi, sizeof(mbi))
    //
    // Returns true on success (region is mapped), false if the address is
    // not mapped into the process.
    //
    // Internal details:
    //   vm_region_64 modifies the `address` parameter to the actual start of
    //   the region (may be rounded down from ptr). This matches VirtualQuery
    //   which also returns the start of the region in mbi.BaseAddress.
    // -----------------------------------------------------------------------
    inline bool QueryRegion(void* ptr, RegionInfo& out) {
        vm_address_t address    = (vm_address_t)ptr;
        vm_size_t    vmsize     = 0;
        vm_region_basic_info_data_64_t info;
        mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t            object;

        kern_return_t kr = vm_region_64(
            mach_task_self(),        // task port for current process
            &address,                // in: query addr; out: region start
            &vmsize,                 // out: region size
            VM_REGION_BASIC_INFO_64, // flavor — basic protection info
            (vm_region_info_t)&info, // out: protection flags etc.
            &count,
            &object
        );

        if (kr != KERN_SUCCESS) {
            out = {};
            return false;
        }

        out.BaseAddress = (uintptr_t)address;
        out.RegionSize  = (size_t)vmsize;
        out.Protection  = info.protection;
        out.IsCommitted = true;
        return true;
    }

    // -----------------------------------------------------------------------
    // IsReadable
    // Windows equivalent:
    //   mbi.State == MEM_COMMIT &&
    //   (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | ...))
    //
    // macOS: check VM_PROT_READ bit and that ptr+size is within the region.
    // -----------------------------------------------------------------------
    inline bool IsReadable(void* ptr, size_t size = 1) {
        if (!ptr) return false;
        RegionInfo info;
        if (!QueryRegion(ptr, info)) return false;
        if (!(info.Protection & VM_PROT_READ)) return false;
        // Ensure the entire range [ptr, ptr+size) lies within the region
        uintptr_t start = (uintptr_t)ptr;
        uintptr_t end   = start + size;
        return start >= info.BaseAddress && end <= info.BaseAddress + info.RegionSize;
    }

    // -----------------------------------------------------------------------
    // IsWritable
    // Windows equivalent:
    //   mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_READWRITE)
    // -----------------------------------------------------------------------
    inline bool IsWritable(void* ptr, size_t size = 1) {
        if (!ptr) return false;
        RegionInfo info;
        if (!QueryRegion(ptr, info)) return false;
        if (!(info.Protection & VM_PROT_WRITE)) return false;
        uintptr_t start = (uintptr_t)ptr;
        uintptr_t end   = start + size;
        return start >= info.BaseAddress && end <= info.BaseAddress + info.RegionSize;
    }

    // -----------------------------------------------------------------------
    // IsExecutable
    // Windows equivalent:
    //   mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)
    // -----------------------------------------------------------------------
    inline bool IsExecutable(void* ptr, size_t size = 1) {
        if (!ptr) return false;
        RegionInfo info;
        if (!QueryRegion(ptr, info)) return false;
        if (!(info.Protection & VM_PROT_EXECUTE)) return false;
        uintptr_t start = (uintptr_t)ptr;
        uintptr_t end   = start + size;
        return start >= info.BaseAddress && end <= info.BaseAddress + info.RegionSize;
    }

} // namespace Darwin
