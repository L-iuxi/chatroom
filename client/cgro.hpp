#ifndef CGRO_HPP
#define CGRO_HPP
#include <iostream>
#include <../other/ui.hpp>
#include <cstring>
#include <thread>
#include <functional>
#include "client.hpp"

using namespace std;
using json = nlohmann::json;
class TCP;
class MSG;
class LOGIN;
class FRI;
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
    void  open_group_owner(TCP &client,LOGIN &login,int &m,string group_id);
    void  open_group_admin(TCP &client,LOGIN &login,int &m,string group_id);
    void  open_group_member(TCP &client,LOGIN &login,int &m,string group_id);
    void manage_member(TCP &client,LOGIN &login,string group_id);
    void send_add_group(TCP &client, LOGIN &login) ;
    void see_add_group(TCP &client,LOGIN &login,string group_id);
    void open_group_block(TCP &client,LOGIN &login,string group_id);
    void receive_group_message(TCP& client,string from_id,string group_id);
    void send_message_group(TCP &client,string from_id,string group_id);
    void send_file_group(TCP &client,LOGIN &login,string group_id);
    void accept_file_group(TCP &client, LOGIN &login,string group_id);
};
#endif