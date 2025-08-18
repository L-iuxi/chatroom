
#ifndef REDIS_HPP
#define REDIS_HPP
#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>  
#include <fcntl.h> 
#include <nlohmann/json.hpp>
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
    static thread_local redisContext* c;
    public:

    //redisContext* c;
    ~DATA() {
        if (c) {
            redisFree(c);
            c = nullptr;
        }
    }
   
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
    string check_add_request_and_revise(string to_id);
   bool remove_friends_request(string from_id, string to_id);
    //bool check_recived_recover(string from_id, string type);
    bool check_recived_recover(string from_id, string type, string& data);
    //把发送的消息存储到消息表中
    bool add_message_log(string from_id,string to_id,string m);
    bool process_and_mark_unread_files(string to_id, vector<pair<string, string>>& result);
    //从消息列表中获取fromid和message
    bool get_messages( const string& toid,vector<string>& fromids,vector<string>& messages);
    bool get_messages_2( const string& toid,vector<string>& fromids,vector<string>& messages); 
    bool add_message_log_unread(string from_id,string to_id,string m);
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
    vector<pair<string, string>> get_group_applications(string group_id);
    bool check_group_status(string applicant_id, string group_id, string status); //检查群聊申请状态是否为status
    bool revise_group_status(string applicant_id, string group_id, string status);//修改群聊申请状态
    bool set_last_read_time(string user_id,string group_id,string timestamp);//设置最后已读时间
    bool set_last_notified_time(const string& user_id, const string& group_id, const string& timestamp);//设置最后通知时间
    vector<pair<string, string>> get_unread_messages(string user_id, string group_id) ;//获取未读信息
    vector<pair<string, string>> get_unnotice_messages(string user_id, string group_id);//获取未通知的消息
    bool add_group_message(string from_id, string message, string group_id, string timestamp);//添加消息
    vector<tuple<string, string, string>> get_read_messages(string user_id, string group_id, int count);//获取已读历史消息
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
    string get_unred_messages(string user_id, const std::string& delimiter); 
    bool delete_notice_messages(string user_id);
    bool add_unread_message(string user_id,string message);
    bool remove_group_application(string applicant_id,string group_id);
    bool check_group_apply(string group_id, string to_id);
    //bool DATA::delete_user(user_id)
};

#endif
