// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_URL_REQUEST_URL_REQUEST_HTTP_JOB_ROSE_H_
#define NET_URL_REQUEST_URL_REQUEST_HTTP_JOB_ROSE_H_

#include "net/url_request/url_request_rose_util.h"
#include "global.hpp"

using namespace std::placeholders;

namespace net {

// in general for test.
// @url: https://www.github.com
bool fetch_url_data(const std::string& _url, const std::string& path, int timeout);
bool fetch_url_2_mem(const std::string& _url, std::string& received_data, int timeout);
std::string err_2_description(int err);
std::unique_ptr<UploadDataStream> CreateSimpleUploadData(const char* data, int size);

struct thttp_agent
{
	thttp_agent(const std::string& _url, const std::string& _method, const std::string& _cert, int timeout);

	const GURL url;
	std::function<bool (URLRequest&, HttpRequestHeaders&, std::string&)> did_pre;
	std::function<bool (const URLRequest&, const RoseDelegate&, int)> did_post;

	const std::string method;
	const std::string cert;
	int timeout;
};
bool handle_http_request(thttp_agent& agent);

}

#endif  // NET_URL_REQUEST_URL_REQUEST_HTTP_JOB_ROSE_H_
