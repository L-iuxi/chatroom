#ifndef SERVER_HPP
#define SERVER_HPP
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h> 
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <sys/epoll.h>
#include <random>
#include <fstream> 
#include "redis.hpp"
#include "fri.hpp"
#include "gro.hpp"
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <set>
#include <sys/stat.h>  // 添加这个头文件
#include <unistd.h>
#include <ctime>
#include <tuple>
#include <algorithm>
#include <shared_mutex>
#include <signal.h>
//#include"friend.hpp"
#include <../other/threadpool.hpp>
//#include "friend.hpp"

#define PORT 8080
#define MAX_CONNECTIONS 5
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024
#define PASV_PORT_MIN  1024
#define PASV_PORT_MAX 65535
#define RED_TEXT(text)      ("\033[1;31m" + std::string(text) + "\033[0m")
#define YELLOW_TEXT(text)  ("\033[1;33m" + std::string(text) + "\033[0m")
#define GREEN_TEXT(text)     ("\033[1;32m" + std::string(text) + "\033[0m")

using namespace std;

using json = nlohmann::json;
extern std::mutex redis_mutex;  // 注意：这里只有声明
extern std::mutex g_chat_pairs_mutex;
extern std::unordered_map<std::string, std::string> chat_pairs;
extern std::unordered_map<std::string, std::string> group_pairs;
extern std::unordered_map<std::string, int> notice_user;


class MSG{

    public:
    bool  rec_m(string &type,string &from_id,string &to_id,string &message,int data_socket);
    void send_m(int data_socket,string type,string message);
    int readn(int data_socket,int size,char *buf);

};
class TCP{
    private:
    int server_socket;
    char buffer[1024];
    Threadpool pool;
     std::mutex redis_mutex_;  // 保护Redis数据访问
    std::mutex notice_mutex_;
      struct SocketPair {
        int data_socket;
        int heart_socket;
        std::chrono::steady_clock::time_point last_heartbeat;
    };
    
    std::unordered_map<int, SocketPair> socket_pairs_; // 以data_socket为key
    std::mutex heartbeat_mutex_;
    std::thread monitor_thread_;
    bool monitoring_ = false;
    public:
    TCP();//创建服务器套接字
    ~TCP();
    void start(DATA &redis_data);//与客户端进行连接
    //redisContext* data_create();

    int user_count;  
    unordered_map<int, string> logged_users; 
    std::unordered_map<int, std::chrono::steady_clock::time_point> client_last_heartbeat_;
    std::atomic<bool> running_{true}; 

    
    string find_user_id(int socket);
    void recived_messages(DATA &redis_data,string user_id,int data_socket,MSG &msg);
    int generate_port();
    int new_socket(int client_socket);
    void make_choice(int data_socket,MSG &msg);
    int find_socket(const std::string& user_id);
    void remove_user(int data_socket);
    int new_transfer_socket(int client_socket);
    void updateHeartbeat(int data_socket);
    void startHeartbeatMonitor();
    void stopHeartbeatMonitor();
    void checkHeartbeats();
    int new_notice_socket(int data_socket);

    //登陆成功创建完noticesocket之后加入表
    bool add_notice_socket(string userid, int sockfd);
    //下线之后从表里面移除通知套接字
    void remove_user_socket(string userid);
    int get_noticesocket_by_userid(string userid);
    string  get_userid_by_noticesocket(int sockfd);
    //发送实时消息
    void send_notice(string from_id,string to_id,string message,DATA &redis_data);
    int new_heartbeat_socket(int data_socket);
    
};

class LOGIN{
   private:
    // int user_count;  
    TCP* server;  
    // unordered_map<int, string> logged_users; 
    public:
    LOGIN(TCP* server_instance) {
        server = server_instance;
    }
    void register_user(int data_socket,DATA &redis_data);
    bool login_user(int data_socket,DATA &redis_data);
    void deregister_user(int data_socket,DATA &redis_data);

};

string delete_line(const std::string& message);
string delete_space(const std::string& message);
string getCurrentTimestamp();
void parse_json_message(const char* data, size_t len, string& type, string& from_id, string& to_id, string& message);
bool is_valid_json(const string& json_str);
#endif