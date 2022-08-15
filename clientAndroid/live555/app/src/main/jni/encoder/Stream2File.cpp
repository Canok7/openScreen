//
// Created by canok on 2022/8/12.
//

#define LOG_TAG "Stream2File"
#include "utils/logs.h"
#include "Stream2File.h"
#include "utils/MediaQueue.h"
void Stream2File::start(std::shared_ptr<MediaQueue> &queue, std::string &workdir){
    mQueue = queue;
    auto thread_run =  [=,function_name="thread_run"](){
        char fileName[256] ={0};
        snprintf(fileName,sizeof fileName,"%s/stream_%s.out",workdir.c_str(),queue->codecName().c_str());
        mfp = fopen(fileName,"w+");
        if(mfp == nullptr){
            ALOGE("[%s%d] open file err: %s",function_name ,__LINE__,fileName);
            return ;
        }

        ALOGI("[%s%d] destfile:%s",function_name ,__LINE__,fileName);
        while (bRun){
            std::shared_ptr<MediaBuffer> buffer =  mQueue->pop();
            if(buffer!= nullptr){
                ALOGD("[%s%d] write one frame:%ld",function_name ,__LINE__,buffer->size());
                fwrite(buffer->data(),1,buffer->size(),mfp);
            }else{
                std::this_thread::yield();
            }
        }

        fclose(mfp);
    };

    if(mThread == nullptr){
        mThread = std::make_unique<std::thread>(thread_run);
    }
}
Stream2File::~Stream2File(){
    if(mThread!= nullptr){
        bRun = false;
        mThread->join();
    }
//    ALOGD("[%s%d]",__FUNCTION__ ,__LINE__);
}
