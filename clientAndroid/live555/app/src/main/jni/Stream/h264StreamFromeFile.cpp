//
// Created by Administrator on 2022/8/16.
//

#define LOG_TAG "h264StreamFormeFile"

#include "utils/logs.h"
#include "h264StreamFromeFile.h"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>


#define MIN(a, b) ((a)<(b)?(a):(b))

#define NALU_TYPE_SLICE 1
#define NALU_TYPE_DPA 2
#define NALU_TYPE_DPB 3
#define NALU_TYPE_DPC 4
#define NALU_TYPE_IDR 5
#define NALU_TYPE_SEI 6
#define NALU_TYPE_SPS 7
#define NALU_TYPE_PPS 8
#define NALU_TYPE_AUD 9
#define NALU_TYPE_EOSEQ 10
#define NALU_TYPE_EOSTREAM 11
#define NALU_TYPE_FILL 12

#define CACH_LEN (1024*1000)//缓冲区不能设置太小，如果出现比某一帧比缓冲区大，会被覆盖掉一部分

static int I_count = 0;
static int PB_count = 0;
static int All_count = 0;
static int SPS_count = 0;
static int PPS_count = 0;
static int AUD_count = 0;//分隔符
__attribute__((unused)) static int checkNal(uint8_t nalHeader) {
    All_count++;
    char type = nalHeader & ((1 << 5) - 1);
    switch (type) {
        case NALU_TYPE_SPS:
            PPS_count++;
            printf("sps\n");
            break;
        case NALU_TYPE_PPS:
            SPS_count++;
            printf("pps\n");
            break;
        case NALU_TYPE_IDR:
            I_count++;
            printf("I slice !!!!!!!!!!!!!!\n");
            break;
        case NALU_TYPE_SLICE:
            PB_count++;
            printf("B/P slice\n");
            break;
        case NALU_TYPE_AUD:// 结束符，没有实际数据
            AUD_count++;
            printf("Delimiter==========\n");
            break;
        default:
            printf("type :%d\n", type);
    }
    return type;
}

static int checkFlag(uint8_t *buffer, int offset) {
    static uint8_t mMark[4] = {0x00, 0x00, 0x00, 0x01};
    return !memcmp(buffer + offset, mMark, 4);
    //return (!memcmp(buffer+offset,mMark,4) && ((buffer[offset+4]&((1<<5)-1)) == 9) );
}

std::shared_ptr<MediaBuffer> h264StreamFromeFile::h264_getOneNal() {
    if (mfp == nullptr) {
        ALOGE("[%s%d]source file open failed!", __FUNCTION__, __LINE__);
        return nullptr;
    }
    int i = 0;
    int startpoint = ioffset;
    int endpoint = ioffset;
    for (i = ioffset + 4; i <= CACH_LEN - 4; i++) {
        if (checkFlag(mBuffers[icach], i)) {
            startpoint = ioffset;
            endpoint = i;
            break;
        }
    }
    if (endpoint - startpoint > 0) {
        int dataLen = endpoint - startpoint;
        std::shared_ptr<MediaBuffer> buf = std::make_shared<MediaBuffer>(dataLen);
        memcpy(buf->data(), mBuffers[icach] + startpoint, dataLen);
        ioffset = endpoint;
        return buf;
    } else {
        int oldLen = CACH_LEN - startpoint;
        memcpy(mBuffers[(icach + 1) % 2], mBuffers[icach] + startpoint, oldLen);

        int newLen = 0;
        newLen = fread(mBuffers[(icach + 1) % 2] + oldLen, 1, CACH_LEN - (oldLen), mfp);
        if (newLen < CACH_LEN - (oldLen)) {
            if (bLoop) {
                fseek(mfp, 0, SEEK_SET);
                ioffset = 0;
                icach = 0;
                fread(mBuffers[icach], 1, CACH_LEN, mfp);
                return h264_getOneNal();
            } else {
                //将未填充数据空间置0,并且继续
                memset(mBuffers[(icach + 1) % 2] + oldLen + newLen, 0,
                       CACH_LEN - (oldLen) - newLen);
                if (newLen <= 0) {//表示上一次已经完全读完
                    return nullptr;
                }
            }
        }

        ioffset = 0;
        icach = (icach + 1) % 2;

        return h264_getOneNal();
    }

}


h264StreamFromeFile::h264StreamFromeFile(const std::string &filename, bool loop) : bLoop(loop) {
    mfp = fopen(filename.c_str(), "r");
    if (mfp == nullptr) {
        ALOGD("[%s%d] fopen err:%s", __FUNCTION__, __LINE__, filename.c_str());
    }

    for (int i = 0; i < sizeof mBuffers / sizeof mBuffers[0]; i++) {
        mBuffers[i] = (uint8_t *) malloc(CACH_LEN);
    }
    if (mfp) {
        if (fread(mBuffers[icach], 1, CACH_LEN, mfp) < CACH_LEN) {
            ALOGD("intpufile too short [%d%s]\n", __LINE__, __FUNCTION__);
            fclose(mfp);
        }
    }
}

h264StreamFromeFile::~h264StreamFromeFile() {
    free(mBuffers[0]);
    free(mBuffers[1]);
    if (mfp) {
        fclose(mfp);
        mfp = nullptr;
    }
}

void h264StreamFromeFile::init(std::string &workdir) {

}

std::shared_ptr<MediaBuffer> h264StreamFromeFile::getOneFrame() {
    return h264_getOneNal();
}