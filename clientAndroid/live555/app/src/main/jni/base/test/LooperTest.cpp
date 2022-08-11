//
// Created by Administrator on 2022/8/1.
//

#define LOG_TAG "LooperTest"
#include <base/utils/logs.h>
#include <base/CHandler.h>
#include <base/CMessage.h>
#include <base/CLooper.h>
#include <unistd.h>


class LooperTest : public CHandler{
public:
    enum{
        MSG_SAY,
        MSG_SUM,
    };
    LooperTest() = default;
    virtual ~LooperTest() = default;
    void onMessageReceived(const std::shared_ptr<CMessage> &msg) override{
        switch (msg->what()) {
            case MSG_SAY:
            {
                std::string say_Str;
                int32_t say_Int=0;
                CHECK(msg->findString("say_Str", &say_Str));
                CHECK(msg->findInt32("say_Int", &say_Int));
                ALOGD("[%s%d](%d) say_Str.c_str():%s,say_Int:%d\n",__FUNCTION__,__LINE__,gettid(),say_Str.c_str(),say_Int);
            }
                break;
            case MSG_SUM:
            {
                //get reply from msg
                std::shared_ptr<CReplyToken> replyID;
                CHECK(msg->senderAwaitsResponse(&replyID));

                //do work
                int32_t add1=0,add2=0;
                msg->findInt32("add1", &add1);
                msg->findInt32("add2", &add2);
                ALOGD("[%s%d](%d) to sum:%d,%d\n",__FUNCTION__,__LINE__,gettid(),add1,add2);

                //post resut
                std::shared_ptr<CMessage> response = std::make_shared<CMessage>();
                response->setInt32("result", add1+add2);
                response->postReply(replyID);
            }
                break;
            default:
                ALOGD("unKnow msg:%d\n",msg->what());
                break;
        }
    }
};
enum{
    TEST_THREAD,
    TEST_SP,
    TEST_LOOPER,
    TEST_ABUFFER_LIST,
};

#if 1
int main(int argc, const char*argv[]){

    if(argc<3){
        printf("usage:testModuleType,testCase\n");
        printf("TEST_THREAD:%d\n",TEST_THREAD);
        printf("TEST_SP:%d\n",TEST_SP);
        printf("TEST_LOOPER:%d\n",TEST_LOOPER);
        printf("TEST_ABUFFER_LIST:%d\n",TEST_ABUFFER_LIST);
        return -1;
    }

    int testModule=atoi(argv[1]);
    int testCase=atoi(argv[2]);
    (void)testModule;
#else
int test_looper(){
    int testCase =0;
#endif
    int count=0;
    std::shared_ptr<CLooper> mSink_Loop = std::make_shared<CLooper>();
    std::shared_ptr<LooperTest> mHander = std::make_shared<LooperTest>();

    mSink_Loop->setName("test_looper");
    mSink_Loop->start();

    mSink_Loop->registerHandler(mHander);

    if(testCase == 0){
//        sp<AMessage> msg = new AMessage(testHandler::MSG_SAY, handler);
        std::shared_ptr<CMessage> msg = std::make_shared<CMessage>(LooperTest::MSG_SAY,mHander);
        msg->setString("say_Str", "hello");
        msg->setInt32("say_Int", count++);
        ALOGD("[%s%d](%d) post message\n",__FUNCTION__,__LINE__,gettid());
        msg->post();
        while(1){
            msg->setInt32("say_Int", count++);
            ALOGD("[%s%d](%d) post measge\n",__FUNCTION__,__LINE__,gettid());
            msg->post();
            usleep(1000*1000*1);
        }
    }else if(testCase == 1){
        std::shared_ptr<CMessage> msg = std::make_shared<CMessage>(LooperTest::MSG_SUM,mHander);
        msg->setInt32("add1", 7);
        msg->setInt32("add2", 8);
        ALOGD("[%s%d](%d) postAndAwaitResponse\n",__FUNCTION__,__LINE__,gettid());
        std::shared_ptr<CMessage> response;
        //this will make a reply,and wait !!
        status_t err = msg->postAndAwaitResponse(&response);
        ALOGD("[%s%d] -----debug %d err ",__FUNCTION__ ,__LINE__,err);
        if (err == OK) {
            int ret=0;
            response->findInt32("result", &ret);
            ALOGD("[%s%d]xxxxxxxxxxxx(%d) get add ret:%d\n",__FUNCTION__,__LINE__,gettid(),ret);
        }
    }
    return 0;
}