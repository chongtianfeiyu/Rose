// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_URL_REQUEST_URL_REQUEST_ROSE_UTIL_H_
#define NET_URL_REQUEST_URL_REQUEST_ROSE_UTIL_H_

#include <stdint.h>
#include <stdlib.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "base/path_service.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "net/base/io_buffer.h"
#include "net/base/load_timing_info.h"
#include "net/base/net_errors.h"
#include "net/base/network_delegate_impl.h"
#include "net/base/request_priority.h"
#include "net/base/transport_info.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
// #include "net/ftp/ftp_network_layer.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_layer.h"
#include "net/http/http_network_session.h"
#include "net/http/http_request_headers.h"
#include "net/ssl/ssl_config_service_defaults.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_context_storage.h"
#include "net/url_request/url_request_interceptor.h"
#include "url/url_util.h"

#include <SDL_timer.h>

namespace gui2 {
class tprogress_;
}

namespace net {

//-----------------------------------------------------------------------------

class RoseURLRequestContext : public URLRequestContext {
 public:
  // Default constructor like TestURLRequestContext() but does not call
  // Init() in case |delay_initialization| is true. This allows modifying the
  // URLRequestContext before it is constructed completely. If
  // |delay_initialization| is true, Init() needs be be called manually.
  explicit RoseURLRequestContext(const std::string& cert);
  ~RoseURLRequestContext() override;

  void Init();

  ClientSocketFactory* client_socket_factory() {
    return client_socket_factory_;
  }
  void set_client_socket_factory(ClientSocketFactory* factory) {
    client_socket_factory_ = factory;
  }

  void set_http_network_session_params(
      std::unique_ptr<HttpNetworkSession::Params> session_params) {
    http_network_session_params_ = std::move(session_params);
  }

  void set_http_network_session_context(
      std::unique_ptr<HttpNetworkSession::Context> session_context) {
    http_network_session_context_ = std::move(session_context);
  }

  void SetCTPolicyEnforcer(
      std::unique_ptr<CTPolicyEnforcer> ct_policy_enforcer) {
    context_storage_.set_ct_policy_enforcer(std::move(ct_policy_enforcer));
  }

  void set_create_default_http_user_agent_settings(bool value) {
    create_default_http_user_agent_settings_ = value;
  }

  // Like CreateRequest, but also updates |site_for_cookies| to give the request
  // a 1st-party context.
  std::unique_ptr<URLRequest> CreateFirstPartyRequest(
      const GURL& url,
      RequestPriority priority,
      URLRequest::Delegate* delegate,
      NetworkTrafficAnnotationTag traffic_annotation) const;

 private:
  const std::string cert_;
  bool initialized_ = false;

  // Optional parameters to override default values.  Note that values in the
  // HttpNetworkSession::Context that point to other objects the
  // TestURLRequestContext creates will be overwritten.
  std::unique_ptr<HttpNetworkSession::Params> http_network_session_params_;
  std::unique_ptr<HttpNetworkSession::Context> http_network_session_context_;

  // Not owned:
  ClientSocketFactory* client_socket_factory_ = nullptr;

  bool create_default_http_user_agent_settings_ = true;

 protected:
  URLRequestContextStorage context_storage_;
};

//-----------------------------------------------------------------------------
struct tshow_slice_executor
{
	tshow_slice_executor(gui2::tprogress_* progress)
		: progress(progress)
        , respbody_spend_ms(0)
		, last_ReadCompleted_ticks(0)
	{};

    void OnReadCompleted()
    {
        uint32_t now = SDL_GetTicks();
        if (last_ReadCompleted_ticks != 0) {
            respbody_spend_ms += now - last_ReadCompleted_ticks;
        }
        last_ReadCompleted_ticks = now;
    }

	gui2::tprogress_* progress;
    uint32_t respbody_spend_ms;
	uint32_t last_ReadCompleted_ticks;
};

class RoseDelegate : public URLRequest::Delegate {
 public:
  RoseDelegate();
  ~RoseDelegate() override;

  // Helpers to create a RunLoop, set |on_<event>| from it, then Run() it.
  void RunUntilComplete();
  void RunUntilRedirect();
  // Enables quitting the message loop in response to auth requests, as opposed
  // to returning credentials or cancelling the request.
  void RunUntilAuthRequired();

  // Sets the closure to be run on completion, for tests which need more fine-
  // grained control than RunUntilComplete().
  void set_on_complete(base::OnceClosure on_complete) {
    use_legacy_on_complete_ = false;
    on_complete_ = std::move(on_complete);
  }

  // Sets the result returned by subsequent calls to OnConnected().
  void set_on_connected_result(int result) { on_connected_result_ = result; }

