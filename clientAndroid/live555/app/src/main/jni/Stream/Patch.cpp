//
// Created by canok on 2022/8/20.
//

#include "Patch.h"
#include "utils/MediaQueue.h"

Patch::Patch() {
    mQueue = std::make_unique<MediaQueue>("Pach");
}

void Patch::init(std::string &workdir, const StreamSinkerInfo &info) {

}

void Patch::init(std::string &workdir) {

}

void Patch::pushOneFrame(std::shared_ptr<MediaBuffer> buffer) {
    mQueue->push(buffer);
}


std::shared_ptr<MediaBuffer> Patch::getOneFrame() {
    return mQueue->pop();
}