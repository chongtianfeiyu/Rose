// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


// This file is autogenerated by
//     base\android\jni_generator\jni_generator.py
// For
//     org/chromium/base/memory/JavaHeapDumpGenerator

#ifndef org_chromium_base_memory_JavaHeapDumpGenerator_JNI
#define org_chromium_base_memory_JavaHeapDumpGenerator_JNI

#include <jni.h>

#include "base/android/jni_generator/jni_generator_helper.h"


// Step 1: Forward declarations.

JNI_REGISTRATION_EXPORT extern const char
    kClassPath_org_chromium_base_memory_JavaHeapDumpGenerator[];
const char kClassPath_org_chromium_base_memory_JavaHeapDumpGenerator[] =
    "org/chromium/base/memory/JavaHeapDumpGenerator";
// Leaking this jclass as we cannot use LazyInstance from some threads.
JNI_REGISTRATION_EXPORT std::atomic<jclass>
    g_org_chromium_base_memory_JavaHeapDumpGenerator_clazz(nullptr);
#ifndef org_chromium_base_memory_JavaHeapDumpGenerator_clazz_defined
#define org_chromium_base_memory_JavaHeapDumpGenerator_clazz_defined
inline jclass org_chromium_base_memory_JavaHeapDumpGenerator_clazz(JNIEnv* env) {
  return base::android::LazyGetClass(env, kClassPath_org_chromium_base_memory_JavaHeapDumpGenerator,
      &g_org_chromium_base_memory_JavaHeapDumpGenerator_clazz);
}
#endif


// Step 2: Constants (optional).


// Step 3: Method stubs.

static std::atomic<jmethodID>
    g_org_chromium_base_memory_JavaHeapDumpGenerator_generateHprof(nullptr);
static jboolean Java_JavaHeapDumpGenerator_generateHprof(JNIEnv* env, const
    base::android::JavaRef<jstring>& filePath) {
  jclass clazz = org_chromium_base_memory_JavaHeapDumpGenerator_clazz(env);
  CHECK_CLAZZ(env, clazz,
      org_chromium_base_memory_JavaHeapDumpGenerator_clazz(env), false);

  jni_generator::JniJavaCallContextChecked call_context;
  call_context.Init<
      base::android::MethodID::TYPE_STATIC>(
          env,
          clazz,
          "generateHprof",
          "(Ljava/lang/String;)Z",
          &g_org_chromium_base_memory_JavaHeapDumpGenerator_generateHprof);

  jboolean ret =
      env->CallStaticBooleanMethod(clazz,
          call_context.base.method_id, filePath.obj());
  return ret;
}

#endif  // org_chromium_base_memory_JavaHeapDumpGenerator_JNI
