#if 0
#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <logs.h>

#include "queue.h"

#define LOOP_ADD(i, r) ((i)+1)>=(r)?0:(i+1)

#define ERRO_INFO(fmt, args...) do {printf("[%s,%s,%d]",__FILE__,__FUNCTION__,__LINE__);printf(fmt,##args);} while(0)
#if 1
//#define DEBUG(fmt, args...)		do {printf("[%s,%s,%d]",__FILE__,__FUNCTION__,__LINE__);printf(fmt,##args);} while(0)
#define DEBUG(...)  __android_log_print(ANDROID_LOG_INFO,"queue",__VA_ARGS__)
#else
#define DEBUG(fmt, args...)  
#endif
#define DEBUG_DIR  "/storage/emulated/0/"
#define LOG_FILE(name) DEBUG_DIR name

CQueue::CQueue(int frames, const char *name) :
        maxFrames(frames),
        mInindex(0),
        mOutindex(0),
        mEffectives(0) {
    if (name) {
        snprintf(sName, sizeof(sName), "%s", name);
    } else {
        snprintf(sName, sizeof(sName), "log_maxFrames%02d", maxFrames);
    }
#if EN_LOG_FILE
    //fp_log = fopen(LOG_FILE(sName),"w+");
    char filepath[128];
    snprintf(filepath,sizeof(filepath),"%s/%s",DEBUG_DIR,sName);
    fp_log = fopen(filepath,"w+");
    if(fp_log == NULL)
    {
        DEBUG("fopen erro");
    }
    else
    {
        setbuf(fp_log,0); //立即写入，不缓存
    }
#endif
    que = (QUEUE_NODE *) malloc(frames * sizeof(QUEUE_NODE));
    // 如果这里不先 memset 调用一下，在某些系统上这里其实并没有真正申请内存，
    // 只有在正在往这个内存写数据的时候才申请，导致误以为内存一直在增长，感觉像内存泄漏，
    // 所以还是set一下比较好,真正做到一次性申请内存。
    memset(que, 0, frames * sizeof(QUEUE_NODE));
    if (que == NULL) {
        printf("erro to malloc\n");
    }

    pthread_mutexattr_t mutextattr;
    pthread_mutexattr_init(&mutextattr);
    // 设置互斥锁在进程之间共享
    pthread_mutexattr_setpshared(&mutextattr, PTHREAD_PROCESS_SHARED);
    int i = 0;
    for (i = 0; i < maxFrames; i++) {
        pthread_mutex_init(&que[i].mlock, &mutextattr);
    }
    pthread_mutex_init(&mIndexlock, &mutextattr);
    sem_init(&mSem, 0, 0);
}

CQueue::~CQueue() {
    int i = 0;
    for (i = 0; i < maxFrames; i++) {
        pthread_mutex_destroy(&que[i].mlock);
    }
    pthread_mutex_destroy(&mIndexlock);
    sem_destroy(&mSem);
    if (que != NULL) {
        free(que);
        que = NULL;
    }
}

int CQueue::pop(uint8_t *data, int len) {
    //DEBUG("in %d out %d len %d\n ",mInindex,mOutindex,len);

    //sem_wait(&mSem);
    sem_trywait(&mSem);
    int iout = mOutindex;

    pthread_mutex_t *ilock = &que[iout].mlock;
    pthread_mutex_lock(ilock);

    if (reduceEffectives() == 0) {
        mEffectives = 0;
        pthread_mutex_unlock(ilock);
        DEBUG("no data\n");
        return 0;
    }

    int copylen = MIN(len, que[iout].datalen);
    if (data != NULL) {
        memcpy(data, que[iout].data, copylen);
    }

    addOutindex();
    pthread_mutex_unlock(ilock);
    return copylen;
}

int CQueue::push(uint8_t *data, int len, DATA_INFO info) {
    //DEBUG("in %d out %d len %d\n ",mInindex,mOutindex,len);
    int iIn = mInindex;
    pthread_mutex_t *ilock = &que[iIn].mlock;

    pthread_mutex_lock(ilock);
    DEBUG("push   mEffectives %d ,maxFrames %d\n", mEffectives, maxFrames);
#if EN_LOG_FILE
    if(fp_log)
    {
        struct timeval test_time;
        struct tm *st_tm = NULL;
        gettimeofday(&test_time,NULL);
        st_tm = gmtime(&test_time.tv_sec);
        long long m0 = test_time.tv_usec;

        fprintf(fp_log,"IN>>>mEffectives:%02d,lenth:%08d [%02d:%02d:%02d:%06lu] \n",mEffectives,len,st_tm->tm_hour,st_tm->tm_min,st_tm->tm_sec,test_time.tv_usec);
        //fwrite(data,1,len,fp_log);
    }
#endif
    int copylen = MIN(len, sizeof(que[iIn].data));

    if (data != NULL) {
        que[iIn].info = info;
        memcpy(que[iIn].data, data, copylen);
        que[iIn].datalen = copylen;
        if (copylen < len) {
            DEBUG("Buffer single frame too short ! Buffer_frame_len:%lu,data_len:%d \n ",
                  sizeof(que[iIn].data), len);
        }
    }

    addInindex();
    sem_post(&mSem);
    if (IncreaseEffectives() == maxFrames) {//覆盖老数据
        DEBUG("cover  mEffectives %d ,maxFrames %d\n", mEffectives, maxFrames);
        //DEBUG("cover  mInindex %d ,mOutindex %d\n",mInindex,mOutindex);
        sem_trywait(&mSem);
        addOutindex();
    }
    pthread_mutex_unlock(ilock);


    return copylen;

}

