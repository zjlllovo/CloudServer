#include "linuxHeader.h"
#include <cstdio>
#include <workflow/WFFacilities.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/Workflow.h>

static WFFacilities::WaitGroup waitGroup(1);

void sigHandler(int num) {
  waitGroup.done();
  fprintf(stderr, "wait group is done\n");
}

void parallelCallback(const ParallelWork *parallelWork) {
  fprintf(stderr, "parallelWork callback\n");
}

void httpcallback(WFHttpTask *httptask) { fprintf(stderr, "http call back\n"); }
int main() {
  signal(SIGINT, sigHandler);

  ParallelWork *parallelwork =
      Workflow::create_parallel_work(parallelCallback); // 创建并行任务
  // 创建任务
  // 每个任务创建一个序列
  // 把序列加入并行任务

  std::vector<std::string> vec = {"baidu.com", "jingdong.com", "taobao.com"};
  for (int i = 0; i < vec.size(); i++) {
    WFHttpTask *httptask =
        WFTaskFactory::create_http_task(vec[i], 0, 0, httpcallback);
    SeriesWork *series = Workflow::create_series_work(httptask, nullptr);
    parallelwork->add_series(series);
  }

  parallelwork->start();
  waitGroup.wait();
}
