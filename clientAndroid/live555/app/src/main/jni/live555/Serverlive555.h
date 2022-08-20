//
// Created by 8216337 on 2022/8/12.
//

#ifndef LIVE555_SERVERLIVE555_H
#define LIVE555_SERVERLIVE555_H

#include <string>

#include <utils/MediaQueue.h>
#include <Stream/StreamSourceInterface.h>

struct ClientInfo {
    std::string name;
    int rtpPort;
    int rtcpPort;
    std::string codecName;
};

class IRtspServerNotifyer {
public:
    enum {
        MSG_NEW_CLIENT,
        MSG_DESTTORYED,
    };

    IRtspServerNotifyer() = default;

    virtual  ~IRtspServerNotifyer() {}

    virtual void onRtspClinetDestoryed(const ClientInfo &info) = 0;

    virtual void onNewClientConnected(const ClientInfo &info) = 0;
};


class Serverlive555 : public IRtspServerNotifyer {
public:
    Serverlive555(std::string workdir, std::shared_ptr<SteamSourceInterface> source);

    virtual ~Serverlive555();

    void start(IRtspServerNotifyer *notifyer, int port = 8554);

    void stop();

    void onRtspClinetDestoryed(const ClientInfo &info) override;

    void onNewClientConnected(const ClientInfo &info) override;

private:
    char volatile bStop = 0;
    std::shared_ptr<SteamSourceInterface> mSource = nullptr;
    std::unique_ptr<std::thread> mThread = nullptr;
    IRtspServerNotifyer *mNotifyer = nullptr;
};


#endif //LIVE555_SERVERLIVE555_H
