//
// Created by xiancan.wang on 8/10/21.
//

#include "AACFileparser.h"
#include "Logs.h"

#define MAKE64_LEFT(d0,d1,d2,d3,d4,d5,d6,d7) ((int64_t)(d0)<<56)|((int64_t)(d1)<<48)|((int64_t)(d2)<<40)|((int64_t)(d3)<<32)|((int64_t)(d4)<<24)|((int64_t)(d5)<<16)|((int64_t)(d6)<<8)|(int64_t)(d7)
#define MAKE64_LEFT_FROM_ARRY(d) MAKE64_LEFT(d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7])
//from 0 start
#define GET64BIT_LEFT(data,start,len) (data)>>(64-start-len)&((1<<len) -1)

#define MAKE32_LEFT(d0,d1,d2,d3) ((int32_t)(d0)<<24)|((int32_t)(d1)<<16)|((int32_t)(d2)<<8)|(int32_t)(d3)
#define GET32BIT_LEFT(data,start,len) (data)>>(32-start-len)&((1<<len)-1)
static unsigned const samplingFrequencyTable[16] = {
        96000, 88200, 64000, 48000,
        44100, 32000, 24000, 22050,
        16000, 12000, 11025, 8000,
        7350, 0, 0, 0
};
AACFileparser::AACFileparser(const char*file, bool loop): bLoop(loop){
    fp_in = fopen(file,"r");
    if(fp_in == NULL){
        ALOGD("[%s%d] fopen err:%s",__FUNCTION__ ,__LINE__,file);
    }
}
AACFileparser::~AACFileparser(){

}

int AACFileparser::getOneFrameWithoutADTS(unsigned char *dest, int buflen){

    //固定７Byte,如果　protection_absent＝0，则有CRC,CRC占最后两个字节，则ADTS头为７＋２个字节,
    unsigned char headers[7]={0};
    if (fread(headers, 1, sizeof headers, fp_in) < sizeof(headers)
        || feof(fp_in) || ferror(fp_in)) {
        // The input source has ended:
        if(bLoop) {
            fseek(fp_in, 0,SEEK_SET);
            return getOneFrameWithoutADTS(dest,buflen);
        } else{
            return -1;
        }
    }

    int64_t  iheader=MAKE64_LEFT(headers[0],headers[1],headers[2],headers[3],
            headers[4],headers[5],headers[6],0);
    //ALOGD("[%s%d]%#lx",__FUNCTION__ ,__LINE__,iheader);
    int id = GET64BIT_LEFT(iheader,12,1);
    int layer = GET64BIT_LEFT(iheader,13,2);
    int protection_absent = GET64BIT_LEFT(iheader,15,1);
    int profile = GET64BIT_LEFT(iheader,16,2);
    int sampling_frequency_index = GET64BIT_LEFT(iheader,18,4);
    int private_bt = GET64BIT_LEFT(iheader,22,1);
    int channel_configuration = GET64BIT_LEFT(iheader,23,3);
    int original_copy = GET64BIT_LEFT(iheader,26,1);
    int home = GET64BIT_LEFT(iheader,27,1);

    //adts_variable_header
    int copyright_identification_bit = GET64BIT_LEFT(iheader,28,1);
    int copyright_identification_start=GET64BIT_LEFT(iheader,29,1);
    int aac_frame_length =GET64BIT_LEFT(iheader,30,13);
    int adts_buffer_fullness=GET64BIT_LEFT(iheader,33,11);
    int number_of_raw_data_blocks_in_frame=GET64BIT_LEFT(iheader,44,2);

    // If there's a 'crc_check' field, skip it:
    int headerlen = sizeof(headers);
    //protection_absent：无CRC设置为1，有CRC设置为0, CRC　占最后两个字节
    if(!protection_absent) {//if there is CRC, skip it!
        fseek(fp_in, 2, SEEK_CUR);
        headerlen= sizeof(headers)+2;
    }
    int raw_datalen = aac_frame_length - headerlen;
    if(buflen < raw_datalen){
        ALOGE("[%s%d] buf too low !! :raw_datalen:%d,buflen:%d",__FUNCTION__ ,__LINE__,raw_datalen,buflen);
        fseek(fp_in,raw_datalen,SEEK_CUR);
        return 0;
    }else{
        ALOGD("[%s%d] get data:%d,channel:%d,frequecy:%d",__FUNCTION__ ,__LINE__,raw_datalen,channel_configuration,samplingFrequencyTable[sampling_frequency_index]);
        return fread(dest,1,raw_datalen,fp_in);
    }
}

int AACFileparser::probeInfo(int *channel, int *samplingFrequency, int *sampleFreInd, int *iprofile){
    do {
        // Now, having opened the input file, read the fixed header of the first frame,
        // to get the audio stream's parameters:
        unsigned char fixedHeader[4]; // it's actually 3.5 bytes long
        if (fread(fixedHeader, 1, sizeof fixedHeader, fp_in) < sizeof fixedHeader) break;

        // Check the 'syncword':
        if (!(fixedHeader[0] == 0xFF && (fixedHeader[1]&0xF0) == 0xF0)) {
            ALOGD("Bad 'syncword' at start of ADTS file");
            break;
        }


        int i=0;
        for(i=0;i<4;i++){
            ALOGD("%d %#x",i,fixedHeader[i]);
        }
        int32_t  header=MAKE32_LEFT(fixedHeader[0],fixedHeader[1],fixedHeader[2],fixedHeader[3]);
        ALOGD("[%s%d]%#x",__FUNCTION__ ,__LINE__,header);
        int id = GET32BIT_LEFT(header,12,1);
        int layer = GET32BIT_LEFT(header,13,2);
        int protection_absent = GET32BIT_LEFT(header,15,1);
        int profile = GET32BIT_LEFT(header,16,2);
        int sampling_frequency_index = GET32BIT_LEFT(header,18,4);
        int private_bt = GET32BIT_LEFT(header,22,1);
        int channel_configuration = GET32BIT_LEFT(header,23,3);
        int original_copy = GET32BIT_LEFT(header,26,1);
        int home = GET32BIT_LEFT(header,27,1);

        if (profile == 3) {
             ALOGD("Bad (reserved) 'profile': 3 in first frame of ADTS file");
            break;
        }
        ALOGD("[%s%d] id:%d,layer:%d,protexiton_absent:%d,profile:%d,sampling_frequency_index:%d,channel_configuration:%d",__FUNCTION__ ,__LINE__,id,layer,protection_absent,profile,sampling_frequency_index,channel_configuration);
        if (samplingFrequencyTable[sampling_frequency_index] == 0) {
            ALOGD("Bad 'sampling_frequency_index' in first frame of ADTS file");
            break;
        }
        if(samplingFrequency){
            *samplingFrequency = samplingFrequencyTable[sampling_frequency_index];
        }
        if(sampleFreInd){
            *sampleFreInd = sampling_frequency_index;
        }
        if(channel){
            *channel  = channel_configuration;
        }
        if(iprofile){
            *iprofile = profile;
        }

        // If we get here, the frame header was OK.
        // Reset the fid to the beginning of the file:
        fseek(fp_in, 0,SEEK_SET);
    }while(0);
    return 0;
}
