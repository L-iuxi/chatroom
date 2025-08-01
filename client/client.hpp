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
using AckCallback = std::function<void(const string&)>;
std::atomic<bool> chat_active{true};//控制好友聊天
std::atomic<bool> group_active{true};  //控制群聊天
std::atomic<bool> heartbeat_received(true);
std::condition_variable cv;
bool is_paused = false;
std::mutex mtx;   
struct FriendRequest {
    string from_id;
    string message;
    string status;
};
class TCP{
    private:
    int client_socket;
    char buffer[BUFFER_SIZE];
    int heart_socket;
    public:
    int transfer_socket;
    int data_socket;
    const char* SERVER_IP;
     
    TCP(const char* IP);//建立客户端套接字
   
    int getClientSocket() {
    return client_socket;
    }

    void send_m(string type,string from_sb,string to_sb,string message);//发送消息的函数
    bool rec_m(string &type,string &message);
    void new_socket();
    void connect_transfer_socket();
    void heartbeat();

   ~TCP()
   {
    close(client_socket);
    close(data_socket);
   }
   void recv_server(int data_socket);
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
class GRO{
    private:
    string group_id;
    std::atomic<bool> is_transfer{false}; 
    public:
    //创建群聊
    void generate_group(TCP &client,LOGIN &login); 
    void delete_group(TCP &client, LOGIN &login);
    void see_all_group(TCP &client, LOGIN &login);
    void see_all_members(TCP &client,LOGIN &login,string group_id);
    void add_admin(TCP &client,LOGIN &login,string group_id);
    void quit_group(TCP &client,LOGIN &login,string group_id,int &m);
    void manage_group(TCP &client,LOGIN &login,string group_id);
    void delete_admin(TCP &client,LOGIN &login,string group_id);
    void manage_admin(TCP &client,LOGIN &login,string group_id);
    void open_group_owner(TCP &client,LOGIN &login,int &m,string group_id);
    void open_group_admin(TCP &client,LOGIN &login,int &m,string group_id);
    void open_group_member(TCP &client,LOGIN &login,int &m,string group_id);
    void manage_member(TCP &client,LOGIN &login,string group_id);
    void send_add_group(TCP &client, LOGIN &login) ;
    void see_add_group(TCP &client,LOGIN &login,string group_id);
    void open_group_block(TCP &client,LOGIN &login,string group_id);
    void receive_group_message(TCP& client,string from_id,string group_id);
    void send_message_group(TCP &client,string from_id,string group_id);
    void send_file_group(TCP &client,LOGIN &login,string group_id);
    void accept_file_group(TCP &client, LOGIN &login,string group_id);
};
class FRI{
    private:
    std::atomic<bool> is_transfer{false}; 
    public:
    void make_choice(TCP &client,LOGIN &login);
    void add_friend(TCP &client,string from_user_id);
    void check_message(TCP &client,LOGIN &login);
    void check_add_friends_request(TCP &client,LOGIN &login);
    void delete_friend(TCP& client,string from_id);
    void see_all_friends(TCP &client,string from_id);
    //void send_message(TCP &client,LOGIN &login);
    void open_block(TCP &client,LOGIN &login,string to_id);
    void shidld_friend(TCP &client,LOGIN &login);
    void cancel_shidld_friend(TCP &client,LOGIN &login);
    void receive_log(TCP& client,string from_id,string to_id);
    void send_message_no(TCP &client,string from_id,string to_id);
    void send_file_to_friends(TCP &client, LOGIN &login,string to_id);
    void manage_friends(TCP &client,LOGIN &login);
    void accept_file(TCP &client, LOGIN &login,string to_id);
    void manage_group(GRO &group,TCP &client,LOGIN &login);
  void open_group(GRO &group,TCP &client,LOGIN &login);
  void choose_command(TCP &client, LOGIN &login);

    //int client_socket,string from_user_id
    // void make_choice(TCP &client,LOGIN &login);
};
