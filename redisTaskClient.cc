#include "linuxHeader.h"
#include <cstdio>
#include <workflow/RedisMessage.h>
#include <workflow/WFFacilities.h>
#include <workflow/WFTask.h>
#include <workflow/WFTaskFactory.h>

static WFFacilities::WaitGroup waitGroup(1);

void signalhandle(int num) {
  fprintf(stderr, "ctrl C Signal\n");
  waitGroup.done();
}

int main() {
  signal(SIGINT, signalhandle);
  WFRedisTask *redistask = WFTaskFactory::create_redis_task(
      "redis://127.0.0.1:6379", 0, [](WFRedisTask *redistask) {
        int state = redistask->get_state();
        int error = redistask->get_error();
        protocol::RedisRequest *req = redistask->get_req();
        protocol::RedisResponse *res = redistask->get_resp();
        protocol::RedisValue redisval;
        switch (state) {
        case WFT_STATE_SYS_ERROR:
          fprintf(stderr, "sys error:%s\n", strerror(error));
          break;
        case WFT_STATE_DNS_ERROR:
          fprintf(stderr, "dns error:%s\n", gai_strerror(error));
          break;
        case WFT_STATE_SUCCESS:
          res->get_result(redisval);
          if (redisval.is_error()) {
            fprintf(stderr, "redis error\n");
            state = WFT_STATE_TASK_ERROR;
          }
          break;
        }
        if (state != WFT_STATE_SUCCESS) {
          fprintf(stderr, "state nume error\n");
        } else {
          fprintf(stderr, "success!\n");
        }
        std::string cmd;
        req->get_command(cmd);
        fprintf(stderr, "redis request cmd = %s\n", cmd.c_str());
        if (redisval.is_string()) {
          fprintf(stderr, "redis val is a string,val = %s\n",
                  redisval.string_value().c_str());
        } else if (redisval.is_array()) {
          fprintf(stderr, "redis val is array");
          for (size_t i = 0; i < redisval.arr_size(); ++i) {
            fprintf(stderr, "value at %lu = %s\n", i,
                    redisval.arr_at(i).string_value().c_str());
          }
        }
      });

  protocol::RedisRequest *req = redistask->get_req();

  req->set_request("SET", {"name", "韩立"});
  redistask->start();
  waitGroup.wait();
  return 0;
}
