#ifndef FRI_HPP
#define FRI_HPP
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
class FRI{
     private:  
    TCP* server;
    
    public:
    FRI(TCP* server_instance) {
        server = server_instance;
    }
     std::mutex pairs_mutex;
    //添加好友
   // void FRI::send_add_resquest(int data_socket,string to_id,string from_id,string message,DATA &redis_data)
   void send_add_request(TCP &client,int data_socket,string to_id,string from_id,string message,DATA &redis_data,MSG &msg);
   void check_add_friends_request(TCP &client,int data_socket,string from_id,DATA &redis_data,int &m,MSG &msg);//查看好友申请
   void add_friend(TCP &client,int data_socket,string to_id,string from_id,DATA &redis_data,MSG &msg);//添加好友
   void delete_friend(TCP &client,int data_socket,string to_id,string from_id,DATA& redis_data,MSG &msg);//删除好友
   void see_all_friends(TCP &client,int data_socket,string from_id,DATA &redis_data,MSG &msg);//查询所有好友
   void refuse_friend_request(TCP &client,int data_socket,string to_id,string from_id,DATA &redis_data,MSG &msg);//拒绝好友申请
   void send_message(TCP &client,int data_socket,string from_id,string to_id,string message,DATA &redis_data,MSG &msg);//给好友发送消息
   void open_block(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data,MSG &msg);
    void quit_chat(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data,MSG &msg);
    void shield_friend(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data,MSG &msg);//屏蔽好友
     void cancel_shield_friend(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data,MSG &msg);//取消屏蔽
     void new_message(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data,MSG &msg);
     void send_file(TCP &client,int data_socket,string from_id, string to_id, string message, DATA &redis_data,MSG &msg);
    
    void accept_file(TCP &client,int data_socket,string from_id, string to_id, string message, DATA &redis_data,MSG &msg);
    void is_friends(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg);
    void store_chat_Pair(string first, string second) ;
    bool check_chat(string a,string b);
    void printChatPairsTable();
    bool delete_chat_pair(string first);
   

    
};
#endif