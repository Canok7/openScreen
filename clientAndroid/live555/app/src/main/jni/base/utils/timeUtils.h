//
// Created by canok on 2022/7/21.
//

#ifndef LIVE555_TIMEUTILS_H
#define LIVE555_TIMEUTILS_H


#include <cstdint>
#include <sys/time.h>
#define MIN(a, b) (a)<(b)?(a):(b)
class timeUtils {
public:
    static uint64_t toUs(struct timeval t);

    static uint64_t  getSystemUs();
};


#endif //LIVE555_TIMEUTILS_H
