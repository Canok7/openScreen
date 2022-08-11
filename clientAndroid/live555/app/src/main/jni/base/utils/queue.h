#if 0// now , it's abandoned ,please use MediaBuffer+MediaQueue


#ifndef  __QUEUE_HEAD_H__
#define __QUEUE_HEAD_H__

#include <pthread.h>
#include <semaphore.h>

typedef unsigned char uint8_t;

#define MIN(a, b) (a)<(b)?(a):(b)
//#define SIGLEFRAME_LEN (1024*125) //125K
#define SIGLEFRAME_LEN (1024*512) //512k

typedef struct _dataInfo_{
    struct timeval presentationTime;
    unsigned durationInMicroseconds;
    uint64_t dts;
    char codeName[5];
} DATA_INFO;
typedef struct _QUEUE_NODE_ {
    pthread_mutex_t mlock;
    int datalen;
    uint8_t data[SIGLEFRAME_LEN];

    DATA_INFO info;
} QUEUE_NODE;


#define EN_LOG_FILE 0

class CQueue {
public :
    CQueue(int frames, const char *name = NULL);//the name max len 128
    ~CQueue();

    /*
    * \\no block
    *\\ pram: data, the dest data buffer, len , dest data buffer lenth,
    *\\ return : real  output data lenth
    */
    int pop(uint8_t *data, int len);


    /*
    *\\ pram: data, the src data, len , the src data lenth
    *\\ return : real  input data lenth
    */
    int push(uint8_t *data, int len, DATA_INFO info);

    /*
    * \\ breif :get the data buffer pointer whith block, Manual release is required after use
    *\\  param : **pdata , *plen :the queue data len
    *\\  return -1; no data to get. other the que index will be return ,
    */
    int getbuffer(uint8_t **pdata, int *plen, DATA_INFO &info);

    /*
    *\\ breif : get back the que data buffer
    *\\ pram:the que index
    *\\ return : 0
    */
    int releasebuffer(int index);

    int bufferCounts();

private:
    void addInindex();

    void addOutindex();

    int IncreaseEffectives();

    int reduceEffectives();

private:

    char sName[128];
    int maxFrames;
    QUEUE_NODE *que;
    int mInindex;
    int mOutindex;
    int mEffectives;
    sem_t mSem;
#if EN_LOG_FILE
    FILE* fp_log;
#endif
    pthread_mutex_t mIndexlock;
};

#include <map>
#include <string>
typedef std::map<std::string, CQueue*> DataMap;
#endif

#endif
