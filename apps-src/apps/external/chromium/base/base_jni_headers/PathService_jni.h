// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


// This file is autogenerated by
//     base\android\jni_generator\jni_generator.py
// For
//     org/chromium/base/PathService

#ifndef org_chromium_base_PathService_JNI
#define org_chromium_base_PathService_JNI

#include <jni.h>

#include "base/android/jni_generator/jni_generator_helper.h"


// Step 1: Forward declarations.


// Step 2: Constants (optional).


// Step 3: Method stubs.
namespace base {
namespace android {

static void JNI_PathService_Override(JNIEnv* env, jint what,
    const base::android::JavaParamRef<jstring>& path);

JNI_GENERATOR_EXPORT void
    Java_org_chromium_base_natives_GEN_1JNI_org_1chromium_1base_1PathService_1override(
    JNIEnv* env,
    jclass jcaller,
    jint what,
    jstring path) {
  return JNI_PathService_Override(env, what, base::android::JavaParamRef<jstring>(env, path));
}


}  // namespace android
}  // namespace base

#endif  // org_chromium_base_PathService_JNI