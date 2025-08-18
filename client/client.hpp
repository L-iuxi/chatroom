#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <nlohmann/json.hpp>
#include <../other/ui.hpp>
#include <thread>
#include <atomic>
#include <functional> 
#include <fstream> 
#include <sys/stat.h> 
#include <sys/types.h>
#include <mutex>
#include <condition_variable>

#include <functional>
#include "cfri.hpp"
#include "cgro.hpp"



//#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024
#define GREEN "\033[32m"
#define RESET "\033[0m"
#define RED   "\033[31m"
#define PRINT_GREEN(msg) cout<<GREEN<<msg<<RESET
#define PRINT_RED(msg) cout<<RED<<msg<<RESET
using namespace std;
using json = nlohmann::json;

extern std::atomic<bool> chat_active;
extern std::atomic<bool> group_active;
extern std::atomic<bool> heartbeat_received;
extern std::condition_variable cv;
extern bool is_paused;  // 或者改为 std::atomic<bool>
extern std::mutex mtx; 
struct FriendRequest {
    string from_id;
    string message;
    string status;
};
class TCP{
    private:
    char buffer[BUFFER_SIZE];
    
    public:
     int heart_socket;
    int notice_socket;
     int client_socket;
    int transfer_socket;
    int data_socket;
    std::atomic<bool> notice_thread_running_{false};
    std::thread notice_thread_;
    const char* SERVER_IP;
     
    TCP(const char* IP);//建立客户端套接字
   
    int getClientSocket() {
    return client_socket;
    }

    void send_m(const string& type,const string& from_sb,const string& to_sb, const string& message);//发送消息的函数
    bool rec_m(string &type,string &message);
    int readn(int data_socket,int size,char *buf);
    void new_socket();
    void connect_transfer_socket();
    void heartbeat();
    void connect_notice_socket();
    void notice_receiver_thread();
    bool new_heartbeat_socket() ;
   ~TCP()
   {
    close(client_socket);
    close(data_socket);
    close(notice_socket);
   }
   void recv_server_(int data_socket);
   void pause_heartbeat()
{
    std::lock_guard<std::mutex> lock(mtx);  // 确保线程安全
    is_paused = true;  // 设置线程暂停标志
}
    void resume_heartbeat()
{
    std::lock_guard<std::mutex> lock(mtx);  // 确保线程安全
    is_paused = false;  // 设置线程恢复标志
    cv.notify_one();  // 通知心跳线程继续
}
   void  notice_message();
};
class LOGIN{
    private:
    
    string user_id;
    string passward;
    string username;
     //TCP &client_socket; 
    public:
    LOGIN() = default;
    
    void register_user(TCP& client);//注册
    bool login_user(TCP& client);//登陆
    void deregister_user(TCP& client);//注销
    string getuser_id()
    {
        return user_id;
    }
    string getpassward()
    {
        return passward; 
    }
    string getusername()
    {
        return username;
    }
};
string hexdump(const char* data, size_t len);
#endif