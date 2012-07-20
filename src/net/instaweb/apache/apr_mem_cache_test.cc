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

// Unit-test the memcache interface.

#include "net/instaweb/apache/apr_mem_cache.h"

#include <cstddef>

#include "apr_pools.h"

#include "base/scoped_ptr.h"
#include "net/instaweb/apache/apr_timer.h"
#include "net/instaweb/util/cache_test_base.h"
#include "net/instaweb/util/public/basictypes.h"
#include "net/instaweb/util/public/google_message_handler.h"
#include "net/instaweb/util/public/gtest.h"
#include "net/instaweb/util/public/md5_hasher.h"
#include "net/instaweb/util/public/shared_string.h"
#include "net/instaweb/util/public/string.h"
#include "net/instaweb/util/public/string_util.h"
#include "net/instaweb/util/public/timer.h"

namespace {

const char kPortString[] = "6765";

}  // namespace

namespace net_instaweb {

class AprMemCacheTest : public CacheTestBase {
 protected:
  AprMemCacheTest() : fail_(false) {
    apr_initialize();
    GoogleString servers = StrCat("localhost:", kPortString);
    cache_.reset(new AprMemCache(servers, 5, &md5_hasher_, &handler_));
    GoogleString buf;
    if (!cache_->Connect()) {
      // apr_memcache actually lazy-connects to memcached, it seems, so
      // if we fail the Connect call then something is truly broken.
      fail_ = true;
    } else if (!cache_->GetStatus(&buf)) {
      LOG(ERROR) << "please start 'memcached -p 6765";
      fail_ = true;
    }
  }
  virtual CacheInterface* Cache() { return cache_.get(); }

  bool fail_;
  GoogleMessageHandler handler_;
  MD5Hasher md5_hasher_;
  scoped_ptr<AprMemCache> cache_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AprMemCacheTest);
};

// Simple flow of putting in an item, getting it, deleting it.
TEST_F(AprMemCacheTest, PutGetDelete) {
  ASSERT_FALSE(fail_);

  CheckPut("Name", "Value");
  CheckGet("Name", "Value");
  CheckNotFound("Another Name");

  CheckPut("Name", "NewValue");
  CheckGet("Name", "NewValue");

  cache_->Delete("Name");
  CheckNotFound("Name");
}

TEST_F(AprMemCacheTest, BasicInvalid) {
  ASSERT_FALSE(fail_);

  // Check that we honor callback veto on validity.
  CheckPut("nameA", "valueA");
  CheckPut("nameB", "valueB");
  CheckGet("nameA", "valueA");
  CheckGet("nameB", "valueB");
  set_invalid_value("valueA");
  CheckNotFound("nameA");
  CheckGet("nameB", "valueB");
}

TEST_F(AprMemCacheTest, SizeTest) {
  ASSERT_FALSE(fail_);

  for (int x = 0; x < 10; ++x) {
    for (int i = 128; i < (1<<20); i += i) {
      GoogleString value(i, 'a');
      GoogleString key = StrCat("big", IntegerToString(i));
      CheckPut(key.c_str(), value.c_str());
      CheckGet(key.c_str(), value.c_str());
    }
  }
}

}  // namespace net_instaweb
