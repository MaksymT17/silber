#include "ProcCommunicator.h"
#include "ServerProcCommunicator.h"
#include "ClientProcCommunicator.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>
#include <csignal>

static const std::string shared_mem_name{"/shm_test_suite"};

#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed: " << #x << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::exit(1); \
    } \
} while(0)

// Helper to run a watchdog timer
void setup_watchdog(int seconds) {
    alarm(seconds);
}

void cancel_watchdog() {
    alarm(0);
}

// TEST 1: Single Client-Server Roundtrip (Blocking IPC)
void run_test1_server() {
    ServerProcCommunicator server(shared_mem_name);
    Message *req = server.receive();
    if (req) {
        Message resp(req->id, MessageType::HANDSHAKE_OK);
        server.send(&resp);
    }
    std::exit(0);
}

void run_test1_client() {
    ClientProcCommunicator client(shared_mem_name);
    Message req(1, MessageType::HANDSHAKE);
    const Message *resp = nullptr;
    
    bool ok = client.sendRequestGetResponse(&req, &resp);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(resp != nullptr);
    ASSERT_TRUE(resp->id == 1);
    ASSERT_TRUE(resp->type == MessageType::HANDSHAKE_OK);
}

void test1_single_roundtrip() {
    std::cout << "[Test 1] Starting Single Client-Server Roundtrip (Blocking IPC)..." << std::endl;
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        std::exit(1);
    }
    if (pid == 0) {
        // Child: Server
        run_test1_server();
    } else {
        // Parent: Client
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait for server to init
        setup_watchdog(5); // 5 seconds watchdog
        run_test1_client();
        cancel_watchdog();
        int status;
        waitpid(pid, &status, 0);
        std::cout << "[Test 1] PASSED" << std::endl;
    }
}

// TEST 2: Multi-Client Concurrency Simulation
void run_test2_server() {
    ServerProcCommunicator server(shared_mem_name);
    // Handle 5 requests
    for (int i = 0; i < 5; ++i) {
        Message *req = server.receive();
        if (req) {
            Message resp(req->id, MessageType::HANDSHAKE_OK);
            server.send(&resp);
        }
    }
    std::exit(0);
}

void run_test2_client(int id) {
    ClientProcCommunicator client(shared_mem_name);
    Message req(id, MessageType::HANDSHAKE);
    const Message *resp = nullptr;
    
    bool ok = client.sendRequestGetResponse(&req, &resp);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(resp != nullptr);
    ASSERT_TRUE(resp->id == id);
    ASSERT_TRUE(resp->type == MessageType::HANDSHAKE_OK);
    std::exit(0);
}

