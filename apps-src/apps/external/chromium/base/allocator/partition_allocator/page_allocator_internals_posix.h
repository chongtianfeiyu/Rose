// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ALLOCATOR_PARTITION_ALLOCATOR_PAGE_ALLOCATOR_INTERNALS_POSIX_H_
#define BASE_ALLOCATOR_PARTITION_ALLOCATOR_PAGE_ALLOCATOR_INTERNALS_POSIX_H_

#include <errno.h>
#include <sys/mman.h>

#include "base/allocator/partition_allocator/partition_alloc_check.h"
#include "base/check_op.h"
#include "base/notreached.h"
#include "base/posix/eintr_wrapper.h"
#include "build/build_config.h"

#if defined(OS_APPLE)
#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"

#include <Availability.h>
#include <Security/Security.h>
#include <mach/mach.h>
#endif
#if defined(OS_ANDROID)
#include <sys/prctl.h>
#endif
#if defined(OS_LINUX) || defined(OS_CHROMEOS)
#include <sys/resource.h>

#include <algorithm>
#endif

#include "base/allocator/partition_allocator/page_allocator.h"

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#if defined(OS_APPLE)

// SecTaskGetCodeSignStatus is marked as unavailable on macOS, although it’s
// available on iOS and other Apple operating systems. It is, in fact, present
// on the system since macOS 10.12.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wavailability"
uint32_t SecTaskGetCodeSignStatus(SecTaskRef task)
#if __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_10_12
    // When redeclaring something previously declared as unavailable, the
    // weak_import attribute won’t be applied unless manually set.
    __attribute__((weak_import))
#endif  // DT < 10.12
    API_AVAILABLE(macos(10.12));
#pragma clang diagnostic pop

#endif  // OS_APPLE

namespace base {

namespace {

#if defined(OS_ANDROID)
const char* PageTagToName(PageTag tag) {
  // Important: All the names should be string literals. As per prctl.h in
  // //third_party/android_ndk the kernel keeps a pointer to the name instead
  // of copying it.
  //
  // Having the name in .rodata ensures that the pointer remains valid as
  // long as the mapping is alive.
  switch (tag) {
    case PageTag::kBlinkGC:
      return "blink_gc";
    case PageTag::kPartitionAlloc:
      return "partition_alloc";
    case PageTag::kChromium:
      return "chromium";
    case PageTag::kV8:
      return "v8";
    default:
      PA_DCHECK(false);
      return "";
  }
}
#endif  // defined(OS_ANDROID)

#if defined(OS_APPLE)
// Tests whether the version of macOS supports the MAP_JIT flag and if the
// current process is signed with the hardened runtime and the allow-jit
// entitlement, returning whether MAP_JIT should be used to allocate regions
// that will contain JIT-compiled executable code.
bool UseMapJit() {
  if (!mac::IsAtLeastOS10_14()) {
    // MAP_JIT existed before macOS 10.14, but had somewhat different semantics.
    // Only one MAP_JIT region was permitted per process, but calling code here
    // will very likely require more than one such region. Since MAP_JIT is not
    // strictly necessary to write code to a region and then execute it on these
    // older OSes, don’t use it at all.
    return false;
  }

  // Until determining that the hardened runtime is enabled, early returns will
  // return true, so that MAP_JIT will be used. This is important on arm64,
  // which only allows pages to be simultaneously writable and executable when
  // in a region allocated with MAP_JIT, regardless of code signing options. On
  // arm64, an attempt to set a non-MAP_JIT page as simultaneously writable and
  // executable fails with EPERM. Although this is not enforced on x86_64,
  // MAP_JIT is harmless in that case.

  ScopedCFTypeRef<SecTaskRef> task(SecTaskCreateFromSelf(kCFAllocatorDefault));
  if (!task) {
    return true;
  }

  uint32_t flags = SecTaskGetCodeSignStatus(task);
  if (!(flags & kSecCodeSignatureRuntime)) {
    // The hardened runtime is not enabled. Note that kSecCodeSignatureRuntime
    // == CS_RUNTIME.
    return true;
  }

  // The hardened runtime is enabled. From this point on, early returns must
  // return false, indicating that MAP_JIT is not to be used. It’s an error
  // (EINVAL) to use MAP_JIT with the hardened runtime unless the JIT
  // entitlement is specified.

  ScopedCFTypeRef<CFTypeRef> jit_entitlement(SecTaskCopyValueForEntitlement(
      task.get(), CFSTR("com.apple.security.cs.allow-jit"), nullptr));
  if (!jit_entitlement)
    return false;

  return mac::CFCast<CFBooleanRef>(jit_entitlement.get()) == kCFBooleanTrue;
}
#endif  // defined(OS_APPLE)

}  // namespace

// |mmap| uses a nearby address if the hint address is blocked.
constexpr bool kHintIsAdvisory = true;
std::atomic<int32_t> s_allocPageErrorCode{0};

int GetAccessFlags(PageAccessibilityConfiguration accessibility) {
  switch (accessibility) {
    case PageRead:
      return PROT_READ;
    case PageReadWrite:
      return PROT_READ | PROT_WRITE;
    case PageReadExecute:
      return PROT_READ | PROT_EXEC;
    case PageReadWriteExecute:
      return PROT_READ | PROT_WRITE | PROT_EXEC;
    default:
      NOTREACHED();
      FALLTHROUGH;
    case PageInaccessible:
      return PROT_NONE;
  }
}

void* SystemAllocPagesInternal(void* hint,
                               size_t length,
                               PageAccessibilityConfiguration accessibility,
                               PageTag page_tag) {
#if defined(OS_APPLE)
  // Use a custom tag to make it easier to distinguish Partition Alloc regions
  // in vmmap(1). Tags between 240-255 are supported.
  PA_DCHECK(PageTag::kFirst <= page_tag);
  PA_DCHECK(PageTag::kLast >= page_tag);
  int fd = VM_MAKE_TAG(static_cast<int>(page_tag));
#else
  int fd = -1;
#endif

  int access_flag = GetAccessFlags(accessibility);
  int map_flags = MAP_ANONYMOUS | MAP_PRIVATE;

#if defined(OS_APPLE)
  // On macOS 10.14 and higher, executables that are code signed with the
  // "runtime" option cannot execute writable memory by default. They can opt
  // into this capability by specifying the "com.apple.security.cs.allow-jit"
  // code signing entitlement and allocating the region with the MAP_JIT flag.
  static const bool kUseMapJit = UseMapJit();
  if (page_tag == PageTag::kV8 && kUseMapJit) {
    map_flags |= MAP_JIT;
  }
#endif

  void* ret = mmap(hint, length, access_flag, map_flags, fd, 0);
  if (ret == MAP_FAILED) {
    s_allocPageErrorCode = errno;
    ret = nullptr;
  }

#if defined(OS_ANDROID)
  // On Android, anonymous mappings can have a name attached to them. This is
  // useful for debugging, and double-checking memory attribution.
  if (ret) {
    // No error checking on purpose, testing only.
    prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, ret, length,
          PageTagToName(page_tag));
  }
#endif

