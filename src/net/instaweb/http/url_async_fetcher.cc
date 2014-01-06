/*
 * Copyright 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/http/public/url_async_fetcher.h"

#include "net/instaweb/http/public/inflating_fetch.h"

namespace net_instaweb {

const int64 UrlAsyncFetcher::kUnspecifiedTimeout = 0;

// Statistics group names.
const char UrlAsyncFetcher::kStatisticsGroup[] = "Statistics";

UrlAsyncFetcher::~UrlAsyncFetcher() {
}

void UrlAsyncFetcher::ShutDown() {
}

AsyncFetch* UrlAsyncFetcher::EnableInflation(
    AsyncFetch* fetch,
    const std::set<const ContentType*>*
        inflation_content_type_blacklist) const {
  InflatingFetch* inflating_fetch = new InflatingFetch(fetch);
  if (fetch_with_gzip_) {
    inflating_fetch->EnableGzipFromBackend();
  }
  if (inflation_content_type_blacklist != NULL) {
    inflating_fetch->set_inflation_content_type_blacklist(
        *inflation_content_type_blacklist);
  }
  return inflating_fetch;
}

}  // namespace net_instaweb
