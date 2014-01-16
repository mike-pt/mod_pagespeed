// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_APACHE_HEADER_UTIL_H_
#define NET_INSTAWEB_APACHE_HEADER_UTIL_H_

struct request_rec;

namespace net_instaweb {

class RequestHeaders;
class ResponseHeaders;

// Converts Apache header structure into RequestHeaders.
void ApacheRequestToRequestHeaders(const request_rec& request,
                                   RequestHeaders* request_headers);

// Converts Apache header structure (request.headers_out) into ResponseHeaders
// headers. If err_headers is not NULL then request.err_headers_out is copied
// into it. In the event that headers == err_headers, the headers from
// request.err_headers_out will be appended to the list of headers, but no
// merging occurs.
void ApacheRequestToResponseHeaders(const request_rec& request,
                                    ResponseHeaders* headers,
                                    ResponseHeaders* err_headers);

// Converts the ResponseHeaders to the output headers.  This function
// does not alter the status code or the major/minor version of the
// Apache request.
void ResponseHeadersToApacheRequest(const ResponseHeaders& response_headers,
                                    request_rec* request);

// Converts ResponseHeaders into Apache request err_headers.  This
// function does not alter the status code or the major/minor version
// of the Apache request.
void ErrorHeadersToApacheRequest(const ResponseHeaders& err_response_headers,
                                 request_rec* request);

// Remove downstream filters that might corrupt our caching headers.
void DisableDownstreamHeaderFilters(request_rec* request);

// Debug utility for printing Apache headers to stdout
void PrintHeaders(request_rec* request);

// Updates headers related to caching (but not Cache-Control).
void DisableCachingRelatedHeaders(request_rec* request);

// Updates caching headers to ensure the resulting response is not cached.
// Removes any max-age specification, and adds max-age=0, no-cache.
void DisableCacheControlHeader(request_rec* request);

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_APACHE_HEADER_UTIL_H_