  return ret;
}

bool TrySetSystemPagesAccessInternal(
    void* address,
    size_t length,
    PageAccessibilityConfiguration accessibility) {
  return 0 ==
         HANDLE_EINTR(mprotect(address, length, GetAccessFlags(accessibility)));
}

void SetSystemPagesAccessInternal(
    void* address,
    size_t length,
    PageAccessibilityConfiguration accessibility) {
  PA_PCHECK(0 == HANDLE_EINTR(
                     mprotect(address, length, GetAccessFlags(accessibility))));
}

void FreePagesInternal(void* address, size_t length) {
  PA_PCHECK(0 == munmap(address, length));
}

void* TrimMappingInternal(void* base,
                          size_t base_length,
                          size_t trim_length,
                          PageAccessibilityConfiguration accessibility,
                          size_t pre_slack,
                          size_t post_slack) {
  void* ret = base;
  // We can resize the allocation run. Release unneeded memory before and after
  // the aligned range.
  if (pre_slack) {
    FreePages(base, pre_slack);
    ret = reinterpret_cast<char*>(base) + pre_slack;
  }
  if (post_slack) {
    FreePages(reinterpret_cast<char*>(ret) + trim_length, post_slack);
  }
  return ret;
}

void DecommitSystemPagesInternal(
    void* address,
    size_t length,
    PageAccessibilityDisposition accessibility_disposition) {
  // In POSIX, there is no decommit concept. Discarding is an effective way of
  // implementing the Windows semantics where the OS is allowed to not swap the
  // pages in the region.
  DiscardSystemPages(address, length);

  // Make pages inaccessible, unless the caller requested to keep permissions.
  //
  // Note, there is a small window between these calls when the pages can be
  // incorrectly touched and brought back to memory. Not ideal, but doing those
  // operaions in the opposite order resulted in PMF regression on Mac (see
  // crbug.com/1153021).
  if (accessibility_disposition == PageUpdatePermissions) {
    SetSystemPagesAccess(address, length, PageInaccessible);
  }
}

void RecommitSystemPagesInternal(
    void* address,
    size_t length,
    PageAccessibilityConfiguration accessibility,
    PageAccessibilityDisposition accessibility_disposition) {
  // On POSIX systems, the caller needs to simply read the memory to recommit
  // it. However, if decommit changed the permissions, recommit has to change
  // them back.
  if (accessibility_disposition == PageUpdatePermissions) {
    SetSystemPagesAccess(address, length, accessibility);
  }

#if defined(OS_APPLE)
  // On macOS, to update accounting, we need to make another syscall. For more
  // details, see https://crbug.com/823915.
  madvise(address, length, MADV_FREE_REUSE);
#endif
}

void DiscardSystemPagesInternal(void* address, size_t length) {
#if defined(OS_APPLE)
  int ret = madvise(address, length, MADV_FREE_REUSABLE);
  if (ret) {
    // MADV_FREE_REUSABLE sometimes fails, so fall back to MADV_DONTNEED.
    ret = madvise(address, length, MADV_DONTNEED);
  }
  PA_PCHECK(ret == 0);
#else
  // We have experimented with other flags, but with suboptimal results.
  //
  // MADV_FREE (Linux): Makes our memory measurements less predictable;
  // performance benefits unclear.
  //
  // Therefore, we just do the simple thing: MADV_DONTNEED.
  PA_PCHECK(0 == madvise(address, length, MADV_DONTNEED));
#endif
}

}  // namespace base

#endif  // BASE_ALLOCATOR_PARTITION_ALLOCATOR_PAGE_ALLOCATOR_INTERNALS_POSIX_H_