int CQueue::getbuffer(uint8_t **pdata, int *plen,DATA_INFO &info) {
    int val = 0;
    sem_wait(&mSem);
    int iout = mOutindex;

    pthread_mutex_t *ilock = &que[iout].mlock;
    pthread_mutex_lock(ilock);
    //DEBUG("pop  mEffectives %d ,maxFrames %d\n",mEffectives,maxFrames);

    if (reduceEffectives() == 0) {
        *pdata = NULL;
        *plen = 0;
        pthread_mutex_unlock(ilock);
        DEBUG("no data\n");
        return -1;
    }

    *pdata = que[iout].data;
    *plen = que[iout].datalen;
    info = que[iout].info;
#if EN_LOG_FILE
    if(fp_log)
    {
        struct timeval test_time;
        struct tm *st_tm = NULL;
        gettimeofday(&test_time,NULL);
        st_tm = gmtime(&test_time.tv_sec);
        long long m0 = test_time.tv_usec;

        fprintf(fp_log,"out<<<mEffectives:%02d,lenth:%08d [%02d:%02d:%02d:%06lu] \n",mEffectives,*plen,st_tm->tm_hour,st_tm->tm_min,st_tm->tm_sec,test_time.tv_usec);
        //fwrite(data,1,len,fp_log);
    }
#endif

    addOutindex();
    //DEBUG("in %d out %d len %d unlock \n ",mInindex,mOutindex,len);
    //pthread_mutex_unlock(ilock);

    //DEBUG("get index  %d \n",iout);
    return iout;
}

int CQueue::releasebuffer(int index) {
    //DEBUG("releas %d \n",index);
    if (0 <= index && index < maxFrames) {
        pthread_mutex_unlock(&que[index].mlock);
    }
    return 0;
}


void CQueue::addInindex() {
    pthread_mutex_lock(&mIndexlock);
    mInindex = LOOP_ADD(mInindex, maxFrames);
    pthread_mutex_unlock(&mIndexlock);
}

void CQueue::addOutindex() {
    pthread_mutex_lock(&mIndexlock);
    mOutindex = LOOP_ADD(mOutindex, maxFrames);
    pthread_mutex_unlock(&mIndexlock);
}

int CQueue::IncreaseEffectives() {
    pthread_mutex_lock(&mIndexlock);
    int ret = mEffectives;
    mEffectives += 1;
    if (mEffectives > maxFrames) {
        mEffectives = maxFrames;
    }
    pthread_mutex_unlock(&mIndexlock);
    return ret;
}

int CQueue::reduceEffectives() {
    pthread_mutex_lock(&mIndexlock);

    int ret = mEffectives;
    mEffectives -= 1;
    if (mEffectives < 0) {
        mEffectives = 0;
    }
    pthread_mutex_unlock(&mIndexlock);
    return ret;
}

int CQueue::bufferCounts() {
    return mEffectives;
}

#if 0
int main()
{

    CQueue * pQueue = new CQueue(3);

    uint8_t buf1[128]="11111";
    uint8_t buf2[128]="22222";
    uint8_t buf3[128]="33333";
    uint8_t buf4[128]="44444";
    uint8_t buf5[128]="55555";
    uint8_t bufget[128]={0};

    int len = sizeof(bufget);
    pQueue->push(buf1,sizeof(buf1));
#if 1
    pQueue->push(buf2,sizeof(buf2));
    pQueue->push(buf3,sizeof(buf3));
    pQueue->push(buf4,sizeof(buf4));
    pQueue->push(buf5,sizeof(buf5));

    pQueue->pop(bufget,len);
    printf("wang bufget :%s \n",bufget);

    pQueue->pop(bufget,len);
    printf("wang bufget :%s \n",bufget);

    pQueue->pop(bufget,len);
    printf("wang bufget :%s \n",bufget);

    pQueue->pop(bufget,len);
    printf("wang bufget :%s \n",bufget);


    pQueue->pop(bufget,len);
    printf("wang bufget :%s \n",bufget);


    //pQueue->push(buf1,sizeof(buf1));
    //len = sizeof(bufget);
#endif
    //pQueue->pop(bufget,len);
    //printf("wang bufget :%s \n",bufget);

    delete pQueue;

    return 0;
}
#endif
#endif


