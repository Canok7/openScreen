//
// Created by canok on 2022/7/21.
//

#include "timeUtils.h"

uint64_t timeUtils::toUs(struct timeval t) {
    return (t.tv_sec * 1000000 + t.tv_usec);
}

uint64_t timeUtils::getSystemUs() {
    struct timeval t = {0};
    gettimeofday(&t, NULL);
    return toUs(t);
}