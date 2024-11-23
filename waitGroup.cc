#include "linuxHeader.h"
#include <workflow/WFFacilities.h>
#include <workflow/WFTaskFactory.h>

static WFFacilities::WaitGroup waitGroup(1);

void sigHandler(int num) {
  waitGroup.done();
  fprintf(stderr, "wait group is done\n");
}

int main() {
  signal(SIGINT, sigHandler);
  waitGroup.wait();
}
