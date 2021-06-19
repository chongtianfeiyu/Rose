// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.
//
// Tencent is pleased to support the open source community by making WeChat QRCode available.
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.

#ifndef __DETECTOR_SSD_DETECTOR_HPP_
#define __DETECTOR_SSD_DETECTOR_HPP_

#include <stdio.h>

#include "opencv2/dnn.hpp"
#include "opencv2/imgproc.hpp"
namespace cv {
namespace wechat_qrcode {

class SSDDetector {
public:
    SSDDetector(){};
    ~SSDDetector(){};
    int init(const char* proto_data, size_t proto_size, const char* model_data, size_t model_size);
    std::vector<Mat> forward(Mat img, const int target_width, const int target_height);

private:
    dnn::Net net_;
};

}  // namespace wechat_qrcode
}  // namespace cv
#endif  // __DETECTOR_SSD_DETECTOR_HPP_
