#pragma once

#include "ProcCommunicator.h"

class ServerProcCommunicator : public ProcCommunicator
{
public:
    ServerProcCommunicator(const std::string &shMemName);
    ServerProcCommunicator(const std::string &shMemName,
                           std::unique_ptr<ISharedMemorySender> sender,
                           std::unique_ptr<ISharedMemoryReceiver> receiver);
    virtual ~ServerProcCommunicator();

    void send(const Message *msg);
    Message *receive();
    bool isValid() const;
};
