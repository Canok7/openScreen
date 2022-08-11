/***

***20190828 canok

*** output: complete frames

**/

#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <string.h>


#define MIN(a, b) ((a)<(b)?(a):(b))

typedef unsigned char uint8_t;     //无符号8位数

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
static uint8_t *g_cach[2] = {NULL, NULL};
static FILE *fp_inH264 = NULL;
static int icach = 0;
static int ioffset = 0;
static int bLoop = 1;
static bool bInit = false;

static int h264_init(const char *filename) {
    if (bInit) {
        return 0;
    } else {
        bInit = true;
    }
    if (g_cach[0] == NULL) {
        g_cach[0] = (uint8_t *) malloc(CACH_LEN);
    }
    if (g_cach[1] == NULL) {
        g_cach[1] = (uint8_t *) malloc(CACH_LEN);
    }

    if (fp_inH264 == NULL) {
        //fp_inH264 = fopen("./live555.video","r");
        fp_inH264 = fopen(filename, "r");
        if (fp_inH264 == NULL) {
            printf("fope erro [%d%s]\n", __LINE__, __FUNCTION__);
            return -1;
        }
    }

    if (fread(g_cach[icach], 1, CACH_LEN, fp_inH264) < CACH_LEN) {
        printf("intpufile too short [%d%s]\n", __LINE__, __FUNCTION__);
        return -1;
    }
    return 0;
}

static int h264_deinit() {
    if (g_cach[0]) {
        free(g_cach[0]);
        g_cach[0] = NULL;
    }
    if (g_cach[1]) {
        free(g_cach[1]);
        g_cach[1] = NULL;
    }

    if (fp_inH264) {
        fclose(fp_inH264);
        fp_inH264 = NULL;
    }

    return 0;
}

static int I_count = 0;
static int PB_count = 0;
static int All_count = 0;
static int SPS_count = 0;
static int PPS_count = 0;
static int AUD_count = 0;//分隔符
static int checkNal(uint8_t nalHeader) {
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

//获取一个Nal到 buf, bufLen表示缓冲区最大可以容纳的数据
//返回实际的帧数据长度
static int h264_getOneNal(uint8_t *buf, int bufLen) {

    int i = 0;
    int startpoint = ioffset;
    int endpoint = ioffset;
    for (i = ioffset + 4; i <= CACH_LEN - 4; i++) {
        if (checkFlag(g_cach[icach], i)) {
            startpoint = ioffset;
            endpoint = i;
            break;
        }
    }
    if (endpoint - startpoint > 0) {
        int dataLen = endpoint - startpoint;
        if (bufLen < dataLen) {
            printf("recive buffer too short , need %d byte!\n", dataLen);
        }
        memcpy(buf, g_cach[icach] + startpoint, MIN(dataLen, bufLen));
        ioffset = endpoint;

        return MIN(dataLen, bufLen);
    } else {
        int oldLen = CACH_LEN - startpoint;
        memcpy(g_cach[(icach + 1) % 2], g_cach[icach] + startpoint, oldLen);

        int newLen = 0;
        newLen = fread(g_cach[(icach + 1) % 2] + oldLen, 1, CACH_LEN - (oldLen), fp_inH264);
        if (newLen < CACH_LEN - (oldLen)) {
            if (bLoop) {
                fseek(fp_inH264, 0, SEEK_SET);
                ioffset = 0;
                icach = 0;
                fread(g_cach[icach], 1, CACH_LEN, fp_inH264);
                return h264_getOneNal(buf, bufLen);
            } else {
                //return -1;
                //将未填充数据空间置0,并且继续
                memset(g_cach[(icach + 1) % 2] + oldLen + newLen, 0, CACH_LEN - (oldLen) - newLen);
                if (newLen <= 0) {//表示上一次已经完全读完
                    return -1;
                }

            }

        }

        ioffset = 0;
        icach = (icach + 1) % 2;

        return h264_getOneNal(buf, bufLen);
    }

}


#if 0
int main()
{
    if(init())
    {
        return -1;
    }
    uint8_t *buffer = (uint8_t*)malloc(CACH_LEN);
    int len =0;
    FILE *fp_out = fopen("out.h264","w+");
    while((len = getOneNal(buffer,CACH_LEN) )> 0)
    {
        printf("get a Nal len:%8d-----",len);
        checkNal(buffer[4]);
        fwrite(buffer,1,len,fp_out);
    }
    fclose(fp_out);
    free(buffer);
    deinit();

    printf("All_count %d\n",All_count);
    printf("I_count %d\n",I_count);
    printf("PB_count %d\n",PB_count);
    printf("AUD_count %d\n",AUD_count);
    printf("SPS_count %d\n",SPS_count);
    printf("PPS_count %d\n",PPS_count);
}
#endif