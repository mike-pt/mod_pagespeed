/*
 * Copyright 2012 Google Inc.
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
// Author: nikhilmadan@google.com (Nikhil Madan)

#include "net/instaweb/rewriter/public/collect_flush_early_content_filter.h"

#include "net/instaweb/rewriter/flush_early.pb.h"
#include "net/instaweb/rewriter/public/rewrite_driver.h"
#include "net/instaweb/rewriter/public/rewrite_options.h"
#include "net/instaweb/rewriter/public/rewrite_test_base.h"
#include "net/instaweb/rewriter/public/server_context.h"
#include "pagespeed/kernel/base/gtest.h"
#include "pagespeed/kernel/base/string.h"
#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/html/html_parse_test_base.h"
#include "pagespeed/kernel/http/content_type.h"

namespace net_instaweb {

class CollectFlushEarlyContentFilterTest : public RewriteTestBase {
 public:
  CollectFlushEarlyContentFilterTest() {}

 protected:
  virtual void SetUp() {
    options()->EnableFilter(RewriteOptions::kFlushSubresources);
    options()->EnableFilter(RewriteOptions::kInlineImportToLink);
    RewriteTestBase::SetUp();
    rewrite_driver()->AddFilters();
  }

  virtual bool AddHtmlTags() const { return false; }

 private:
  DISALLOW_COPY_AND_ASSIGN(CollectFlushEarlyContentFilterTest);
};

TEST_F(CollectFlushEarlyContentFilterTest, CollectFlushEarlyContentFilter) {
  GoogleString html_input =
      "<!doctype html PUBLIC \"HTML 4.0.1 Strict>"
      "<html>"
      "<head>"
        "<script src=\"a.js\">"
        "</script>"
        "<link type=\"text/css\" rel=\"stylesheet\" href=\"a.css\" "
          "media=\"print\"/>"
        "<link type=\"text/css\" rel=\"stylesheet\" href=\"b.css\"/>"
        "<link type=\"text/css\" rel=\"stylesheet\" "
        "href=\"d.css.pagespeed.ce.0.css\" media=\"print\"/>"
        "<link type=\"text/css\" rel=\"stylesheet\" "
        "href=\"http://www.test.com/I.e.css+f.css.pagespeed.cc.0.css\" "
        "media=\"print\"/>"
        "<script src=\"b.js\" type=\"text/javascript\"></script>"
        "<noscript>"
        "<script src=\"c.js\">"
        "</script>"
        "</noscript>"
        "<img src=\"data:image/gif;base64,R0lGODlhAQABAIAAAP\"/>"
      "</head>"
      "<body>"
        "<link type=\"text/css\" rel=\"stylesheet\" href=\"c.css\"/>"
        "<script src=\"c.js\"></script>"
      "</body>"
      "</html>";

  Parse("not_flushed_early", html_input);
  FlushEarlyInfo* flush_early_info = rewrite_driver()->flush_early_info();
  EXPECT_STREQ("<script src=\"http://test.com/a.js\"></script>"
               "<link type=\"text/css\" rel=\"stylesheet\" "
               "href=\"http://test.com/a.css\"/>"
               "<link type=\"text/css\" rel=\"stylesheet\" "
               "href=\"http://test.com/b.css\"/>"
               "<link type=\"text/css\" rel=\"stylesheet\" "
               "href=\"http://test.com/d.css\"/>"
               "<script type=\"text/javascript\" src=\"http://test.com/b.js\">"
               "</script>"
               "<body>"
               "<link type=\"text/css\" rel=\"stylesheet\" "
               "href=\"http://test.com/c.css\"/>"
               "<script src=\"http://test.com/c.js\"></script>"
               "</body>",
               flush_early_info->resource_html());
  EXPECT_STREQ(output_buffer_, html_input);
}

// Without flush_more_resources_early_if_time_permits we ignore custom
// url-valued attributes.
TEST_F(CollectFlushEarlyContentFilterTest, MultipleUrlValuedAttributes) {
  options()->ClearSignatureForTesting();
  options()->AddUrlValuedAttribute(
      "script", "data-src", semantic_type::kScript);
  server_context()->ComputeSignature(options());

  GoogleString html_input =
      "<!doctype html PUBLIC \"HTML 4.0.1 Strict>"
      "<html>"
      "<head>"
        "<script src=\"a.js\" data-src=\"b.js\"></script>"
      "</head>"
      "<body>"
        "<script src=\"c.js\" data-src=\"d.js\"></script>"
      "</body>"
      "</html>";

  Parse("not_flushed_early", html_input);
  FlushEarlyInfo* flush_early_info = rewrite_driver()->flush_early_info();
  EXPECT_STREQ("<script src=\"http://test.com/a.js\"></script>"
               "<body>"
               "<script src=\"http://test.com/c.js\"></script>"
               "</body>",
               flush_early_info->resource_html());
  EXPECT_STREQ(output_buffer_, html_input);
}

TEST_F(CollectFlushEarlyContentFilterTest, WithNoScriptCssAddStyles) {
  GoogleString html_input =
      "<!doctype html PUBLIC \"HTML 4.0.1 Strict>"
      "<html>"
      "<head>"
      "<noscript class=\"psa_add_styles\">"
      "<link rel='stylesheet' href='a.css' type='text/css' media='print'>"
      "</noscript></head>"
      "<body>"
      "<noscript class=\"psa_add_styles\">"
      "<link rel='stylesheet' href='b.css' type='text/css'>"
      "</noscript></body></html>";

  GoogleString expected_output =
       "<link type=\"text/css\" rel=\"stylesheet\" "
       "href=\"http://test.com/a.css\"/>"
       "<body>"
       "<link type=\"text/css\" rel=\"stylesheet\" "
       "href=\"http://test.com/b.css\"/>"
       "</body>";

  options()->ClearSignatureForTesting();
  options()->set_enable_flush_early_critical_css(true);
  server_context()->ComputeSignature(options());
  Parse("noscript_styles_flushed", html_input);
  FlushEarlyInfo* flush_early_info = rewrite_driver()->flush_early_info();
  EXPECT_STREQ(expected_output, flush_early_info->resource_html());
}

TEST_F(CollectFlushEarlyContentFilterTest, WithNoScriptCssAndFlagDisabled) {
  GoogleString html_input =
      "<!doctype html PUBLIC \"HTML 4.0.1 Strict>"
      "<html>"
      "<head>"
      "<noscript id=\"psa_add_styles\">"
      "<link rel='stylesheet' href='a.css' type='text/css' media='print'>"
      "</noscript></head>"
      "<body>"
      "<noscript id=\"psa_add_styles\">"
      "<link rel='stylesheet' href='b.css' type='text/css'>"
      "</noscript></body></html>";

  GoogleString expected_output = "";

  Parse("noscript_styles_not_flushed", html_input);
  FlushEarlyInfo* flush_early_info = rewrite_driver()->flush_early_info();
  EXPECT_STREQ(expected_output, flush_early_info->resource_html());
}

TEST_F(CollectFlushEarlyContentFilterTest, WithInlineInportToLinkFilter) {
  GoogleString html_input =
      "<!doctype html PUBLIC \"HTML 4.0.1 Strict>"
      "<html>"
      "<head>"
        "<style>@import url(assets/styles.css);</style>"
      "</head>"
      "<body>"
      "</body>"
      "</html>";

  Parse("not_flushed_early", html_input);
  FlushEarlyInfo* flush_early_info = rewrite_driver()->flush_early_info();
  EXPECT_STREQ("<link rel=\"stylesheet\" "
               "href=\"http://test.com/assets/styles.css\"/><body></body>",
               flush_early_info->resource_html());
}

TEST_F(CollectFlushEarlyContentFilterTest, RelativeUrlsWithBaseTag) {
  GoogleString html_input =
      "<!doctype html PUBLIC \"HTML 4.0.1 Strict>"
      "<html>"
      "<head>"
        "<link type=\"text/css\" rel=\"stylesheet\" href=\"a.css\"/>"
        "<link type=\"text/css\" rel=\"stylesheet\" href=\"/b.css\"/>"
        "<base href=\"http://test.com/path/\"/>"
        "<link type=\"text/css\" rel=\"stylesheet\" href=\"/c.css\"/>"
      "</head>"
      "<body>"
      "</body>"
      "</html>";

  Parse("not_flushed_early", html_input);
  FlushEarlyInfo* flush_early_info = rewrite_driver()->flush_early_info();
  EXPECT_STREQ("<link type=\"text/css\" rel=\"stylesheet\" "
               "href=\"http://test.com/c.css\"/><body></body>",
               flush_early_info->resource_html());
}

// If an element has only a single resource, flush it early.
TEST_F(CollectFlushEarlyContentFilterTest, ApplySrcAndDataSrc) {
  rewrite_driver()->set_flushing_early(true);
  options()->ClearSignatureForTesting();
  options()->set_flush_more_resources_early_if_time_permits(true);
  options()->AddUrlValuedAttribute("img", "data-src", semantic_type::kImage);
  server_context()->ComputeSignature(options());
  SetResponseWithDefaultHeaders(StrCat(kTestDomain, "1.jpg"),
                                kContentTypeJpeg, "placeholder", 100);
  SetResponseWithDefaultHeaders(StrCat(kTestDomain, "2.jpg"),
                                kContentTypeJpeg, "fullrez", 100);
  GoogleString input_html =
    "<head></head>"
    "<body>"
      "<img src=\"1.jpg\">"
      "<img data-src=\"2.jpg\">"
    "</body>";
  GoogleString output_html =
    "<head></head>"
    "<body>"
      "<img src=\"1.jpg\" pagespeed_size=\"11\">"
      "<img data-src=\"2.jpg\" pagespeed_size=\"7\">"
    "</body>";
  ValidateExpected("flushing_early", input_html, output_html);
  FlushEarlyInfo* flush_early_info = rewrite_driver()->flush_early_info();
  EXPECT_STREQ("", flush_early_info->resource_html());
}

// If an element has multiple url valued attributes but only one resource, flush
// that resource early.
TEST_F(CollectFlushEarlyContentFilterTest, ApplyToPosterAndNotSrcOnVideo) {
  rewrite_driver()->set_flushing_early(true);
  options()->ClearSignatureForTesting();
  options()->set_flush_more_resources_early_if_time_permits(true);
  server_context()->ComputeSignature(options());
  SetResponseWithDefaultHeaders(StrCat(kTestDomain, "1.video"),
                                kContentTypeJpeg, "video", 100);
  SetResponseWithDefaultHeaders(StrCat(kTestDomain, "2.poster"),
                                kContentTypeJpeg, "poster", 100);
  GoogleString input_html =
    "<head></head>"
    "<body>"
      "<video src=\"1.video\" poster=\"2.poster\"></video>"
    "</body>";
  GoogleString output_html =
    "<head></head>"
    "<body>"
      "<video src=\"1.video\" poster=\"2.poster\" pagespeed_size=\"6\"></video>"
    "</body>";
  ValidateExpected("flushing_early", input_html, output_html);
  FlushEarlyInfo* flush_early_info = rewrite_driver()->flush_early_info();
  EXPECT_STREQ("", flush_early_info->resource_html());
}

TEST_F(CollectFlushEarlyContentFilterTest, ApplyIfFlushingEarlyAndOptionSet) {
  rewrite_driver()->set_flushing_early(true);
  options()->ClearSignatureForTesting();
  options()->set_flush_more_resources_early_if_time_permits(true);
  server_context()->ComputeSignature(options());
  SetResponseWithDefaultHeaders(StrCat(kTestDomain, "1.jpg"),
                                kContentTypeJpeg, "image", 100);
  GoogleString input_html =
    "<head></head>"
    "<body>"
      "<img src=\"1.jpg\"/>"
      "<img src=\"data:image/gif;base64,R0lGODlhAQABAIAAAP\"/>"
    "</body>";
  GoogleString output_html =
    "<head></head>"
    "<body>"
      "<img src=\"1.jpg\" pagespeed_size=\"5\"/>"
      "<img src=\"data:image/gif;base64,R0lGODlhAQABAIAAAP\"/>"
    "</body>";
  ValidateExpected("flushing_early", input_html, output_html);
}

TEST_F(CollectFlushEarlyContentFilterTest, DontApplyIfOptionNotSet) {
  rewrite_driver()->set_flushing_early(true);
  SetResponseWithDefaultHeaders(StrCat(kTestDomain, "1.jpg"),
                                kContentTypeJpeg, "image", 100);
  GoogleString input_html =
    "<head></head>"
    "<body>"
      "<img src=\"1.jpg\"/>"
      "<img src=\"data:image/gif;base64,R0lGODlhAQABAIAAAP\"/>"
    "</body>";
  ValidateExpected("flushing_early", input_html, input_html);
}

}  // namespace net_instaweb
