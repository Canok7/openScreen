//
// Created by canok on 2022/7/21.
//

#include "aacUtils.h"

//想用Mediacodec　，得遵循mediacodec的规则，什么样的规则？没有说明，只能看源码。
//以下函数，可以创建　aac　的csd-0 参数，从android源码拷贝来的。
//frome framworks/av/media/libstagefright/MeteDataUntils.cpp  ,  android mediacodec 专门为部分编解码器设置的 Codec specific data,特殊信息，什么样的格式？？？看android 源码
bool aacUtils::MakeAACCodecSpecificData(AMediaFormat *meta, unsigned profile,
                                        unsigned sampling_freq_index,
                                        unsigned channel_configuration) {

    if (sampling_freq_index > 11u) {
        return false;
    }

    uint8_t csd[2];
    csd[0] = ((profile + 1) << 3) | (sampling_freq_index >> 1);
    csd[1] = ((sampling_freq_index << 7) & 0x80) | (channel_configuration << 3);

    static const int32_t kSamplingFreq[] = {
            96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
            16000, 12000, 11025, 8000
    };
    int32_t sampleRate = kSamplingFreq[sampling_freq_index];

    // AMediaFormat_setBuffer(meta, AMEDIAFORMAT_KEY_CSD_0, csd, sizeof(csd));//api 28
    AMediaFormat_setBuffer(meta, "csd-0", csd, sizeof(csd));
    // AMediaFormat_setString(meta, AMEDIAFORMAT_KEY_MIME, MEDIA_MIMETYPE_AUDIO_AAC);
    AMediaFormat_setInt32(meta, AMEDIAFORMAT_KEY_SAMPLE_RATE, sampleRate);
    AMediaFormat_setInt32(meta, AMEDIAFORMAT_KEY_CHANNEL_COUNT, (int32_t) channel_configuration);

    return true;
}