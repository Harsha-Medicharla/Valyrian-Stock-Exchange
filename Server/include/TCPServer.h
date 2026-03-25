#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <netinet/in.h>
#include <sys/epoll.h>

#include "../../EMS/queue/RingBuffer.h"
#include "../../EMS/model/OrderRequest.h"
#include "../../EMS/model/ClientResponse.h"

namespace Server {

template<size_t RingSize>
class TCPServer {
public:
    TCPServer(uint16_t port, 
              RingBuffer<EMS::model::OrderRequest, RingSize>& q1,
              RingBuffer<EMS::model::ClientResponse, RingSize>& q4)
        : port_(port), running_(false), q1_(q1), q4_(q4) {}
    ~TCPServer() { stop(); }

    void start();
    void stop();

private:
    void threadLoop();
    void setNonBlocking(int fd);
    void handleNewConnection();
    void handleClientData(int client_fd);
    void pollQ4();

    uint16_t port_;
    int server_fd_{-1};
    int epoll_fd_{-1};
    
    std::atomic<bool> running_;
    std::thread worker_thread_;

    RingBuffer<EMS::model::OrderRequest, RingSize>& q1_;
    RingBuffer<EMS::model::ClientResponse, RingSize>& q4_;

    std::unordered_map<int, uint64_t> socket_to_user_;
    std::unordered_map<uint64_t, int> user_to_socket_;
};

} // namespace Server

#include "../src/TCPServer.tpp"
