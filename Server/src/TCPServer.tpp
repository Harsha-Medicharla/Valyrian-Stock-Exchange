#include "TCPServer.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <vector>

namespace Server {

template<size_t RingSize>
void TCPServer<RingSize>::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        throw std::runtime_error("setsockopt failed");
    }

    setNonBlocking(server_fd_);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr *)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Bind failed");
    }

    if (listen(server_fd_, SOMAXCONN) < 0) {
        throw std::runtime_error("Listen failed");
    }

    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        throw std::runtime_error("epoll_create1 failed");
    }

    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_fd_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &ev) == -1) {
        throw std::runtime_error("epoll_ctl: server_fd");
    }

    running_ = true;
    worker_thread_ = std::thread(&TCPServer<RingSize>::threadLoop, this);
    std::cout << "[TCPServer] Started on port " << port_ << std::endl;
}

template<size_t RingSize>
void TCPServer<RingSize>::stop() {
    running_ = false;
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
    }
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }
    std::cout << "[TCPServer] Stopped" << std::endl;
}

template<size_t RingSize>
void TCPServer<RingSize>::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

template<size_t RingSize>
void TCPServer<RingSize>::threadLoop() {
    const int MAX_EVENTS = 64;
    std::vector<epoll_event> events(MAX_EVENTS);

    while (running_) {
        // Poll Q4 for outbound messages without blocking
        pollQ4();

        // Wait for epoll events with small timeout (1ms)
        int nfds = epoll_wait(epoll_fd_, events.data(), MAX_EVENTS, 1);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd_) {
                handleNewConnection();
            } else {
                handleClientData(events[i].data.fd);
            }
        }
    }
}

template<size_t RingSize>
void TCPServer<RingSize>::handleNewConnection() {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                std::cerr << "[TCPServer] accept failed" << std::endl;
                break;
            }
        }

        setNonBlocking(client_fd);
        
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = client_fd;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            std::cerr << "[TCPServer] epoll_ctl failed for client" << std::endl;
            close(client_fd);
        } else {
            std::cout << "[TCPServer] New client connected FD: " << client_fd << std::endl;
        }
    }
}

template<size_t RingSize>
void TCPServer<RingSize>::handleClientData(int client_fd) {
    EMS::model::OrderRequest order{}; 
    
    while (true) {
        ssize_t bytes_read = read(client_fd, &order, sizeof(EMS::model::OrderRequest));
        
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                std::cerr << "[TCPServer] Client FD " << client_fd << " disconnect with error." << std::endl;
                epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr);
                close(client_fd);
                socket_to_user_.erase(client_fd);
                break;
            }
        } else if (bytes_read == 0) {
            std::cout << "[TCPServer] Client FD " << client_fd << " gracefully disconnected." << std::endl;
            epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr);
            close(client_fd);
            socket_to_user_.erase(client_fd);
            break;
        } else if (bytes_read == sizeof(EMS::model::OrderRequest)) {
            user_to_socket_[order.user_id] = client_fd;
            socket_to_user_[client_fd] = order.user_id;

            if (!q1_.push(order)) {
                std::cerr << "[TCPServer] Q1 Buffer full, dropped order_id: " << order.order_id << std::endl;
            }
        } else {
            std::cerr << "[TCPServer] Partial read " << bytes_read << " bytes" << std::endl;
            break; 
        }
    }
}

template<size_t RingSize>
void TCPServer<RingSize>::pollQ4() {
    EMS::model::ClientResponse response;
    int count = 0;
    while (q4_.pop(response) && count < 10) {
        uint64_t user_id = 0;
        if (response.type == EMS::model::ResponseType::EMS_DECISION) {
            user_id = response.decision.original_request.user_id;
        }

        if (user_to_socket_.count(user_id) > 0) {
            int client_fd = user_to_socket_[user_id];
            ssize_t sent = write(client_fd, &response, sizeof(EMS::model::ClientResponse));
            if (sent == -1) {
                std::cerr << "[TCPServer] Failed to write response to client FD: " << client_fd << std::endl;
            }
        } else {
            std::cerr << "[TCPServer] No connected socket found for user_id: " << user_id << std::endl;
        }
        count++;
    }
}

} // namespace Server
