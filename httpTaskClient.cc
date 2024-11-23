#include "linuxHeader.h"
#include <cstdio>
#include <netdb.h>
#include <workflow/HttpMessage.h>
#include <workflow/HttpUtil.h>
#include <workflow/WFFacilities.h>
#include <workflow/WFTask.h>
#include <workflow/WFTaskFactory.h>

static WFFacilities::WaitGroup waitGroup(1);

void handsig(int num) {
  waitGroup.done();
  fprintf(stderr, "waitGroup is down\n");
}

int main() {
  signal(SIGINT, handsig);
  WFHttpTask *httptask = WFTaskFactory::create_http_task(
      "http://www.baidu.com", 0, 0, [](WFHttpTask *httptask) {
        protocol::HttpRequest *req = httptask->get_req();
        protocol::HttpResponse *res = httptask->get_resp();

        int state = httptask->get_state();
        int error = httptask->get_error();
        switch (state) {
        case WFT_STATE_SYS_ERROR:
          fprintf(stderr, "system error:%s\n", strerror(error));
          break;
        case WFT_STATE_DNS_ERROR:
          fprintf(stderr, "DNS error:%s\n", gai_strerror(error));
          break;
        case WFT_STATE_SUCCESS:
          break;
        }
        if (state != WFT_STATE_SUCCESS) {
          fprintf(stderr, "failed");
          return;
        }
        fprintf(stderr, "success!\n");
        fprintf(stderr, "%s %s %s\r\n", req->get_method(),
                req->get_http_version(), req->get_request_uri());
        fprintf(stderr, "%s %s %s \r\n", res->get_http_version(),
                res->get_status_code(), res->get_reason_phrase());
        protocol::HttpHeaderCursor reqCursor(req);
        std::string name;
        std::string value;
        while (reqCursor.next(name, value)) {
          fprintf(stderr, "%s %s\r\n", name.c_str(), value.c_str());
        }
        protocol::HttpHeaderCursor respCursor(res);
        while (respCursor.next(name, value)) {
          fprintf(stderr, "%s %s\r\n", name.c_str(), value.c_str());
        }

        const void *body;
        size_t body_len;
        res->get_parsed_body(&body, &body_len);
        fwrite(body, 1, body_len, stderr);
      });
  protocol::HttpRequest *req = httptask->get_req();
  req->add_header_pair("Accept", "*/*");
  req->add_header_pair("User-Agent", "TestAgent");
  req->add_header_pair("Connection", "close");
  httptask->start();
  waitGroup.wait();
  return 0;
}
