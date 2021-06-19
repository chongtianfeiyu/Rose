// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


// This file is autogenerated by
//     base\android\jni_generator\jni_generator.py
// For
//     org/chromium/base/CpuFeatures

#ifndef org_chromium_base_CpuFeatures_JNI
#define org_chromium_base_CpuFeatures_JNI

#include <jni.h>

#include "base/android/jni_generator/jni_generator_helper.h"


// Step 1: Forward declarations.


// Step 2: Constants (optional).


// Step 3: Method stubs.
namespace base {
namespace android {

static jint JNI_CpuFeatures_GetCoreCount(JNIEnv* env);

JNI_GENERATOR_EXPORT jint
    Java_org_chromium_base_natives_GEN_1JNI_org_1chromium_1base_1CpuFeatures_1getCoreCount(
    JNIEnv* env,
    jclass jcaller) {
  return JNI_CpuFeatures_GetCoreCount(env);
}

static jlong JNI_CpuFeatures_GetCpuFeatures(JNIEnv* env);

JNI_GENERATOR_EXPORT jlong
    Java_org_chromium_base_natives_GEN_1JNI_org_1chromium_1base_1CpuFeatures_1getCpuFeatures(
    JNIEnv* env,
    jclass jcaller) {
  return JNI_CpuFeatures_GetCpuFeatures(env);
}


}  // namespace android
}  // namespace base

#endif  // org_chromium_base_CpuFeatures_JNI
