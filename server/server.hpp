
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <hiredis/hiredis.h>
#include <fcntl.h> 
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <sys/epoll.h>
#include <random>
#include <fstream> 
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <sys/stat.h>  // 添加这个头文件
#include <unistd.h>
#include <ctime>
#include <tuple>
#include <algorithm>
//#include"friend.hpp"
#include </home/liuyuxi/chatroom/other/threadpool.hpp>
//#include "friend.hpp"

#define PORT 8080
#define MAX_CONNECTIONS 5
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024
#define PASV_PORT_MIN  1024
#define PASV_PORT_MAX 65535

using namespace std;
using json = nlohmann::json;
  struct FriendRequest {
    string from_id;
    string message;
    string status;
};
class DATA{
    private:
      bool checkTransactionResult(redisReply* reply);
    public:
    redisContext* c;
    redisContext* data_create();
    bool check_username_duplicate(string username);//检查用户名是否重复
    bool add_user(string user_id_str,string username,string password);//在数据库中添加用户数据
    string get_username_by_id(string user_id);//通过用户id寻找对应的用户名，也可以用于检查用户是否存在
    bool check_password(string password,string user_id);//检查密码与数据库中密码是否相同
    bool delete_user(string user_id);//从数据库中删除某个用户数据
    bool add_friends_request(string from_id,string to_id,string message,string status);//把发过来的好友申请存到好友申请表里面
    string check_add_request(string to_id);//在好友申请列表里面寻找是否有好友请求
    void add_friends(string to_id,string from_id);//添加好友
    bool revise_status(string from_id,string to_id,string new_status);//修改好友列表里面的好友状态
    void rdelete_friend(string to_id,string from_id);//删除好友
    void see_all_friends(string from_id,vector<string>&buffer);//查询全部好友
    bool is_friend(string to_id, string from_id);//检查两人是否为好友
    bool check_dealed_request(string from_id,string to_id,string type);//检查好友申请是否已经被处理过
    bool check_add_friend_request_duplicata(string to_id,string from_id);//检查to_id是否已经向from_id发送过申请
   bool remove_friends_request(string from_id, string to_id,string message,string status);
    //bool check_recived_recover(string from_id, string type);
    bool check_recived_recover(string from_id, string type, string& data);
    //把发送的消息存储到消息表中
    bool add_message_log(string from_id,string to_id,string m);
    //从消息列表中获取fromid和message
    bool get_messages( const string& toid,vector<string>& fromids,vector<string>& messages);
    bool get_messages_2( const string& toid,vector<string>& fromids,vector<string>& messages); 
    bool see_all_other_message(string from_id,string to_id,vector<string>& messages);
    bool see_all_my_message(string from_id,string to_id,vector<string>& messages);
    bool get_id_messages(const string& toid,const string& fromid, vector<string>& messages);
    bool get_files(string from_id, string to_id,vector<string>& result);
    bool revise_file_status(string from_id,string to_id,string filename);
    bool get_unread_files(string to_id,vector<pair<string, string>>& result);
    bool add_file(string from_id,string to_id, string filename);
     bool generate_group(string from_id,string message,string group_id);//初始化群聊
     bool add_group_member(string group_id,string to_id);//添加群成员
     bool is_group_owner(string group_id, string user_id);//检查是不是群主
     bool delete_group(string group_id);//注销群聊
     vector<string> get_groups_by_user(string user_id);
     string get_group_name(string group_id);
     vector<string> get_all_members(string group_id);//查询群聊内全部成员
     int get_admin_count(string group_id);//获取管理员数量
     bool add_admins(string user_id, string group_id);//添加用户为管理员
     //检查是不是管理员
    bool is_admin(string user_id, string group_id);
    bool is_in_group(string group_id,string user_id);//检查用户在不在群中
    bool is_group_exists(string group_id);//检查群聊是否存在
    bool remove_group_admin(string group_id,string member_id);//移除某人管理员身份
    bool remove_group_member(string group_id, string member_id);//从群成员列表里面移除某人
    vector<pair<string, string>> get_unread_applications(string group_id);
    bool apply_to_group(string applicant_id, string group_id,string message);
    
