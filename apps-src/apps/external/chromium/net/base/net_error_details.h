// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_NET_ERROR_DETAILS_H_
#define NET_BASE_NET_ERROR_DETAILS_H_

#include "net/base/net_export.h"
#include "net/http/http_response_info.h"
// #include "net/third_party/quiche/src/quic/core/quic_error_codes.h"

namespace net {

// A record of net errors with granular error specification generated by
// net stack.
struct NET_EXPORT NetErrorDetails {
  NetErrorDetails()
      : quic_broken(false),
        // quic_connection_error(quic::QUIC_NO_ERROR),
        connection_info(HttpResponseInfo::CONNECTION_INFO_UNKNOWN),
        quic_port_migration_detected(false) {}

  NetErrorDetails(bool quic_broken/*, quic::QuicErrorCode quic_connection_error*/)
      : quic_broken(quic_broken),
        // quic_connection_error(quic_connection_error),
        connection_info(HttpResponseInfo::CONNECTION_INFO_UNKNOWN),
        quic_port_migration_detected(false) {}

  // True if all QUIC alternative services are marked broken for the origin.
  bool quic_broken;
  // QUIC granular error info.
  // quic::QuicErrorCode quic_connection_error;
  // Early prediction of the connection type that this request attempts to use.
  // Will be discarded by upper layers if the connection type can be fetched
  // from response header from the server.
  HttpResponseInfo::ConnectionInfo connection_info;
  // True if receives a GoAway frame from the server due to connection
  // migration with port change.
  bool quic_port_migration_detected;
};

}  // namespace net

#endif  // NET_BASE_NET_ERROR_DETAILS_H_
