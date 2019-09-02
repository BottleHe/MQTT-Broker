#include "define.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "Master.h"
#include "logger.h"

using namespace std;
int main(int argc, char* argv[]) {
    LOG_DEBUG("程序开始运行");
    Master master;
    master.Startup();
    return 0;
}