    bool check_group_status(string applicant_id, string group_id, string status); //检查群聊申请状态是否为status
    bool revise_group_status(string applicant_id, string group_id, string status);//修改群聊申请状态
    bool set_last_read_time(string user_id,string group_id,string timestamp);//设置最后已读时间
    vector<pair<string, string>> get_unread_messages(string user_id, string group_id) ;//获取未读信息
    bool add_group_message(string from_id, string message, string group_id, string timestamp);//添加消息
    vector<tuple<string, string, string>> get_read_messages(string user_id, string group_id) ;//获取已读历史消息
    bool add_file_to_unread_list(string group_id,string from_id,string filename);//把文件添加到群所有成员未读列表
    bool remove_unread_file(string group_id,string user_id,string file_value);//移除群里某成员的未读文件列表
    vector<string> get_unread_file_group(string group_id,string user_id);//获取群里某成员的未读文件
    bool add_file_group(string group_id,string from_id,string filename);//把群文件添加到redis
    bool clear_group_files( string group_id);//删除群内的所有文件
    bool clear_group_request(string group_id);//删除群内的所有请求
    bool clear_group_messages(string group_id);//删除群聊的所有消息
    bool delete_file_records(string user_id);//删除某人的所有文件（接收or发送
    bool delete_message_logs(string user_id);//删除某人的所有消息
    bool delete_friends(string user_id) ;//删除某人的所有好友
    //bool DATA::delete_user(user_id)
};
class TCP{
    private:
    int server_socket;
    char buffer[1024];
    Threadpool pool;
    public:
    
    TCP();//创建服务器套接字
    ~TCP();
    void start(DATA &redis_data);//与客户端进行连接
    //redisContext* data_create();

    int user_count;  
    unordered_map<int, string> logged_users; 
   unordered_map<string, chrono::system_clock::time_point> client_last_online_time;
   // const chrono::minutes timeout_limit = chrono::minutes(5);
   const chrono::seconds timeout_limit = chrono::seconds(10);
    
    string find_user_id(int socket);
    void recived_message(DATA &redis_data,string user_id,int data_socket);
    bool  rec_m(string &type,string &from_id,string &to_id,string &message,int data_socket);
   void send_m(int data_socket,string type,string message);
    int generate_port();
    int new_socket(int client_socket);
    void make_choice(int data_socket,DATA &redis_data);
    int find_socket(const std::string& user_id);
    void remove_user(int data_socket);
    int new_transfer_socket(int client_socket);
    void checkTimeout();
    void handleTimeout(const string& client_id);
     void updateLastOnlineTime(const string& client_id);
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
class FRI{
     private:  
    TCP* server;
    public:
    FRI(TCP* server_instance) {
        server = server_instance;
    }
    //添加好友
   // void FRI::send_add_resquest(int data_socket,string to_id,string from_id,string message,DATA &redis_data)
   void send_add_request(TCP &client,int data_socket,string to_id,string from_id,string message,DATA &redis_data);
   void check_add_friends_request(TCP &client,int data_socket,string from_id,DATA &redis_data,int &m);//查看好友申请
   void add_friend(TCP &client,int data_socket,string to_id,string from_id,DATA &redis_data);//添加好友
   void delete_friend(TCP &client,int data_socket,string to_id,string from_id,DATA& redis_data);//删除好友
   void see_all_friends(TCP &client,int data_socket,string from_id,DATA &redis_data);//查询所有好友
   void refuse_friend_request(TCP &client,int data_socket,string to_id,string from_id,DATA &redis_data);//拒绝好友申请
   void send_message(TCP &client,int data_socket,string from_id,string to_id,string message,DATA &redis_data);//给好友发送消息
void open_block(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data);
    void shield_friend(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data);//屏蔽好友
     void cancel_shield_friend(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data);//取消屏蔽
     void new_message(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data);
     void send_file(TCP &client,int data_socket,string from_id, string to_id, string message, DATA &redis_data);
    
void accept_file(TCP &client,int data_socket,string from_id, string to_id, string message, DATA &redis_data);

  

    
};
class GRO{
    private:  
    TCP* server;
    public:
    GRO(TCP* server_instance) {
        server = server_instance;
    }
    //生成随机群号
    int generateNumber();
    //创建群
    void generate_group(TCP &client,int data_socket,string from_id,string to_id,string message,DATA &redis_data);
    //解散群
    void delete_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    //查看该用户加入的所有群聊
    void see_all_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    //查看某群聊的全部成员
    void see_all_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    //添加为管理
    void add_admin(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void open_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    //退出群聊
    void quit_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void delete_admin(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void add_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void send_add_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void see_add_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void add_group_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void refuse_group_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void receive_group_message(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void add_group_message(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void open_group_block(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void send_file_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void accept_file_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
    void delete_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data);
};
string delete_line(const std::string& message);
string delete_space(const std::string& message);
string getCurrentTimestamp();
void parse_json_message(const char* data, size_t len, string& type, string& from_id, string& to_id, string& message);