  void set_cancel_in_received_redirect(bool val) { cancel_in_rr_ = val; }
  void set_cancel_in_response_started(bool val) { cancel_in_rs_ = val; }
  void set_cancel_in_received_data(bool val) { cancel_in_rd_ = val; }
  void set_cancel_in_received_data_pending(bool val) {
    cancel_in_rd_pending_ = val;
  }

  void set_allow_certificate_errors(bool val) {
    allow_certificate_errors_ = val;
  }
  void set_credentials(const AuthCredentials& credentials) {
    credentials_ = credentials;
  }

  // Returns the list of arguments with which OnConnected() was called.
  // The arguments are listed in the same order as the calls were received.
  const std::vector<TransportInfo>& transports() const { return transports_; }

  // query state
  const std::string& data_received() const { return data_received_; }
  int bytes_received() const { return static_cast<int>(data_received_.size()); }
  int response_started_count() const { return response_started_count_; }
  int received_bytes_count() const { return received_bytes_count_; }
  int received_redirect_count() const { return received_redirect_count_; }
  bool received_data_before_response() const {
    return received_data_before_response_;
  }
  RedirectInfo redirect_info() { return redirect_info_; }
  bool request_failed() const { return request_failed_; }
  bool have_certificate_errors() const { return have_certificate_errors_; }
  bool certificate_errors_are_fatal() const {
    return certificate_errors_are_fatal_;
  }
  int certificate_net_error() const { return certificate_net_error_; }
  bool auth_required_called() const { return auth_required_; }
  bool response_completed() const { return response_completed_; }
  int request_status() const { return request_status_; }

  // URLRequest::Delegate:
  int OnConnected(URLRequest* request, const TransportInfo& info) override;
  void OnReceivedRedirect(URLRequest* request,
                          const RedirectInfo& redirect_info,
                          bool* defer_redirect) override;
  void OnAuthRequired(URLRequest* request,
                      const AuthChallengeInfo& auth_info) override;
  // NOTE: |fatal| causes |certificate_errors_are_fatal_| to be set to true.
  // (Unit tests use this as a post-condition.) But for policy, this method
  // consults |allow_certificate_errors_|.
  void OnSSLCertificateError(URLRequest* request,
                             int net_error,
                             const SSLInfo& ssl_info,
                             bool fatal) override;
  void OnResponseStarted(URLRequest* request, int net_error) override;
  void OnReadCompleted(URLRequest* request, int bytes_read) override;

  void OnSendBodyCompleted(URLRequest* request, int sofar_bytes, int expected_bytes) override;
  const tshow_slice_executor& executor() const { return executor_; }

 private:
  static const int kBufferSize = 4096;

  virtual void OnResponseCompleted(URLRequest* request);

  tshow_slice_executor executor_;

  // options for controlling behavior
  int on_connected_result_ = OK;
  bool cancel_in_rr_ = false;
  bool cancel_in_rs_ = false;
  bool cancel_in_rd_ = false;
  bool cancel_in_rd_pending_ = false;
  bool allow_certificate_errors_ = false;
  AuthCredentials credentials_;

  // True if legacy on-complete behaviour, using QuitCurrent*Deprecated(), is
  // enabled. This is cleared if any of the Until*() APIs are used.
  bool use_legacy_on_complete_ = true;

  // Used to register RunLoop quit closures, to implement the Until*() closures.
  base::OnceClosure on_complete_;
  base::OnceClosure on_redirect_;
  base::OnceClosure on_auth_required_;

  // tracks status of callbacks
  std::vector<TransportInfo> transports_;
  int response_started_count_ = 0;
  int received_bytes_count_ = 0;
  int received_redirect_count_ = 0;
  bool received_data_before_response_ = false;
  bool request_failed_ = false;
  bool have_certificate_errors_ = false;
  bool certificate_errors_are_fatal_ = false;
  int certificate_net_error_ = 0;
  bool auth_required_ = false;
  std::string data_received_;
  bool response_completed_ = false;

  // tracks status of request
  int request_status_ = ERR_IO_PENDING;

  // our read buffer
  scoped_refptr<IOBuffer> buf_;

  RedirectInfo redirect_info_;
};

//-----------------------------------------------------------------------------

class TestNetworkDelegate : public NetworkDelegateImpl {
 public:
  enum Options {
    NO_GET_COOKIES = 1 << 0,
    NO_SET_COOKIE  = 1 << 1,
  };

  TestNetworkDelegate();
  ~TestNetworkDelegate() override;

  // Writes the LoadTimingInfo during the most recent call to OnBeforeRedirect.
  bool GetLoadTimingInfoBeforeRedirect(
      LoadTimingInfo* load_timing_info_before_redirect) const;

