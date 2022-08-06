//
// Created by Administrator on 2022/7/31.
//

#include "CHandler.h"

void CHandler::deliverMessage(const std::shared_ptr<CMessage> &msg) {
    onMessageReceived(msg);
    mMessageCounter++;
}