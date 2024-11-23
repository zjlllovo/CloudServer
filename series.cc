#include "linuxHeader.h"
#include <cstdio>
#include <workflow/RedisMessage.h>
#include <workflow/WFFacilities.h>
#include <workflow/WFTask.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/Workflow.h>

static WFFacilities::WaitGroup waitGroup(1);

void sigHandle(int num) {
  fprintf(stderr, "Ctrl C signal shutdown\n");
  waitGroup.done();
}

void seriesCallback(const SeriesWork *series) {
  fprintf(stderr, "series callback on\n");
  std::string *pkey = static_cast<std::string *>(series->get_context());
  delete pkey;
}

void callback(WFRedisTask *redistask) {
  int state = redistask->get_state();
  int error = redistask->get_error();

  protocol::RedisRequest *Req = redistask->get_req();
  protocol::RedisResponse *Res = redistask->get_resp();

  protocol::RedisValue value;

  switch (state) {
  case WFT_STATE_SYS_ERROR:
    fprintf(stderr, "sys error!\n");
    break;
  case WFT_STATE_DNS_ERROR:
    fprintf(stderr, "dns error\n");
  case WFT_STATE_SUCCESS:
    Res->get_result(value);
    fprintf(stderr, "success!\n");
  }
  std::string cmd;
  Req->get_command(cmd);

  if (cmd == "SET") {
    fprintf(stderr, "first redistask callback begin\n");
    std::string *pkey = static_cast<std::string *>(redistask->user_data);

    WFRedisTask *secondTask =
        WFTaskFactory::create_redis_task("redis://127.0.0.1:6379", 0, callback);
    protocol::RedisRequest *secondReq = secondTask->get_req();
    secondReq->set_request("GET", {*pkey});
    SeriesWork *series = series_of(redistask);
    series->set_context(static_cast<void *>(pkey)); // 把pkey放到序列的共享内存
    series->set_callback(seriesCallback);
    series->push_back(secondTask);
    fprintf(stderr, "second redistask callback end\n");

  } else {
    fprintf(stderr, "first redistask callback begin\n");
    fprintf(stderr, "redis request:cmd = %s\n", cmd.c_str());
    if (value.is_string()) {
      fprintf(stderr, "value is a string = %s\n", value.string_value().c_str());
    } else if (value.is_array()) {
      fprintf(stderr, "value is array\n");
      for (size_t i = 0; i < value.arr_size(); ++i) {
        fprintf(stderr, "value at %lu = %s\n", i,
                value.arr_at(i).string_value().c_str());
        fprintf(stderr, "second redistask callback begin\n");
      }
    }
  }
}
int main() {
  signal(SIGINT, sigHandle);
  std::string *pkey = new std::string("name");
  WFRedisTask *firstredistask =
      WFTaskFactory::create_redis_task("redis://127.0.0.1:6379", 0, callback);
  protocol::RedisRequest *req = firstredistask->get_req();
  req->set_request("SET", {*pkey, "zjl"});
  firstredistask->user_data = static_cast<void *>(pkey);

  firstredistask->start();
  waitGroup.wait();

  return 0;
}

// 思考这个代码分三个递进
//  redis 的key为固定字符
//  redis 的key为变量字符
//  key为 堆空间怎么回收内存