  // Will redirect once to the given URL when the next set of headers are
  // received.
  void set_redirect_on_headers_received_url(
      GURL redirect_on_headers_received_url) {
    redirect_on_headers_received_url_ = redirect_on_headers_received_url;
  }

  // Adds a X-Network-Delegate header to the first OnHeadersReceived call, but
  // not subsequent ones.
  void set_add_header_to_first_response(bool add_header_to_first_response) {
    add_header_to_first_response_ = add_header_to_first_response;
  }

  void set_preserve_fragment_on_redirect_url(
      const base::Optional<GURL>& preserve_fragment_on_redirect_url) {
    preserve_fragment_on_redirect_url_ = preserve_fragment_on_redirect_url;
  }

  void set_cookie_options(int o) {cookie_options_bit_mask_ = o; }

  int last_error() const { return last_error_; }
  int error_count() const { return error_count_; }
  int created_requests() const { return created_requests_; }
  int destroyed_requests() const { return destroyed_requests_; }
  int completed_requests() const { return completed_requests_; }
  int canceled_requests() const { return canceled_requests_; }
  int blocked_get_cookies_count() const { return blocked_get_cookies_count_; }
  int blocked_set_cookie_count() const { return blocked_set_cookie_count_; }
  int set_cookie_count() const { return set_cookie_count_; }

  void set_cancel_request_with_policy_violating_referrer(bool val) {
    cancel_request_with_policy_violating_referrer_ = val;
  }

  int before_start_transaction_count() const {
    return before_start_transaction_count_;
  }

  int headers_received_count() const { return headers_received_count_; }

  void set_before_start_transaction_fails() {
    before_start_transaction_fails_ = true;
  }

 protected:
  // NetworkDelegate:
  int OnBeforeURLRequest(URLRequest* request,
                         CompletionOnceCallback callback,
                         GURL* new_url) override;
  int OnBeforeStartTransaction(URLRequest* request,
                               CompletionOnceCallback callback,
                               HttpRequestHeaders* headers) override;
  int OnHeadersReceived(
      URLRequest* request,
      CompletionOnceCallback callback,
      const HttpResponseHeaders* original_response_headers,
      scoped_refptr<HttpResponseHeaders>* override_response_headers,
      const IPEndPoint& endpoint,
      base::Optional<GURL>* preserve_fragment_on_redirect_url) override;
  void OnBeforeRedirect(URLRequest* request, const GURL& new_location) override;
  void OnResponseStarted(URLRequest* request, int net_error) override;
  void OnCompleted(URLRequest* request, bool started, int net_error) override;
  void OnURLRequestDestroyed(URLRequest* request) override;
  void OnPACScriptError(int line_number, const base::string16& error) override;
/*
  bool OnCanGetCookies(const URLRequest& request,
                       bool allowed_from_caller) override;
  bool OnCanSetCookie(const URLRequest& request,
                      const net::CanonicalCookie& cookie,
                      CookieOptions* options,
                      bool allowed_from_caller) override;
*/
  bool OnCancelURLRequestWithPolicyViolatingReferrerHeader(
      const URLRequest& request,
      const GURL& target_url,
      const GURL& referrer_url) const override;

  void InitRequestStatesIfNew(int request_id);

  // Gets a request ID if it already has one, assigns a new one and returns that
  // if not.
  int GetRequestId(URLRequest* request);

  GURL redirect_on_headers_received_url_;
  // URL to mark as retaining its fragment if redirected to at the
  // OnHeadersReceived() stage.
  base::Optional<GURL> preserve_fragment_on_redirect_url_;

  int last_error_;
  int error_count_;
  int created_requests_;
  int destroyed_requests_;
  int completed_requests_;
  int canceled_requests_;
  int cookie_options_bit_mask_;
  int blocked_get_cookies_count_;
  int blocked_set_cookie_count_;
  int set_cookie_count_;
  int before_start_transaction_count_;
  int headers_received_count_;

  // NetworkDelegate callbacks happen in a particular order (e.g.
  // OnBeforeURLRequest is always called before OnBeforeStartTransaction).
  // This bit-set indicates for each request id (key) what events may be sent
  // next.
  std::map<int, int> next_states_;

  // A log that records for each request id (key) the order in which On...
  // functions were called.
  std::map<int, std::string> event_order_;

  LoadTimingInfo load_timing_info_before_redirect_;
  bool has_load_timing_info_before_redirect_;

  bool cancel_request_with_policy_violating_referrer_;  // false by default
  bool before_start_transaction_fails_;
  bool add_header_to_first_response_;
  int next_request_id_;
};

}  // namespace net

#endif  // NET_URL_REQUEST_URL_REQUEST_ROSE_UTIL_H_
