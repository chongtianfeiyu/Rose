// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


// This file is autogenerated by
//     base\android\jni_generator\jni_generator.py
// For
//     org/chromium/net/AndroidCertVerifyResult

#ifndef org_chromium_net_AndroidCertVerifyResult_JNI
#define org_chromium_net_AndroidCertVerifyResult_JNI

#include <jni.h>

#include "base/android/jni_generator/jni_generator_helper.h"


// Step 1: Forward declarations.

JNI_REGISTRATION_EXPORT extern const char kClassPath_org_chromium_net_AndroidCertVerifyResult[];
const char kClassPath_org_chromium_net_AndroidCertVerifyResult[] =
    "org/chromium/net/AndroidCertVerifyResult";
// Leaking this jclass as we cannot use LazyInstance from some threads.
JNI_REGISTRATION_EXPORT std::atomic<jclass>
    g_org_chromium_net_AndroidCertVerifyResult_clazz(nullptr);
#ifndef org_chromium_net_AndroidCertVerifyResult_clazz_defined
#define org_chromium_net_AndroidCertVerifyResult_clazz_defined
inline jclass org_chromium_net_AndroidCertVerifyResult_clazz(JNIEnv* env) {
  return base::android::LazyGetClass(env, kClassPath_org_chromium_net_AndroidCertVerifyResult,
      &g_org_chromium_net_AndroidCertVerifyResult_clazz);
}
#endif


// Step 2: Constants (optional).


// Step 3: Method stubs.
namespace net {
namespace android {


static std::atomic<jmethodID> g_org_chromium_net_AndroidCertVerifyResult_getStatus(nullptr);
static jint Java_AndroidCertVerifyResult_getStatus(JNIEnv* env, const
    base::android::JavaRef<jobject>& obj) {
  jclass clazz = org_chromium_net_AndroidCertVerifyResult_clazz(env);
  CHECK_CLAZZ(env, obj.obj(),
      org_chromium_net_AndroidCertVerifyResult_clazz(env), 0);

  jni_generator::JniJavaCallContextChecked call_context;
  call_context.Init<
      base::android::MethodID::TYPE_INSTANCE>(
          env,
          clazz,
          "getStatus",
          "()I",
          &g_org_chromium_net_AndroidCertVerifyResult_getStatus);

  jint ret =
      env->CallIntMethod(obj.obj(),
          call_context.base.method_id);
  return ret;
}

static std::atomic<jmethodID>
    g_org_chromium_net_AndroidCertVerifyResult_isIssuedByKnownRoot(nullptr);
static jboolean Java_AndroidCertVerifyResult_isIssuedByKnownRoot(JNIEnv* env, const
    base::android::JavaRef<jobject>& obj) {
  jclass clazz = org_chromium_net_AndroidCertVerifyResult_clazz(env);
  CHECK_CLAZZ(env, obj.obj(),
      org_chromium_net_AndroidCertVerifyResult_clazz(env), false);

  jni_generator::JniJavaCallContextChecked call_context;
  call_context.Init<
      base::android::MethodID::TYPE_INSTANCE>(
          env,
          clazz,
          "isIssuedByKnownRoot",
          "()Z",
          &g_org_chromium_net_AndroidCertVerifyResult_isIssuedByKnownRoot);

  jboolean ret =
      env->CallBooleanMethod(obj.obj(),
          call_context.base.method_id);
  return ret;
}

static std::atomic<jmethodID>
    g_org_chromium_net_AndroidCertVerifyResult_getCertificateChainEncoded(nullptr);
static base::android::ScopedJavaLocalRef<jobjectArray>
    Java_AndroidCertVerifyResult_getCertificateChainEncoded(JNIEnv* env, const
    base::android::JavaRef<jobject>& obj) {
  jclass clazz = org_chromium_net_AndroidCertVerifyResult_clazz(env);
  CHECK_CLAZZ(env, obj.obj(),
      org_chromium_net_AndroidCertVerifyResult_clazz(env), NULL);

  jni_generator::JniJavaCallContextChecked call_context;
  call_context.Init<
      base::android::MethodID::TYPE_INSTANCE>(
          env,
          clazz,
          "getCertificateChainEncoded",
          "()[[B",
          &g_org_chromium_net_AndroidCertVerifyResult_getCertificateChainEncoded);

  jobjectArray ret =
      static_cast<jobjectArray>(env->CallObjectMethod(obj.obj(),
          call_context.base.method_id));
  return base::android::ScopedJavaLocalRef<jobjectArray>(env, ret);
}

}  // namespace android
}  // namespace net

#endif  // org_chromium_net_AndroidCertVerifyResult_JNI
