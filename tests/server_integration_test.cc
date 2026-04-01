#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../Server/include/TCPServer.h"
#include "../EMS/model/OrderRequest.h"
#include "../Settlement/entities/Confirmation.h"
#include "../Settlement/common/Pool.h"

using namespace Server;
using namespace EMS::model;

class TCPServerTest : public ::testing::Test {
protected:
    static constexpr size_t RING_SIZE = 1024;
    RingBuffer<OrderRequest, RING_SIZE> q1;
    Pool<Confirmation> q5{RING_SIZE};
    std::unique_ptr<TCPServer<RING_SIZE>> server;
    uint16_t port = 8080;

    void SetUp() override {
        server = std::make_unique<TCPServer<RING_SIZE>>(port, q1, q5);
        server->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow server to bind and start
    }

    void TearDown() override {
        if (server) server->stop();
    }
};

TEST_F(TCPServerTest, EndToEndMockTest) {
    // 1. Connect Client
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(client_fd, -1);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    ASSERT_EQ(connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)), 0);

    // 2. Send OrderRequest
    OrderRequest req{};
    req.order_id = 42;
    req.user_id = 7;
    req.symbol = 100;
    req.quantity = 50;

    ssize_t sent = write(client_fd, &req, sizeof(OrderRequest));
    ASSERT_EQ(sent, sizeof(OrderRequest));

    // 3. Verify OrderRequest is in Q1
    OrderRequest popped_req{};
    bool found_in_q1 = false;
    for (int i = 0; i < 50; ++i) { // Retry for 500ms
        if (q1.pop(popped_req)) {
            found_in_q1 = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(found_in_q1);
    EXPECT_EQ(popped_req.order_id, 42);
    EXPECT_EQ(popped_req.user_id, 7);

    // 4. Push Confirmation to Q5
    Confirmation* conf = q5.allocate();
    ASSERT_NE(conf, nullptr);
    conf->trade_id = 1;
    conf->user_id = 7;
    conf->order_id = 42;
    conf->exec_qty = 50;
    conf->status = 1; // FILLED
    
    q5.push(conf);

    // 5. Read response back on Client Socket
    Confirmation received_conf{};
    // Use select to wait with timeout
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_fd, &read_fds);
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    int ret = select(client_fd + 1, &read_fds, NULL, NULL, &tv);
    ASSERT_GT(ret, 0) << "Timeout waiting for Server to reply";

    ssize_t received = read(client_fd, &received_conf, sizeof(Confirmation));
    ASSERT_EQ(received, sizeof(Confirmation));
    EXPECT_EQ(received_conf.order_id, 42);
    EXPECT_EQ(received_conf.user_id, 7);
    EXPECT_EQ(received_conf.exec_qty, 50);

    close(client_fd);
}
