#ifndef  __LIVE555_HEAD_H__
#define __LIVE555_HEAD_H__
#include "queue.h"
int live555_start(char *url, char *workdir,bool bTCP,unsigned  udpReorderTimeUs);
int live555_stop();
//for get data
CQueue *live55_getDataQueue();
#endif