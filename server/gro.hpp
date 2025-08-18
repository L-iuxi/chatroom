#ifndef GRO_HPP
#define GRO_HPP
#include "redis.hpp"
#include "server.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <fcntl.h> 
#include <mutex> 
#include <nlohmann/json.hpp>
class TCP;
class MSG;
class DATA;
class FRI;

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
    void generate_group(TCP &client,int data_socket,string from_id,string to_id,string message,DATA &redis_data,MSG &msg);
    //解散群
    void delete_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    //查看该用户加入的所有群聊
    void see_all_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    //查看某群聊的全部成员
    void see_all_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    //添加为管理
    void add_admin(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void open_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    //退出群聊
    void quit_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void delete_admin(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void add_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void send_add_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void see_add_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void add_group_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void refuse_group_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void receive_group_message(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void add_group_message(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void open_group_block(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void send_file_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void accept_file_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void delete_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    bool check_chat(string a,string b); 
    bool delete_chat_pair(string first); 
    void printChatPairsTable();
    void store_chat_Pair(string first, string second);
   void quit_chat(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
};
#endif