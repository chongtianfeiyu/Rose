/*
 *  Copyright 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/video_coding/codecs/test/objc_codec_h264_test.h"

#import "WebRTC/RTCVideoCodecH264.h"
#include "rtc_base/ptr_util.h"
#include "sdk/objc/Framework/Classes/VideoToolbox/objc_video_decoder_factory.h"
#include "sdk/objc/Framework/Classes/VideoToolbox/objc_video_encoder_factory.h"

namespace webrtc {

std::unique_ptr<VideoEncoderFactory> CreateObjCEncoderFactory() {
  return rtc::MakeUnique<ObjCVideoEncoderFactory>([[RTCVideoEncoderFactoryH264 alloc] init]);
}

std::unique_ptr<VideoDecoderFactory> CreateObjCDecoderFactory() {
  return rtc::MakeUnique<ObjCVideoDecoderFactory>([[RTCVideoDecoderFactoryH264 alloc] init]);
}

std::unique_ptr<cricket::WebRtcVideoEncoderFactory> CreateObjCEncoderFactory2() {
  return rtc::MakeUnique<ObjCVideoEncoderFactory>([[RTCVideoEncoderFactoryH264 alloc] init]);
}

std::unique_ptr<cricket::WebRtcVideoDecoderFactory> CreateObjCDecoderFactory2() {
  return rtc::MakeUnique<ObjCVideoDecoderFactory>([[RTCVideoDecoderFactoryH264 alloc] init]);
}

}  // namespace webrtc
