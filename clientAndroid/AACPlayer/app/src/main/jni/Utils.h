//
// Created by xiancan.wang on 8/11/21.
//

#ifndef AACPLAYER_UTILS_H
#define AACPLAYER_UTILS_H
#include "media/NdkMediaCodec.h"
#include "media/NdkMediaFormat.h"

class Utils {

public:
    static bool MakeAACCodecSpecificData(AMediaFormat *meta, unsigned profile, unsigned sampling_freq_index,
                                         unsigned channel_configuration);
};


#endif //AACPLAYER_UTILS_H
