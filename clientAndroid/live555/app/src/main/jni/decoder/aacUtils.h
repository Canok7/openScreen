//
// Created by canok on 2022/7/21.
//

#ifndef LIVE555_AACUTILS_H
#define LIVE555_AACUTILS_H

#include "media/NdkMediaCodec.h"
#include "media/NdkMediaFormat.h"

class aacUtils {
public:
    static bool MakeAACCodecSpecificData(AMediaFormat *meta, unsigned profile, unsigned sampling_freq_index,
                                         unsigned channel_configuration);
};


#endif //LIVE555_AACUTILS_H