void test2_multi_client() {
    std::cout << "[Test 2] Starting Multi-Client Concurrency..." << std::endl;
    pid_t server_pid = fork();
    if (server_pid == 0) {
        run_test2_server();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    setup_watchdog(10);
    
    pid_t client_pids[5];
    for (int i = 0; i < 5; ++i) {
        pid_t c_pid = fork();
        if (c_pid == 0) {
            run_test2_client(i + 1); // ID 1 to 5
        }
        client_pids[i] = c_pid;
    }
    
    // Wait for all clients
    for (int i = 0; i < 5; ++i) {
        int status;
        waitpid(client_pids[i], &status, 0);
        ASSERT_TRUE(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }
    
    int server_status;
    waitpid(server_pid, &server_status, 0);
    cancel_watchdog();
    std::cout << "[Test 2] PASSED" << std::endl;
}

// TEST 3: Non-Blocking Timeout Behavior
void run_test3_server() {
    ServerProcCommunicator server(shared_mem_name);
    // Server sleeps before responding to simulate slow calculation / delay
    Message *req = server.receive();
    if (req) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        Message resp(req->id, MessageType::HANDSHAKE_OK);
        server.send(&resp);
    }
    std::exit(0);
}

void run_test3_client() {
    ClientProcCommunicator client(shared_mem_name);
    Message req(1, MessageType::HANDSHAKE);
    const Message *resp = nullptr;
    
    // Set a very short timeout (50ms)
    std::cout << "[Test 3] Calling sendRequestGetResponse with 50ms timeout..." << std::endl;
    auto start = std::chrono::steady_clock::now();
    bool ok = client.sendRequestGetResponse(&req, &resp, 50);
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "[Test 3] Call returned after " << elapsed << " ms. Success=" << ok << std::endl;
    
    // The call should fail due to timeout
    ASSERT_TRUE(!ok);
    std::exit(0);
}

void test3_timeout() {
    std::cout << "[Test 3] Starting Non-Blocking Timeout Test..." << std::endl;
    pid_t server_pid = fork();
    if (server_pid == 0) {
        run_test3_server();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // We expect the client to timeout. However, due to Bug A, it will likely block forever.
    // So we set a watchdog to abort the test if it hangs for more than 3 seconds.
    setup_watchdog(3);
    
    pid_t client_pid = fork();
    if (client_pid == 0) {
        run_test3_client();
    }
    
    int client_status;
    waitpid(client_pid, &client_status, 0);
    cancel_watchdog();
    
    // Kill server if it's still running
    kill(server_pid, SIGKILL);
    int status;
    waitpid(server_pid, &status, 0);
    
    ASSERT_TRUE(WIFEXITED(client_status) && WEXITSTATUS(client_status) == 0);
    std::cout << "[Test 3] PASSED" << std::endl;
}

// TEST 4: Dynamic Transaction ID Stress (Large ID value)
void run_test4_server() {
    ServerProcCommunicator server(shared_mem_name);
    Message *req = server.receive();
    if (req) {
        Message resp(req->id, MessageType::HANDSHAKE_OK);
        server.send(&resp);
    }
    std::exit(0);
}

void run_test4_client() {
    ClientProcCommunicator client(shared_mem_name);
    // Send a message with ID = 100 (which is > MAX_CLIENTS_COUNT)
    Message req(100, MessageType::HANDSHAKE);
    const Message *resp = nullptr;
    
    bool ok = client.sendRequestGetResponse(&req, &resp);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(resp != nullptr);
    ASSERT_TRUE(resp->id == 100);
    std::exit(0);
}

void test4_large_id() {
    std::cout << "[Test 4] Starting Large Transaction ID Test..." << std::endl;
    pid_t server_pid = fork();
    if (server_pid == 0) {
        run_test4_server();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    setup_watchdog(3);
    
    pid_t client_pid = fork();
    if (client_pid == 0) {
        run_test4_client();
    }
    
    int client_status;
    waitpid(client_pid, &client_status, 0);
    cancel_watchdog();
    
    kill(server_pid, SIGKILL);
    int status;
    waitpid(server_pid, &status, 0);
    
    ASSERT_TRUE(WIFEXITED(client_status) && WEXITSTATUS(client_status) == 0);
    std::cout << "[Test 4] PASSED" << std::endl;
}

// TEST 5: Graceful Error Handling (No Exceptions / No Exit)
void test5_error_handling() {
    std::cout << "[Test 5] Starting Graceful Error Handling (No Exceptions / No Exit)..." << std::endl;
    setup_watchdog(3);

    // Initialize with a name that is too long to trigger ENAMETOOLONG in shm_open
    std::string long_name(2000, 'A');
    ClientProcCommunicator client(long_name);
    ASSERT_TRUE(!client.isValid());

    ServerProcCommunicator server(long_name);
    ASSERT_TRUE(!server.isValid());

    // Verify calls on invalid communicators do not crash/throw and return false/nullptr
    const Message *resp = nullptr;
    Message req(1, MessageType::HANDSHAKE);
    bool ok = client.sendRequestGetResponse(&req, &resp);
    ASSERT_TRUE(!ok);

    Message *incoming = server.receive();
    ASSERT_TRUE(incoming == nullptr);

    cancel_watchdog();
    std::cout << "[Test 5] PASSED" << std::endl;
}

// TEST 6: Performance Benchmark
void run_benchmark_server() {
    ServerProcCommunicator server(shared_mem_name);
    for (int i = 0; i < 100000; ++i) {
        Message *req = server.receive();
        if (req) {
            Message resp(req->id, MessageType::HANDSHAKE_OK);
            server.send(&resp);
        }
    }
    std::exit(0);
}

void run_benchmark_client() {
    ClientProcCommunicator client(shared_mem_name);
    Message req(1, MessageType::HANDSHAKE);
    const Message *resp = nullptr;
    
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 100000; ++i) {
        bool ok = client.sendRequestGetResponse(&req, &resp);
        ASSERT_TRUE(ok);
    }
    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "[Benchmark] 100,000 round-trips completed." << std::endl;
    std::cout << "[Benchmark] Total Time: " << elapsed_ms << " ms" << std::endl;
    std::cout << "[Benchmark] Latency: " << (elapsed_ms * 1000.0 / 100000.0) << " microseconds / round-trip" << std::endl;
    std::cout << "[Benchmark] Throughput: " << (100000.0 / (elapsed_ms / 1000.0)) << " round-trips / sec" << std::endl;
    std::exit(0);
}

void test_performance_benchmark() {
    std::cout << "[Benchmark] Starting Performance Benchmark (100,000 round-trips)..." << std::endl;
    pid_t server_pid = fork();
    if (server_pid == 0) {
        run_benchmark_server();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    setup_watchdog(30); // 30 seconds watchdog
    
    pid_t client_pid = fork();
    if (client_pid == 0) {
        run_benchmark_client();
    }
    
    int client_status;
    waitpid(client_pid, &client_status, 0);
    cancel_watchdog();
    
    kill(server_pid, SIGKILL);
    int status;
    waitpid(server_pid, &status, 0);
    
    ASSERT_TRUE(WIFEXITED(client_status) && WEXITSTATUS(client_status) == 0);
    std::cout << "[Benchmark] Completed successfully." << std::endl;
}

int main() {
    // Clean up any stale shared memory and semaphores from previous runs
    shm_unlink("/shm_test_suite_master");
    shm_unlink("/shm_test_suite_slave");
    sem_unlink("/shm_test_suite_m_rsem");
    sem_unlink("/shm_test_suite_s_rsem");
    sem_unlink("/shm_test_suite_m_sent");
    sem_unlink("/shm_test_suite_s_sent");
    sem_unlink("/shm_test_suite_s_ready");

    // Watchdog signal handler to catch deadlock hangs
    signal(SIGALRM, [](int sig) {
        std::cerr << "\n!!! WATCHDOG TIMEOUT: Test deadlocked or took too long !!!" << std::endl;
        std::exit(1);
    });

    test1_single_roundtrip();
    std::cout << "------------------------------------------" << std::endl;
    
    test2_multi_client();
    std::cout << "------------------------------------------" << std::endl;
    
    test3_timeout();
    std::cout << "------------------------------------------" << std::endl;
    
    test4_large_id();
    std::cout << "------------------------------------------" << std::endl;

    test5_error_handling();
    std::cout << "------------------------------------------" << std::endl;

    test_performance_benchmark();
    std::cout << "------------------------------------------" << std::endl;
    
    std::cout << "ALL TESTS COMPLETED" << std::endl;
    return 0;
}
