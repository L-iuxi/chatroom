#ifndef CFRI_HPP
#define CFRI_HPP
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
class GRO;
class LOGIN;
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
#endif