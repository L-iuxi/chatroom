
#include "server.hpp"
//连接redis
redisContext* DATA::data_create() {
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            std::cout << "Connection error: " << c->errstr << std::endl;
        } else {
            std::cout << "Unable to connect to Redis" << std::endl;
        }
        return NULL;
    }
    return c;
 }
//检查用户名是否重复
bool DATA::check_username_duplicate(string username)
 {
    redisReply* check_username_reply = (redisReply*)redisCommand(c, "SISMEMBER usernames_set %s", username.c_str());
    if (check_username_reply == NULL) {
        cout << "检查用户名是否存在时出错" << endl;
        return false;
    }
    if (check_username_reply->integer == 1) {
        return true;
        freeReplyObject(check_username_reply);
        
    }
    freeReplyObject(check_username_reply);
    return false;
}
//在数据库中添加用户数据
bool DATA:: add_user(string user_id_str,string username,string password){
    redisReply* reply2 = (redisReply*)redisCommand(c, "HSET users:%s username %s password %s", user_id_str.c_str(), username.c_str(), password.c_str());
    
    
     if(reply2)
    {
     redisCommand(c, "SADD user:%d:friends", user_id_str); 
     //初始时自己是自己的好友
     add_friends(user_id_str,user_id_str);

     redisCommand(c, "SADD user:%d:groups", user_id_str);
    //把用户信息序列化写进文件里，因为不写的话我记不住QAQ
    nlohmann::json json_data;
    json_data["id"] = user_id_str;
    json_data["name"] = username;
    json_data["password"] = password;
    //std::ofstream file("data.json");
    std::ofstream file("/home/liuyuxi/chatroom/other/data.json",std::ios::app);
    if(file.is_open())
    {
        file << json_data.dump(4)<<endl;
        file.close();
        cout<<"用户数据已保存至data文件"<<endl;
    }else{
        cout<<"error open"<<endl;
    }


    redisCommand(c, "SADD usernames_set %s", username.c_str());
    freeReplyObject(reply2);
    return true;
 }
 freeReplyObject(reply2);
 return false;
 }
//通过用户id寻找对应的用户名，也可以用于检查用户是否存在
string DATA::get_username_by_id(string user_id){
    string username;
 redisReply* reply_findusername= (redisReply*)redisCommand(c, "HGET users:%s username", user_id.c_str());
    if (reply_findusername != NULL && reply_findusername->type == REDIS_REPLY_STRING) {
        
        username = reply_findusername->str;
        freeReplyObject(reply_findusername);
    }else{
        freeReplyObject(reply_findusername);
        return "";
    }
    return username;
 } 
//检查密码与数据库中密码是否相同
bool DATA ::check_password(string password,string user_id){
    redisReply* reply = (redisReply*)redisCommand(c, "HGETALL users:%s", user_id.c_str());
    string stored_password;
    for (int i = 0; i < reply->elements; i += 2) {
        if (string(reply->element[i]->str) == "password") {
            stored_password = reply->element[i + 1]->str;
        }
    }
    // cout<<"数据库中密码为"<<stored_password<<endl;
    // cout<<"接收到密码为"<<password<<endl;

    if (stored_password == password)
    {
        freeReplyObject(reply);
        return true;
    }
    freeReplyObject(reply);
    return false;
 }
//从数据库中删除某个用户数据
bool DATA::delete_user(string user_id){
   redisReply* reply_find_username = (redisReply*)redisCommand(c, "HGET users:%s username", user_id.c_str());

   if (reply_find_username->type == REDIS_REPLY_STRING) {
    string username = reply_find_username->str;
   // cout<<"注销时要删除的用户名为"<<username<<endl;
   
    redisReply *reply_del_userinfo = (redisReply*)redisCommand(c, "DEL users:%s", user_id.c_str());
    if(reply_del_userinfo == NULL)
    {
        cout<<"删除用户相关信息失败"<<endl;
        freeReplyObject(reply_find_username);
        freeReplyObject(reply_del_userinfo);
        return false;
    }
    freeReplyObject(reply_del_userinfo);
    //删除用户名列表内的username
    redisReply *reply_del_username = (redisReply*)redisCommand(c, "SREM usernames_set %s", username.c_str());
    if(reply_del_username == NULL)
    {
        cout<<"删除用户名列表失败"<<endl;
        freeReplyObject(reply_find_username);
        freeReplyObject(reply_del_username);
        return false;
    }
    freeReplyObject(reply_find_username);
    freeReplyObject(reply_del_username);
    return true;
   }
    freeReplyObject(reply_find_username);
    return false;
 }
//把发过来的好友申请存到好友申请表里面
bool DATA::add_friends_request(string from_id, string to_id, string message, string status) {
   //用列表储存一个用户收到的所有好友申请
    redisReply *reply = (redisReply*) redisCommand(c, "RPUSH request:%s %s|%s|%s", to_id.c_str(), from_id.c_str(), message.c_str(), status.c_str());

    if (reply && reply->type == REDIS_REPLY_INTEGER && reply->integer > 0) {
        freeReplyObject(reply); 
        return true;
    }
    freeReplyObject(reply);
    return false;
  }

//删除好友请求列表里面的某一项
bool DATA::remove_friends_request(string from_id, string to_id, string message, string status) {
    string match_string = from_id + "|" + message + "|" + status;  
   // cout << "to_id is: " << to_id << endl;
   // cout << "匹配字符串: " << match_string << endl;

    redisReply *reply = (redisReply*) redisCommand(c, "LRANGE %s 0 -1", to_id.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_ARRAY) {
        cout << "读取列表失败" << endl;
        return false;
    }

    bool found = false;
    for (size_t i = 0; i < reply->elements; ++i) {
        string item = reply->element[i]->str;
        if (item == "requests") {
            continue;
        }

        if (item == match_string) {
            found = true;
            break;  
        }
    }

    if (!found) {
        cout << "未找到匹配项" << endl;
        freeReplyObject(reply);
        return false; 
    }

    // 执行 LREM 命令，删除匹配的元素
    redisReply *deleteReply = (redisReply*) redisCommand(c, "LREM %s 0 %s", to_id.c_str(), match_string.c_str());

    if (deleteReply == nullptr) {
        cout << "Redis 删除命令执行失败" << endl;
        freeReplyObject(reply);
        return false;
    }

    if (deleteReply->type == REDIS_REPLY_INTEGER) {
        if (deleteReply->integer > 0) {
            //cout << "删除成功，删除了 " << deleteReply->integer << " 项。" << endl;
            freeReplyObject(reply);
            freeReplyObject(deleteReply);
            return true;  
        } else {
            cout << "删除失败，未找到匹配项。" << endl;
        }
    } else {
        cout << "返回的结果类型不正确，预期为整数类型。" << endl;
    }

    freeReplyObject(reply);
    freeReplyObject(deleteReply);
    return false;  
}
//发送在线好友请求
string DATA::check_add_request_and_revise(string to_id) {
    // 获取指定 to_id 的所有好友请求
    redisReply* reply = (redisReply*)redisCommand(c, "LRANGE request:%s 0 -1", to_id.c_str());

    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        vector<FriendRequest> requests;
        for (size_t i = 0; i < reply->elements; ++i) {
            // 跳过非请求数据（如"requests"）
            if (strcmp(reply->element[i]->str, "requests") == 0) {
                continue;
            }

            string request_str = reply->element[i]->str;
            size_t pos1 = request_str.find('|');
            size_t pos2 = request_str.find('|', pos1 + 1);

            if (pos1 != string::npos && pos2 != string::npos) {
                string from_id = request_str.substr(0, pos1);
                string message = request_str.substr(pos1 + 1, pos2 - pos1 - 1);
                string status = request_str.substr(pos2 + 1);

                // 只处理status为"unread"的请求
                if (status  == "unread") {
                    FriendRequest req;
                    req.from_id = from_id;
                    req.message = message;
                    req.status = status;
                    requests.push_back(req);
                    revise_status(from_id,to_id,"send");
                }
            }
        }

        try {
            json j;
            for (const auto& req : requests) {
                j.push_back({
                    {"from_id", req.from_id},
                    {"message", req.message},
                    {"status", req.status}
                });
            }
            string serialized_data = j.dump();
            freeReplyObject(reply);
            return serialized_data;
        } catch (const std::exception& e) {
            std::cerr << "JSON 序列化失败: " << e.what() << std::endl;
            freeReplyObject(reply);
            return "";
        }
    }
    freeReplyObject(reply);
    return "";
}
string DATA::check_add_request(string to_id) {
    // 获取指定 to_id 的所有好友请求
    redisReply* reply = (redisReply*)redisCommand(c, "LRANGE request:%s 0 -1", to_id.c_str());

    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        vector<FriendRequest> requests;
        for (size_t i = 0; i < reply->elements; ++i) {
            // 跳过非请求数据（如"requests"）
            if (strcmp(reply->element[i]->str, "requests") == 0) {
                continue;
            }

            string request_str = reply->element[i]->str;
            size_t pos1 = request_str.find('|');
            size_t pos2 = request_str.find('|', pos1 + 1);

            if (pos1 != string::npos && pos2 != string::npos) {
                string from_id = request_str.substr(0, pos1);
                string message = request_str.substr(pos1 + 1, pos2 - pos1 - 1);
                string status = request_str.substr(pos2 + 1);

                // 只处理status为"unread"的请求
                if (status  != "accept" && status != "refuse") {
                    FriendRequest req;
                    req.from_id = from_id;
                    req.message = message;
                    req.status = status;
                    requests.push_back(req);
                }
            }
        }

        try {
            json j;
            for (const auto& req : requests) {
                j.push_back({
                    {"from_id", req.from_id},
                    {"message", req.message},
                    {"status", req.status}
                });
            }
            string serialized_data = j.dump();
            freeReplyObject(reply);
            return serialized_data;
        } catch (const std::exception& e) {
            std::cerr << "JSON 序列化失败: " << e.what() << std::endl;
            freeReplyObject(reply);
            return "";
        }
    }
    freeReplyObject(reply);
    return "";
}
//把好友添加到好友列表
void DATA::add_friends(string to_id,string from_id){
    redisCommand(c, "SADD user:%s:friends %s", from_id.c_str(), to_id.c_str());
}
//删除某人的全部好友列表
bool DATA::delete_friends(string user_id) {
    redisReply* reply = (redisReply*)redisCommand(
        c, 
        "DEL user:%s:friends", 
        user_id.c_str()
    );

    bool success = (reply != nullptr && reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    return success;
}
//修改好友申请状态
bool DATA::revise_status(string from_id, string to_id, string new_status) {
    // 获取列表中的所有好友请求
    redisReply *reply = (redisReply*) redisCommand(c, "LRANGE request:%s 0 -1", to_id.c_str());

    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        bool found = false;

        // 遍历列表中的每个元素
        for (size_t i = 0; i < reply->elements; ++i) {
            // 每个元素的内容形式是 "from_id|message|status"
            std::string request = reply->element[i]->str;
            size_t pos1 = request.find('|');
            size_t pos2 = request.find('|', pos1 + 1);

            if (pos1 != std::string::npos && pos2 != std::string::npos) {
                std::string stored_from_id = request.substr(0, pos1);
                std::string stored_message = request.substr(pos1 + 1, pos2 - pos1 - 1);
                std::string stored_status = request.substr(pos2 + 1);

                
                if (stored_from_id == from_id) {
                    
                    std::string updated_request = stored_from_id + "|" + stored_message + "|" + new_status;

                    
                    redisCommand(c, "LSET request:%s %d %s", to_id.c_str(), i, updated_request.c_str());
                    found = true;
                    break;
                }
            }
        }

        freeReplyObject(reply);


        if (found) {
            return true;
        }
    }

    return false;
}
//检查两人是否为好友
bool DATA::is_friend(string to_id, string from_id) {

    redisReply* reply = (redisReply*)redisCommand(c, "SISMEMBER user:%s:friends %s", to_id.c_str(), from_id.c_str());

    if (reply == nullptr) {
       
        return false;
    }
    bool isFriend = (reply->integer == 1);
    freeReplyObject(reply);

    return isFriend;
}


void DATA::rdelete_friend(string to_id,string from_id){    
    redisCommand(c, "SREM user:%s:friends %s", from_id.c_str(), to_id.c_str());
   // redisCommand(c, "SREM user:%s:friends %s", to_id.c_str(), from_id.c_str());
    }
void DATA::see_all_friends(string from_id,vector<string>&buffer){
    redisReply* reply = (redisReply*)redisCommand(c, "SMEMBERS user:%s:friends", from_id.c_str());

    if (reply == nullptr || reply->type != REDIS_REPLY_ARRAY) {
        std::cerr << "Error: Unable to retrieve friends list or invalid response." << std::endl;
        return;
    }
    buffer.clear();

    for (size_t i = 0; i < reply->elements; ++i) {
        buffer.push_back(reply->element[i]->str);  

    }
    freeReplyObject(reply);
}
//检查to_id是否已经向from_id发送过申请，是否在好友申请表里
bool DATA::check_add_friend_request_duplicata(string to_id, string from_id){
    redisReply *reply = (redisReply*) redisCommand(c, "LRANGE request:%s 0 -1", to_id.c_str());

    if (reply == nullptr) {
        cerr << "Error: No reply from Redis." << std::endl;
        return false;
    }
    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return false; 
    }
    bool found = false;

    for (size_t i = 0; i < reply->elements; ++i) {
        string request = reply->element[i]->str;
        size_t pos = request.find('|');
        if (pos != string::npos) {
            string request_from_id = request.substr(0, pos);

            if (request_from_id == from_id) {
                found = true;
                break;  
            }
        }
    }

    freeReplyObject(reply);

    return found;  
}
//检查好友申请是否已经被处理
bool DATA::check_dealed_request(string from_id, string to_id, string type) {
    string key = "request:" + to_id;

    // 执行 LRANGE 命令，获取指定范围的列表元素（此处获取所有元素）
    redisReply *reply = (redisReply*) redisCommand(c, "LRANGE %s 0 -1", key.c_str());

    if (reply == nullptr) {
        cerr << "Error: No reply from Redis." << endl;
        return false;
    }

    if (reply->type == REDIS_REPLY_NIL) {
        // 如果返回的是空值，说明没有找到请求
        freeReplyObject(reply);
        return false;
    }

    if (reply->type == REDIS_REPLY_ARRAY) {
        bool found = false;
        string stored_from_id;
        string stored_status;

        // 遍历所有好友请求
        for (size_t i = 0; i < reply->elements; ++i) {
            string request = reply->element[i]->str;

            // 解析 request 字符串，按 | 分隔
            size_t pos1 = request.find('|');
            size_t pos2 = request.find('|', pos1 + 1);

            if (pos1 != string::npos && pos2 != string::npos) {
                // 提取 from_id, message, status
                string stored_from_id = request.substr(0, pos1);
                string stored_message = request.substr(pos1 + 1, pos2 - pos1 - 1);
                string stored_status = request.substr(pos2 + 1);

                // 判断 from_id 是否匹配
                if (stored_from_id == from_id) {
                    found = true;

                    // 检查请求状态是否匹配
                    if (stored_status == type) {
                        cout << "Friend request already sent from " << from_id << " to " << to_id << " with matching type." << std::endl;
                        freeReplyObject(reply);
                        return true; // 请求已存在并且类型匹配
                    }
                }
            }
        }

        if (!found) {
            cout << "Friend request not found." << std::endl;
        } else {
            cout << "Friend request found, but type mismatch." << std::endl;
        }

        freeReplyObject(reply);
        return false; // 未找到或类型不匹配
    }

    freeReplyObject(reply);
    return false;
}
//寻找该用户有没有被通过的好友请求或拒绝的好友请求
bool DATA::check_recived_recover(string from_id, string type, string& data) {
    redisReply *reply = (redisReply*) redisCommand(c, "KEYS request:*"); // 获取所有的 request:<to_id> 列表

    if (reply == nullptr || reply->type != REDIS_REPLY_ARRAY) {
        cerr << "Error: No reply or invalid reply type from Redis." << endl;
        return false;
    }
    bool found = false;
    data.clear();  

    for (size_t i = 0; i < reply->elements; ++i) {
        string to_id = reply->element[i]->str;
        redisReply *requestReply = (redisReply*) redisCommand(c, "LRANGE %s 0 -1", to_id.c_str());

        if (requestReply == nullptr || requestReply->type != REDIS_REPLY_ARRAY) {
            continue;  
        }

        for (size_t j = 0; j < requestReply->elements; ++j) {
            string request = requestReply->element[j]->str;
            size_t pos1 = request.find('|');
            size_t pos2 = request.find('|', pos1 + 1);

            if (pos1 != string::npos && pos2 != string::npos) {
                string stored_from_id = request.substr(0, pos1);
                string stored_message = request.substr(pos1 + 1, pos2 - pos1 - 1);
                string stored_status = request.substr(pos2 + 1);

               
                if (stored_from_id == from_id) {
        
                    if (stored_status == type) {
                        found = true;
                        if (!data.empty()) {
                            data += ", "; 
                        }
                        data += to_id; 
                        remove_friends_request(from_id,to_id,stored_message, stored_status);
                        //cout << "Found a matching friend request from " << from_id << " with status " << type << " for to_id: " << to_id << endl;
                    }
                }
            }
        }

        freeReplyObject(requestReply);  
    }

    freeReplyObject(reply);  
    

    return found;  
}
//把消息添加到消息列表
bool DATA::add_message_log(string from_id,string to_id,string m){
 
    string message = delete_space(m);
    string timestamp = getCurrentTimestamp();
    message += "|unread|" + timestamp;
   char a = '"';
    string listCommand = "RPUSH messages:" + from_id + ":" + to_id +" " + message;
    cout<<"listcommand is"<<listCommand.c_str()<<endl;
  
    redisReply* reply = (redisReply*)redisCommand(c, listCommand.c_str());
  
   //redisReply* reply = (redisReply*)redisCommand(c, listCommand.c_str());

    if (reply && reply->type == REDIS_REPLY_INTEGER && reply->integer > 0) {
    cout << "Pushed message successfully. List length: " << reply->integer << endl;
    freeReplyObject(reply);
    return true;
    } else {
    cout << "Failed to push message" << endl;
    if (reply) 
    freeReplyObject(reply);
    return false;
    }


    return false;
}
//删除含某人的所有消息记录
bool DATA::delete_message_logs(string user_id) {
    // 需要删除该用户作为发送方和接收方的所有消息
    redisReply* reply1 = (redisReply*)redisCommand(
        c, 
        "KEYS messages:%s:*", 
        user_id.c_str()
    );

    redisReply* reply2 = (redisReply*)redisCommand(
        c, 
        "KEYS messages:*:%s", 
        user_id.c_str()
    );

    bool success = true;
    
    // 删除作为发送方的消息
    if (reply1 && reply1->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply1->elements; i++) {
            redisCommand(c, "DEL %s", reply1->element[i]->str);
        }
    } else {
        success = false;
    }

    // 删除作为接收方的消息
    if (reply2 && reply2->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply2->elements; i++) {
            redisCommand(c, "DEL %s", reply2->element[i]->str);
        }
    } else {
        success = false;
    }

    if (reply1) freeReplyObject(reply1);
    if (reply2) freeReplyObject(reply2);
    return success;
}
//把所有的来自fromid的给toid的未读消息找出来,不改为已读
bool DATA::see_all_other_message(string from_id,string to_id,vector<string>& messages)
{
    redisReply* reply = (redisReply*)redisCommand(c, "KEYS messages:%s:%s",to_id.c_str(),from_id.c_str());
   
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }
    for (size_t i = 0; i < reply->elements; i++) {
        std::string key = reply->element[i]->str;
        
        //  获取该键下的所有消息
        redisReply* msg_reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", key.c_str());
        if (!msg_reply || msg_reply->type == REDIS_REPLY_ERROR) {
            freeReplyObject(msg_reply);
            continue;
        }

        //  检查每条消息的状态
        for (size_t j = 0; j < msg_reply->elements; j++) {
            string message = msg_reply->element[j]->str;
            size_t pipe_pos1 = message.find('|');  // Find first pipe for separating content and status
            size_t pipe_pos2 = message.find('|', pipe_pos1 + 1); 
            if (pipe_pos1 != std::string::npos && pipe_pos2 != std::string::npos) {
                string content = message.substr(0, pipe_pos1);
                string status = message.substr(pipe_pos1 + 1, pipe_pos2 - pipe_pos1 - 1);
                 string timestamp = message.substr(pipe_pos2 + 1);
                
                // if (status == "unread") {
                    // 如果是未读消息，保存fromid和消息内容
                   string m=delete_space(content);
                    string formatted_message =  m+ "|" +timestamp;
                    messages.push_back(formatted_message);
                    
                    // string new_message = content + "|read";
                    // redisCommand(c, "LSET %s %d %s", key.c_str(), j, new_message.c_str());
                // }
            }
        }

        freeReplyObject(msg_reply);
    }

    freeReplyObject(reply);
    return true;
}
//找到from_id给toid的所有消息，并且改为已读
bool DATA::see_all_my_message(string from_id,string to_id,vector<string>& messages)
{
    redisReply* reply = (redisReply*)redisCommand(c, "KEYS messages:%s:%s",to_id.c_str(),from_id.c_str());
   
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }
    for (size_t i = 0; i < reply->elements; i++) {
        std::string key = reply->element[i]->str;
        
        //  获取该键下的所有消息
        redisReply* msg_reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", key.c_str());
        if (!msg_reply || msg_reply->type == REDIS_REPLY_ERROR) {
            freeReplyObject(msg_reply);
            continue;
        }

        //  检查每条消息的状态
        for (size_t j = 0; j < msg_reply->elements; j++) {
            string message = msg_reply->element[j]->str;
           size_t pipe_pos1 = message.find('|');  // Find first pipe for separating content and status
            size_t pipe_pos2 = message.find('|', pipe_pos1 + 1); 
            if (pipe_pos1 != std::string::npos && pipe_pos2 != std::string::npos) {
                string content = message.substr(0, pipe_pos1);
               string status = message.substr(pipe_pos1 + 1, pipe_pos2 - pipe_pos1 - 1);
                 string timestamp = message.substr(pipe_pos2 + 1);
            if (status == "unread") {
             string new_message = content + "|read|"+timestamp;
                    redisCommand(c, "LSET %s %d %s", key.c_str(), j, new_message.c_str());
                }
                   string m=delete_space(content);
                    string formatted_message =  m+ "|" +timestamp;
                    messages.push_back(formatted_message);
                
            }
        }

        freeReplyObject(msg_reply);
    }

    freeReplyObject(reply);
    return true;
}
//获取未读的message和其fromid
bool DATA::get_messages( const string& toid,
                        vector<string>& fromids,
                        vector<string>& messages) {
      // 获取所有相关的消息键
    redisReply* reply = (redisReply*)redisCommand(c, "KEYS messages:*:%s", toid.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }

    // 遍历所有的消息键
    for (size_t i = 0; i < reply->elements; i++) {
        std::string key = reply->element[i]->str;
        
        // 提取fromid (格式为 messages:fromid:toid)
        size_t first_colon = key.find(':');
        size_t second_colon = key.find(':', first_colon + 1);
        std::string fromid = key.substr(first_colon + 1, second_colon - first_colon - 1);

        // 获取该键下的所有消息
        redisReply* msg_reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", key.c_str());
        if (!msg_reply || msg_reply->type == REDIS_REPLY_ERROR) {
            freeReplyObject(msg_reply);
            continue;
        }

        // 检查每条消息的状态
        for (size_t j = 0; j < msg_reply->elements; j++) {
            std::string message = msg_reply->element[j]->str;
            
            // 分割消息内容、状态和时间戳
            size_t first_pipe = message.find('|');
            size_t second_pipe = message.find('|', first_pipe + 1);
            if (first_pipe != std::string::npos && second_pipe != std::string::npos) {
                std::string content = message.substr(0, first_pipe);  // 消息内容
                std::string status = message.substr(first_pipe + 1, second_pipe - first_pipe - 1);  // 消息状态
                std::string timestamp = message.substr(second_pipe + 1);  // 消息时间戳

                // 如果状态是未读的消息
                if (status != "read") {
                    // 保存fromid和消息内容
                    fromids.push_back(fromid);
                    
                    // 去除消息中的空格并保存
                    string cleaned_message = delete_space(content);
                    messages.push_back(cleaned_message);
                }
            }
        }

        freeReplyObject(msg_reply);
    }

    freeReplyObject(reply);
    return true;
}
//改为send
bool DATA::get_messages_2( const string& toid,
                        vector<string>& fromids,
                        vector<string>& messages) {
      // 获取所有相关的消息键
    redisReply* reply = (redisReply*)redisCommand(c, "KEYS messages:*:%s", toid.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }

    // 遍历所有的消息键
    for (size_t i = 0; i < reply->elements; i++) {
        std::string key = reply->element[i]->str;
        if (!reply->element[i] || 
        reply->element[i]->type != REDIS_REPLY_STRING) {
        continue;  // 跳过无效元素
    }
        // 提取fromid (格式为 messages:fromid:toid)
        size_t first_colon = key.find(':');
        size_t second_colon = key.find(':', first_colon + 1);
        std::string fromid = key.substr(first_colon + 1, second_colon - first_colon - 1);

        // 获取该键下的所有消息
        redisReply* msg_reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", key.c_str());
        if (!msg_reply || msg_reply->type == REDIS_REPLY_ERROR) {
            freeReplyObject(msg_reply);
            continue;
        }

        // 检查每条消息的状态
        for (size_t j = 0; j < msg_reply->elements; j++) {
            std::string message = msg_reply->element[j]->str;
            
            // 分割消息内容、状态和时间戳
            size_t first_pipe = message.find('|');
            size_t second_pipe = message.find('|', first_pipe + 1);
            if (first_pipe != std::string::npos && second_pipe != std::string::npos) {
                std::string content = message.substr(0, first_pipe);  // 消息内容
                std::string status = message.substr(first_pipe + 1, second_pipe - first_pipe - 1);  // 消息状态
                std::string timestamp = message.substr(second_pipe + 1);  // 消息时间戳

                // 如果状态是未读的消息
                if (status == "unread") {
                    // 保存fromid和消息内容
                    fromids.push_back(fromid);
                    string new_message = content + "|send|"+timestamp;
                    redisCommand(c, "LSET %s %d %s", key.c_str(), j, new_message.c_str());
                
                    // 去除消息中的空格并保存
                    string cleaned_message = delete_space(content);
                    messages.push_back(cleaned_message);
                }
            }
        }

        freeReplyObject(msg_reply);
    }

    freeReplyObject(reply);
    return true;
}
bool DATA::get_id_messages(const string& toid,const string& fromid, vector<string>& messages) {
    // 构建具体的消息键: messages:fromid:toid
    std::string key = "messages:" + fromid + ":" + toid;

    // 获取该键下的所有消息
    redisReply* msg_reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", key.c_str());
    if (!msg_reply || msg_reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(msg_reply);
        return false;
    }

    // 检查每条消息的状态
    for (size_t j = 0; j < msg_reply->elements; j++) {
        std::string message = msg_reply->element[j]->str;

        // 分割消息内容、状态和时间戳
        size_t first_pipe = message.find('|');
        size_t second_pipe = message.find('|', first_pipe + 1);
        if (first_pipe != std::string::npos && second_pipe != std::string::npos) {
            std::string content = message.substr(0, first_pipe);  // 消息内容
            std::string status = message.substr(first_pipe + 1, second_pipe - first_pipe - 1);  // 消息状态
            std::string timestamp = message.substr(second_pipe + 1);  // 消息时间戳

            // 如果状态是未读的消息
            if (status == "unread") {
                // 去除消息中的空格并保存
                string new_message = content + "|read|"+timestamp;
                redisCommand(c, "LSET %s %d %s", key.c_str(), j, new_message.c_str());
                string cleaned_message = delete_space(content);
                messages.push_back(cleaned_message);
            }
        }
    }

    freeReplyObject(msg_reply);
    return true;
}
//添加文件到文件表
bool DATA:: add_file(string from_id,string to_id, string filename) {
        string status = "unread";
        std::string key = "file:" + from_id + ":" + to_id;
        std::string value = filename + "|" + status;
        
        redisReply* reply = (redisReply*)redisCommand(c, "RPUSH %s %s", key.c_str(), value.c_str());
        
        bool success = (reply != nullptr && reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
        freeReplyObject(reply);
        return success;
    
}
//从文件表中删除含某人的所有项
bool DATA::delete_file_records(string user_id) {
    // 需要删除该用户作为发送方和接收方的所有文件记录
    redisReply* reply1 = (redisReply*)redisCommand(
        c, 
        "KEYS file:%s:*", 
        user_id.c_str()
    );

    redisReply* reply2 = (redisReply*)redisCommand(
        c, 
        "KEYS file:*:%s", 
        user_id.c_str()
    );

    bool success = true;
    
    // 删除作为发送方的文件记录
    if (reply1 && reply1->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply1->elements; i++) {
            redisCommand(c, "DEL %s", reply1->element[i]->str);
        }
    } else {
        success = false;
    }

    // 删除作为接收方的文件记录
    if (reply2 && reply2->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply2->elements; i++) {
            redisCommand(c, "DEL %s", reply2->element[i]->str);
        }
    } else {
        success = false;
    }

    if (reply1) freeReplyObject(reply1);
    if (reply2) freeReplyObject(reply2);
    return success;
}
//已知from和to获取文件名
bool DATA::get_files(string from_id, string to_id,vector<string>& result) {
    std::string key = "file:" + from_id + ":" + to_id;  // 修改键格式
    result.clear();
        redisReply* reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", key.c_str());

        if (!reply || reply->type != REDIS_REPLY_ARRAY) {
            if (reply) freeReplyObject(reply);
            return false; // Redis 查询失败
        }

        for (size_t i = 0; i < reply->elements; i++) {
        std::string entry(reply->element[i]->str, reply->element[i]->len);
        size_t separator_pos = entry.find('|');

        if (separator_pos == std::string::npos) {
            continue; // 格式错误，跳过
        }

        std::string filename = entry.substr(0, separator_pos);
        std::string status = entry.substr(separator_pos + 1);

         if (status != "read") 
         {
            cout<<"找到文件"<<endl;
        result.push_back(filename);
       // string new_entry = filename + "|read";
        //redisReply* lset_reply = (redisReply*)redisCommand(c, "LSET %s %d %s", key.c_str(), (int)i, new_entry.c_str());
       // freeReplyObject(lset_reply); 
    } 
    }

    freeReplyObject(reply);
    return true;
    }

//修改文件为已读
bool DATA::revise_file_status(string from_id,string to_id,string filename) {
        std::string key = "file:" + from_id + ":" + to_id; 

        // 1. 获取整个列表
        redisReply* lrange_reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", key.c_str());
        if (!lrange_reply || lrange_reply->type != REDIS_REPLY_ARRAY) {
            freeReplyObject(lrange_reply);
            return false;
        }

        // 2. 遍历查找匹配的记录，并更新状态
        bool found = false;
        for (size_t i = 0; i < lrange_reply->elements; i++) {
            std::string entry = lrange_reply->element[i]->str;
            size_t pos1 = entry.find('|');
            size_t pos2 = entry.rfind('|');

            if (pos1 == std::string::npos || pos2 == std::string::npos || pos1 == pos2) {
                continue; // 格式错误，跳过
            }

            std::string current_from_id = entry.substr(0, pos1);
            std::string current_filename = entry.substr(pos1 + 1, pos2 - pos1 - 1);
            std::string status = entry.substr(pos2 + 1);

            if (current_from_id == from_id && current_filename == filename && (status == "send"||status == "unread")) {
                // 3. 更新状态为 "read"
                std::string new_entry = from_id + "|" + filename + "|read";
                redisReply* lset_reply = (redisReply*)redisCommand(
                    c, "LSET %s %d %s", key.c_str(), (int)i, new_entry.c_str()
                );
                found = (lset_reply != nullptr && lset_reply->type == REDIS_REPLY_STATUS);
                freeReplyObject(lset_reply);
                break;
            }
        }

        freeReplyObject(lrange_reply);
        return found;
    }
//寻找是否有未读文件并返回
bool DATA::get_unread_files(string to_id, vector<pair<string, string>>& result) {
    result.clear();
    
    redisReply* keys_reply = (redisReply*)redisCommand(c, "KEYS file:*:%s", to_id.c_str());
    
    if (!keys_reply || keys_reply->type != REDIS_REPLY_ARRAY) {
        if (keys_reply) freeReplyObject(keys_reply);
        return false;
    }

    for (size_t i = 0; i < keys_reply->elements; i++) {
        string key = keys_reply->element[i]->str;
        
        size_t first_colon = key.find(':');
        size_t second_colon = key.find(':', first_colon + 1);
        string from_id = key.substr(first_colon + 1, second_colon - first_colon - 1);

        redisReply* files_reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", key.c_str());
        
        if (!files_reply || files_reply->type != REDIS_REPLY_ARRAY) {
            if (files_reply) freeReplyObject(files_reply);
            continue;
        }

        for (size_t j = 0; j < files_reply->elements; j++) {
            string entry = files_reply->element[j]->str;
            size_t separator = entry.find('|');
            
            if (separator == string::npos) continue;
            
            string filename = entry.substr(0, separator);
            string status = entry.substr(separator + 1);
            
            if (status == "unread") {
                result.emplace_back(from_id, filename);
            }
        }
        
        freeReplyObject(files_reply);
    }
    
    freeReplyObject(keys_reply);
    return true;
}
//找到未读文件，更新为send
bool DATA::process_and_mark_unread_files(string to_id, vector<pair<string, string>>& result) {
    result.clear();
    bool any_updated = false;
    redisReply* keys_reply = (redisReply*)redisCommand(c, "KEYS file:*:%s", to_id.c_str());
    
    if (!keys_reply || keys_reply->type != REDIS_REPLY_ARRAY) {
        if (keys_reply) freeReplyObject(keys_reply);
        return false;
    }

    for (size_t i = 0; i < keys_reply->elements; i++) {
        string key = keys_reply->element[i]->str;
        
    
        size_t first_colon = key.find(':');
        size_t second_colon = key.find(':', first_colon + 1);
        string from_id = key.substr(first_colon + 1, second_colon - first_colon - 1);

    
        redisReply* files_reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", key.c_str());
        
        if (!files_reply || files_reply->type != REDIS_REPLY_ARRAY) {
            if (files_reply) freeReplyObject(files_reply);
            continue;
        }

        // 4. 检查每个文件状态并更新
        for (size_t j = 0; j < files_reply->elements; j++) {
            string entry = files_reply->element[j]->str;
            size_t separator = entry.find('|');
            
            if (separator == string::npos) continue;
            
            string filename = entry.substr(0, separator);
            string status = entry.substr(separator + 1);
            
            if (status == "unread") {
                // 添加到结果集
                result.emplace_back(from_id, filename);
                
                // 更新状态为 "read"
               // cout<<"已更新"<<endl;
                string new_entry = filename + "|send";
                redisReply* lset_reply = (redisReply*)redisCommand(
                    c, "LSET %s %d %s", key.c_str(), (int)j, new_entry.c_str()
                );
                
                if (lset_reply && lset_reply->type == REDIS_REPLY_STATUS) {
                    any_updated = true;
                }
                
                if (lset_reply) freeReplyObject(lset_reply);
            }
        }
        
        freeReplyObject(files_reply);
    }
    
    freeReplyObject(keys_reply);
    return any_updated || !result.empty();
}
//从所有成员中移除某人
bool DATA::remove_group_member(string group_id, string member_id) {
    // 1. 从所有成员集合中移除
    string all_members_key = "group:" + group_id + ":all_members";
    redisReply* reply = (redisReply*)redisCommand(c, "SREM %s %s", 
                                                all_members_key.c_str(), 
                                                member_id.c_str());
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "从群组全部成员中移除失败: " 
             << (reply ? reply->str : "无响应") << endl;
        if(reply) freeReplyObject(reply);
        return false;
    }
    
    bool success = (reply->integer == 1);
    freeReplyObject(reply);
    
    if(!success) {
        cerr << "该成员不在群组中: " << member_id << endl;
        return false;
    }
    
    // 2. 检查并可能从管理员集合中移除
    string admins_key = "group:" + group_id + ":admins";
    reply = (redisReply*)redisCommand(c, "SREM %s %s", 
                                    admins_key.c_str(), 
                                    member_id.c_str());
    
    // 管理员移除不影响主操作结果，仅记录
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "从管理员集合移除时出错: " 
             << (reply ? reply->str : "无响应") << endl;
    } else if (reply->integer == 1) {
        cout << "成员 " << member_id << " 同时被从管理员中移除" << endl;
    }
    
    if(reply) freeReplyObject(reply);
    return true;
}
//取消某人的管理员身份
bool DATA::remove_group_admin(string group_id, string member_id) {
    // 1. 验证该成员确实是管理员
    string admins_key = "group:" + group_id + ":admins";
    redisReply* reply = (redisReply*)redisCommand(c, "SISMEMBER %s %s", 
                                                admins_key.c_str(), 
                                                member_id.c_str());
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "检查管理员身份失败: " 
             << (reply ? reply->str : "无响应") << endl;
        if(reply) freeReplyObject(reply);
        return false;
    }
    
    if(reply->integer != 1) {
        cerr << "该成员不是管理员: " << member_id << endl;
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    // 2. 从管理员集合中移除
    reply = (redisReply*)redisCommand(c, "SREM %s %s", 
                                    admins_key.c_str(), 
                                    member_id.c_str());
    
    if (!reply || reply->type == REDIS_REPLY_ERROR || reply->integer != 1) {
        cerr << "从管理员集合移除失败: " 
             << (reply ? reply->str : "无响应") << endl;
        if(reply) freeReplyObject(reply);
        return false;
    }
    
    freeReplyObject(reply);
    cout << "成功将成员 " << member_id << " 从管理员降为普通成员" << endl;
    return true;
}
//创建群聊，创建并清空管理员列表，群成员列表
bool DATA::generate_group(string from_id, string message, string group_id) {
    redisReply* reply;
    
  
    string group_key = "group:" + group_id;
    reply = (redisReply*)redisCommand(c, "HSET %s name %s owner %s", group_key.c_str(), message.c_str(), from_id.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "设置群组信息失败: " << (reply ? reply->str : "无响应") << endl;
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);

    // 初始化各种集合
    string keys[] = {
        "group:" + group_id + ":admins",
        "group:" + group_id + ":members",
        "group:" + group_id + ":all_members"
    };
    
    for (auto& key : keys) {
        reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            cerr << "清空集合 " << key << " 失败" << endl;
            freeReplyObject(reply);
            return false;
        }
        freeReplyObject(reply);
    }

    // 添加群主到成员集合
    string all_members_key = "group:" + group_id + ":all_members";
    reply = (redisReply*)redisCommand(c, "SADD %s %s", all_members_key.c_str(), from_id.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR || reply->integer != 1) {
        cerr << "添加群主到成员集合失败" << endl;
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    reply = (redisReply*)redisCommand(c, "SADD groups %s", group_id.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "将群组ID添加到 groups 集合失败" << endl;
        freeReplyObject(reply);
        return false;
    }
    return true;
}
//添加群成员
bool DATA::add_group_member(string group_id, string to_id) {
    redisReply* reply;
    
    // 1. 添加到普通成员集合
    string member_key = "group:" + group_id + ":members";
    reply = (redisReply*)redisCommand(c, "SADD %s %s", member_key.c_str(), to_id.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "添加到普通成员集合失败" << endl;
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);

    // 2. 添加到所有成员集合
    string all_members_key = "group:" + group_id + ":all_members";
    reply = (redisReply*)redisCommand(c, "SADD %s %s", all_members_key.c_str(), to_id.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "添加到所有成员集合失败" << endl;
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);

    return true;
}
//删除群聊
bool DATA::delete_group(string group_id) {
    redisReply* reply;
    string owner_key = "group:" + group_id;
    reply = (redisReply*)redisCommand(c, "HGET %s owner", owner_key.c_str());
    if (!reply || reply->type != REDIS_REPLY_STRING) {
        cerr << "获取群主信息失败或群组不存在" << endl;
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);

    vector<string> keys_to_delete = {
        "group:" + group_id,                    
        "group:" + group_id + ":admins",      
        "group:" + group_id + ":members",       
        "group:" + group_id + ":all_members"    
    };

    reply = (redisReply*)redisCommand(c, "SREM groups %s", group_id.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "删除群组ID从 groups 集合失败" << endl;
        freeReplyObject(reply);
        return false;  // 删除失败，返回
    }
    freeReplyObject(reply);

   
    for (const auto& key : keys_to_delete) {
        reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            cerr << "删除键 " << key << " 失败" << endl;
            freeReplyObject(reply);
            continue;  
        }
        freeReplyObject(reply);
    }

    return true;
}
//检查是不是群主
bool DATA::is_group_owner(string group_id, string user_id) {
    redisReply* reply;
    string group_key = "group:" + group_id;
    reply = (redisReply*)redisCommand(c, "HGET %s owner", group_key.c_str());
    
    if (!reply) {
        cerr << "Redis命令执行失败" << std::endl;
        return false;
    }

    bool is_owner = false;
    if (reply->type == REDIS_REPLY_STRING) {
        is_owner = (strcmp(reply->str, user_id.c_str()) == 0);
    } else if (reply->type == REDIS_REPLY_NIL) {
        cerr << "群组不存在或没有owner字段" << endl;
    } else {
        cerr << "意外的Redis回复类型" << endl;
    }
    freeReplyObject(reply);
    return is_owner;
}
//检查是不是管理员
bool DATA::is_admin(string user_id, string group_id) {
    redisReply* reply;
    string admins_key = "group:" + group_id + ":admins";

    reply = (redisReply*)redisCommand(c, "SISMEMBER %s %s", admins_key.c_str(), user_id.c_str());
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "检查用户 " << user_id << " 是否为管理员失败" << endl;
        freeReplyObject(reply);
        return false;
    }
    
    bool is_admin = reply->integer == 1;
    freeReplyObject(reply);
    return is_admin;
}

//添加用户为管理员
bool DATA::add_admins(string user_id, string group_id) {
    redisReply* reply;
    string admins_key = "group:" + group_id + ":admins";
    
    reply = (redisReply*)redisCommand(c, "SADD %s %s", admins_key.c_str(), user_id.c_str());
    
    if (!reply || reply->type == REDIS_REPLY_ERROR || reply->integer != 1) {
        cerr << "将用户 " << user_id << " 添加到管理员集合失败" << endl;
        freeReplyObject(reply);
        return false;
    }
    string count_key = "group:" + group_id + ":admin_count";
    reply = (redisReply*)redisCommand(c, "INCR %s", count_key.c_str());
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "更新管理员数量失败" << endl;
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}
//返回管理员数量
int DATA::get_admin_count(string group_id) {
    redisReply* reply;
    string count_key = "group:" + group_id + ":admin_count";
    reply = (redisReply*)redisCommand(c, "GET %s", count_key.c_str());
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "获取管理员数量失败" << endl;
        freeReplyObject(reply);
        return -1;  
    }
    
    int admin_count = (reply->type == REDIS_REPLY_INTEGER) ? reply->integer : 0;
    freeReplyObject(reply);
    return admin_count;
}

//返回所有有该用户的群聊
vector<string> DATA::get_groups_by_user(string user_id) {
    vector<string> group_ids;
    redisReply* reply;

    reply = (redisReply*)redisCommand(c, "SMEMBERS groups");
    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        cerr << "获取群组列表失败" << endl;
        if (reply) freeReplyObject(reply);
        return group_ids;
    }
    
    for (size_t i = 0; i < reply->elements; i++) {
        string group_id = reply->element[i]->str;
        string all_members_key = "group:" + group_id + ":all_members";
        
        redisReply* member_reply = (redisReply*)redisCommand(
            c, "SISMEMBER %s %s", all_members_key.c_str(), user_id.c_str());
        
        if (member_reply && member_reply->type == REDIS_REPLY_INTEGER && member_reply->integer == 1) {
            group_ids.push_back(group_id);
        }
        
        if (member_reply) freeReplyObject(member_reply);
    }
    
    freeReplyObject(reply);
    return group_ids;
}
//根据群号找群名
string DATA::get_group_name(string group_id) {
    redisReply* reply;
    string group_name;
    
    string group_key = "group:" + group_id;
    reply = (redisReply*)redisCommand(c, "HGET %s name", group_key.c_str());
    
    if (reply && reply->type == REDIS_REPLY_STRING) {
        group_name = reply->str;
    } else {
        cerr << "获取群名失败或群组不存在" << endl;
        group_name = "";
    }
    
    if (reply) freeReplyObject(reply);
    return group_name;
}
// 查询群聊内的所有成员
vector<string> DATA::get_all_members(string group_id) {
    redisReply* reply;
    string all_members_key = "group:" + group_id + ":all_members";
    
    reply = (redisReply*)redisCommand(c, "SMEMBERS %s", all_members_key.c_str());
    vector<string> members;
    
    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        cerr << "获取群聊成员失败: " << (reply ? reply->str : "无响应") << endl;
        freeReplyObject(reply);
        return members;  // 返回空的成员列表
    }

    // 将所有成员添加到 vector 中
    for (size_t i = 0; i < reply->elements; ++i) {
        members.push_back(reply->element[i]->str);
    }
    
    freeReplyObject(reply);
    return members;
}
// 检查用户是否在群中
bool DATA::is_in_group(string group_id,string user_id) {
    
    vector<string> members = get_all_members(group_id);

    for (const auto& member : members) {
        if (member == user_id) {
            return true;  
        }
    }
    return false;  
}
//检查群聊是否存在
bool DATA::is_group_exists(string group_id) {
    redisReply* reply;
    
    reply = (redisReply*)redisCommand(c, "SISMEMBER groups %s", group_id.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "检查群组是否存在失败: " << (reply ? reply->str : "无响应") << endl;
        freeReplyObject(reply);
        return false;
    }

    bool exists = (reply->integer == 1);  
    freeReplyObject(reply);
    return exists;
}
//把加群申请添加到申请表
bool DATA::apply_to_group(string applicant_id, string group_id,string message) {
    // 构造申请记录
    string apply_value = applicant_id + ":" + message + "|unread";
    
    redisReply* reply = (redisReply*)redisCommand(
        c, 
        "RPUSH group:apply:%s %s", 
        group_id.c_str(), 
        apply_value.c_str()
    );
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "申请加群失败: " << (reply ? reply->str : "无响应") << endl;
        if(reply) freeReplyObject(reply);
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}
//检查申请状态是否为status
bool DATA::check_group_status(string applicant_id, string group_id, string status) {
  
    redisReply* reply = (redisReply*)redisCommand( c, "LRANGE group:apply:%s 0 -1",  group_id.c_str());
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "检查申请状态失败: " << (reply ? reply->str : "无响应") << endl;
        if(reply) freeReplyObject(reply);
        return false;
    }

    for (size_t i = 0; i < reply->elements; i++) {
        string apply_value = reply->element[i]->str;
        size_t pos = apply_value.find(":");
        if (pos != string::npos) {
            string id = apply_value.substr(0, pos);
            if (id == applicant_id) {
                string status_value = apply_value.substr(pos + 1);
                pos = status_value.find("|");
                if (pos != string::npos) {
                    string field_status = status_value.substr(pos + 1);
                    freeReplyObject(reply);
                    return (field_status == status);  
                }
            }
        }
    }

    freeReplyObject(reply);
    return false;
}
//修改群聊申请状态
bool DATA::revise_group_status(string applicant_id, string group_id, string status) {
    // 检查 Redis 连接
    if (c == NULL || c->err) {
        cerr << "Redis 连接异常: " << (c ? c->errstr : "连接未初始化") << endl;
        return false;
    }

    // 获取所有申请记录
    redisReply* reply = (redisReply*)redisCommand(c, "LRANGE group:apply:%s 0 -1", group_id.c_str());
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "获取申请列表失败: " << (reply ? reply->str : "无响应") << endl;
        if (reply) freeReplyObject(reply);
        return false;
    }

    if (reply->elements == 0) {
        cerr << "错误：该群组没有申请记录，group:apply:" << group_id << " 不存在或为空" << endl;
        freeReplyObject(reply);
        return false;
    }
  
    for (size_t i = 0; i < reply->elements; i++) {
        string apply_value = reply->element[i]->str;
        size_t id_pos = apply_value.find(":");
        if (id_pos != string::npos) {
            string id = apply_value.substr(0, id_pos);
            if (id == applicant_id) {
                // 提取原始消息内容（去掉原有状态）
                size_t status_pos = apply_value.find("|");
                string message = (status_pos != string::npos) ? 
                    apply_value.substr(id_pos + 1, status_pos - id_pos - 1) :
                    apply_value.substr(id_pos + 1);
                
                // 构建新的申请记录
                string new_apply_value = id + ":" + message + "|" + status;
                cout<<"new_apply_is"<<new_apply_value<<endl;
                cout<<"i:"<<i<<endl;
                // 替换可能的非法字符（如换行符）
                size_t newline_pos = new_apply_value.find('\n');
                if (newline_pos != string::npos) {
                    new_apply_value.replace(newline_pos, 1, " ");
                }

                // 更新记录
                redisReply* update_reply = (redisReply*)redisCommand(
                    c, 
                    "LSET group:apply:%s %d %s", 
                    group_id.c_str(), 
                    i, 
                    new_apply_value.c_str()
                );
                cout<<"LSET group:apply:" <<group_id<<i<<new_apply_value<<endl; 
                    
                if (!update_reply || update_reply->type == REDIS_REPLY_ERROR) {
                    cerr << "更新申请状态失败: " << (update_reply ? update_reply->str : "无响应") << endl;
                    if (update_reply) freeReplyObject(update_reply);
                    freeReplyObject(reply);
                    return false;
                }

                freeReplyObject(update_reply);
                freeReplyObject(reply);
                return true;
            }
        }
    }

    cerr << "未找到申请人 " << applicant_id << " 的申请记录" << endl;
    freeReplyObject(reply);
    return false;
}

//获取为status的申请并返回整个值
vector<pair<string, string>> DATA::get_unread_applications(string group_id) {
    vector<pair<string, string>> result;
    
    string redis_key = "group:apply:" + group_id;
    
    redisReply* reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", redis_key.c_str());
    
    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        if (reply) freeReplyObject(reply);
        return result;
    }
    
    for (size_t i = 0; i < reply->elements; i++) {
        string entry = reply->element[i]->str;
        
        size_t colon_pos = entry.find(':');
        size_t pipe_pos = entry.find('|');
        
        if (colon_pos == string::npos || pipe_pos == string::npos) {
            continue; 
        }
        
        string from_id = entry.substr(0, colon_pos);
        string message = entry.substr(colon_pos + 1, pipe_pos - colon_pos - 1);
        string status = entry.substr(pipe_pos + 1);
        
        if (status == "unread"||status == "send") {
            result.emplace_back(from_id, message);
        }
    }
    
    freeReplyObject(reply);
    return result;
}
//实时发送群申请
vector<pair<string, string>> DATA::get_group_applications(string group_id) {
    vector<pair<string, string>> result;
    
    string redis_key = "group:apply:" + group_id;
    
    redisReply* reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", redis_key.c_str());
    
    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        if (reply) freeReplyObject(reply);
        return result;
    }
    
    for (size_t i = 0; i < reply->elements; i++) {
        string entry = reply->element[i]->str;
        
        size_t colon_pos = entry.find(':');
        size_t pipe_pos = entry.find('|');
        
        if (colon_pos == string::npos || pipe_pos == string::npos) {
            continue; 
        }
        
        string from_id = entry.substr(0, colon_pos);
        string message = entry.substr(colon_pos + 1, pipe_pos - colon_pos - 1);
        string status = entry.substr(pipe_pos + 1);
        
        if (status == "unread") {
            result.emplace_back(from_id, message);
            revise_group_status(from_id,group_id,"send") ;
        }
    }
    
    freeReplyObject(reply);
    return result;
}
//将消息添加到群消息列表
bool DATA::add_group_message(string from_id, string message, string group_id, string timestamp) {
    // 值简化为 from_id|message
    string value = from_id + "|" + message;
    
    redisReply* reply = (redisReply*)redisCommand(
        c, 
        "ZADD chat:%s %s %s",  // score=timestamp, value=from|message
        group_id.c_str(),
        timestamp.c_str(),
        value.c_str()
    );

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "存储消息失败: " << (reply ? reply->str : "无响应") << endl;
        if (reply) freeReplyObject(reply);
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}
//设置某用户在某群的最后已读时间
bool DATA::set_last_read_time(string user_id,string group_id,string timestamp) {
    redisReply* reply = (redisReply*)redisCommand(
        c,
        "SET user:last_read:%s:%s %s",user_id.c_str(),group_id.c_str(),timestamp.c_str()
    );

    bool read_success = (reply && reply->type == REDIS_REPLY_STATUS && 
                        strcmp(reply->str, "OK") == 0);
    
    bool notify_success = set_last_notified_time(user_id, group_id, timestamp);
    if (!read_success || !notify_success) {
        cerr << "设置时间失败 - 已读:" << (read_success ? "成功" : "失败")
             << " 通知:" << (notify_success ? "成功" : "失败") << endl;
        return false;
    }
    freeReplyObject(reply);
    return true; 
}
//设置最后通知时间
bool DATA::set_last_notified_time(const string& user_id, const string& group_id, const string& timestamp) {
    redisReply* reply = (redisReply*)redisCommand(
        c,
        "SET user:last_notified:%s:%s %s",
        user_id.c_str(), group_id.c_str(), timestamp.c_str()
    );

    bool success = (reply && reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0);
    
    if (!success) {
        cerr << "设置最后通知时间失败: " 
             << (reply ? reply->str : "无响应") << endl;
    }
    
    freeReplyObject(reply);
    return success;
}
//获取所有未通知的消息
vector<pair<string, string>> DATA::get_unnotice_messages(string user_id, string group_id) {
   string max_time;
   redisReply* reply = (redisReply*)redisCommand(
        c, "GET user:last_notified:%s:%s", user_id.c_str(), group_id.c_str()
    );
    string last_notified = "0";
    if (reply && reply->type == REDIS_REPLY_STRING) {
        last_notified = reply->str;
    }
    freeReplyObject(reply);

    // 3. 获取所有未读消息（基于最后已读时间）
    reply = (redisReply*)redisCommand(
        c, "ZRANGEBYSCORE chat:%s (%s +inf WITHSCORES",  
        group_id.c_str(),last_notified.c_str()
    );

    vector<pair<string, string>> messages;
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i += 2) {
            if (i+1 >= reply->elements) break;
            
            string timestamp = reply->element[i+1]->str;
            string full_message = reply->element[i]->str;

            // 只返回未通知的消息（时间戳 > last_notified）
            if (timestamp > last_notified) {
                size_t colon_pos = full_message.find('|');
                if (colon_pos != string::npos) {
                    messages.emplace_back(
                        full_message.substr(0, colon_pos),
                        full_message.substr(colon_pos + 1)
                    );
                } else {
                    messages.emplace_back("unknown", full_message);
                }
                 max_time = timestamp;
            }

        }
    }
    if (!messages.empty()) {
        set_last_notified_time(user_id, group_id, max_time);
    }
    freeReplyObject(reply);
    return messages;
}
//获取所有小于最后已读时间的
vector<tuple<string, string, string>> DATA::get_read_messages(string user_id, string group_id) {
    // 1. 获取用户最后阅读时间
    redisReply* reply = (redisReply*)redisCommand(
        c,
        "GET user:last_read:%s:%s",
        user_id.c_str(),
        group_id.c_str()
    );

    string last_read = "0"; // 默认从最早的消息开始
    if (reply && reply->type == REDIS_REPLY_STRING) {
        last_read = reply->str;
        // 验证时间戳是否为合法数字
        if (!std::all_of(last_read.begin(), last_read.end(), ::isdigit)) {
            cerr << "警告: 非法的最后阅读时间格式，重置为0: " << last_read << endl;
            last_read = "0";
        }
    }
    freeReplyObject(reply);

    // 2. 获取已读消息（带时间戳）
    reply = (redisReply*)redisCommand(
        c,
        "ZRANGEBYSCORE chat:%s -inf %s WITHSCORES",
        group_id.c_str(),
        last_read.c_str()
    );

    vector<tuple<string, string, string>> messages;
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i += 2) {
            if (i+1 >= reply->elements) break;
            
            string full_message = reply->element[i]->str;
            string timestamp = reply->element[i+1]->str;

            // 解析 fromid 和 message
            size_t separator = full_message.find('|');
            if (separator != string::npos) {
                string fromid = full_message.substr(0, separator);
                string content = full_message.substr(separator + 1);
                messages.emplace_back(fromid, content, timestamp);
            } else {
                // 处理格式错误的情况
                cerr << "警告: 消息格式错误: " << full_message << endl;
                messages.emplace_back("unknown", full_message, timestamp);
            }
        }
    } else if (!reply) {
        cerr << "错误: Redis命令执行失败" << endl;
    } else if (reply->type != REDIS_REPLY_ARRAY) {
        cerr << "错误: 预期返回数组，实际得到类型: " << reply->type << endl;
    }
    
    if (reply) freeReplyObject(reply);
    return messages;
}
// 获取用户未读消息
vector<pair<string, string>> DATA::get_unread_messages(string user_id, string group_id) {
  
   redisReply* reply = (redisReply*)redisCommand(
        c,
        "GET user:last_read:%s:%s",
        user_id.c_str(),
        group_id.c_str()
    );
    string last_read = "0"; 
    if (reply && reply->type == REDIS_REPLY_STRING) {
        last_read = reply->str;
    }
    freeReplyObject(reply);

    // 获取未读消息（带分数，即时间戳）
    reply = (redisReply*)redisCommand(
        c,
        "ZRANGEBYSCORE chat:%s (%s +inf WITHSCORES",  
        group_id.c_str(),
        last_read.c_str()
    );

    vector<pair<string, string>> messages; // pair<fromid, message>
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i += 2) {
            if (i+1 >= reply->elements) break;
            
            string full_message = reply->element[i]->str; // 格式假设是 "fromid:message"
            string timestamp = reply->element[i+1]->str;  // 时间戳（这里不用，但保留解析）

            // 解析 fromid 和实际消息内容
            size_t colon_pos = full_message.find('|');
            if (colon_pos != string::npos) {
                string fromid = full_message.substr(0, colon_pos);
                string content = full_message.substr(colon_pos + 1);
                messages.emplace_back(fromid, content);
            } else {
                // 如果没有 ':'，默认整个字符串是消息，fromid 设为空或默认值
                messages.emplace_back("unknown", full_message);
            }
        }
    }
    
    freeReplyObject(reply);
    return messages;
}
//把群文件添加到redis
bool DATA:: add_file_group(string group_id,string from_id,string filename) {
 
    string redis_key = "file:" + group_id;
    string value = from_id + "|" + filename;

    redisReply *reply = (redisReply *)redisCommand(c, "RPUSH %s %s", redis_key.c_str(), value.c_str());
    if (!reply) {
        std::cerr << "Redis 命令执行失败" << std::endl;
        return false;
    }

    bool success = false;
    if (reply->type == REDIS_REPLY_INTEGER) {
        success = (reply->integer >= 1);
    } else {
        std::cerr << "Redis 返回类型错误" << std::endl;
    }
    freeReplyObject(reply);
    return success;
}
//获取群里某成员的未读文件
vector<string> DATA::get_unread_file_group(string group_id,string user_id)
 {
    string unread_key = "file:unread:" + group_id + ":" + user_id;
    redisReply *reply = (redisReply *)redisCommand(c, "SMEMBERS %s", unread_key.c_str());

    std::vector<std::string> unread_files;
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i++) {
            unread_files.push_back(reply->element[i]->str);
        }
    }

    freeReplyObject(reply);
    return unread_files;
}
//移除群里某成员的未读文件列表
bool DATA::remove_unread_file(string group_id,string user_id,string file_value) {
    string unread_key = "file:unread:" + group_id + ":" + user_id;
    redisReply *reply = (redisReply *)redisCommand(c, "SREM %s %s", unread_key.c_str(), file_value.c_str());

    bool success = false;
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        success = (reply->integer == 1); // 返回1表示成功移除
    }

    freeReplyObject(reply);
    return success;
}
//把文件添加到群所有成员未读列表
bool DATA:: add_file_to_unread_list(string group_id,string from_id,string filename) {
   
    string file_value = from_id + "|" + filename;

    vector<string> members = get_all_members(group_id);
    for (const auto &user_id : members) {
        if (user_id == from_id) {
            continue; // 不给自己添加未读提醒
        }

        string unread_key = "file:unread:" + group_id + ":" + user_id;

        redisReply *reply = (redisReply *)redisCommand(
            c, "SADD %s %s", unread_key.c_str(), file_value.c_str()
        );

        if (!reply || reply->type != REDIS_REPLY_INTEGER) {
            std::cerr << "添加未读文件失败: " << (reply ? "无效返回类型" : "命令执行失败") << std::endl;
            if (reply) freeReplyObject(reply);
            return false;
        }

        freeReplyObject(reply);
    }

    return true;
}
//删除群聊天记录
bool DATA::clear_group_messages(string group_id) {
    redisReply* reply = (redisReply*)redisCommand(
        c, 
        "DEL chat:%s",  // 直接删除整个键
        group_id.c_str()
    );

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "清空群消息失败: " << (reply ? reply->str : "无响应") << endl;
        if (reply) freeReplyObject(reply);
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}
//删除群申请列表
bool DATA::clear_group_request(string group_id) {
    redisReply* reply = (redisReply*)redisCommand(
        c, 
        "DEL group:apply:%s",  // 直接删除整个键
        group_id.c_str()
    );

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "删除申请列表失败: " << (reply ? reply->str : "无响应") << endl;
        if (reply) freeReplyObject(reply);
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}
//删除文件列表
bool DATA::clear_group_files( string group_id) {
    string redis_key = "file:" + group_id;
    redisReply* reply = (redisReply*)redisCommand(
        c,
        "DEL %s",  // 直接删除整个文件列表
        redis_key.c_str()
    );

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "删除群文件列表失败: " << (reply ? reply->str : "无响应") << endl;
        if (reply) freeReplyObject(reply);
        return false;
    }

    freeReplyObject(reply);
    return true;
}

//发送序列化字符串
void TCP::send_m(int data_socket, string type, string message) {
    nlohmann::json j;
    j["type"] = type;
    j["message"] = message;
    
    string serialized_message = j.dump();
    
    // 准备发送缓冲区
    vector<char> send_buf(sizeof(uint32_t) + serialized_message.size());
    
    // 写入长度前缀
    uint32_t len = htonl(serialized_message.size());
    memcpy(send_buf.data(), &len, sizeof(len));
    
    // 写入数据
    memcpy(send_buf.data() + sizeof(len), serialized_message.data(), serialized_message.size());
    
    if (send(data_socket, send_buf.data(), send_buf.size(), 0) != send_buf.size()) {
        cerr << "服务器发送失败" << endl;
    }
    //cout<<"已发送"<<serialized_message<<endl;
}

// 服务器接收
bool TCP::rec_m(string &type, string &from_id, string &to_id, string &message, int data_socket) {
    static vector<char> buffer;  // 保存未处理的数据
    static size_t expected_len = 0;  // 期望接收的消息体长度
    
    // 先读取长度头
    if (expected_len == 0) {
        uint32_t len;
        ssize_t bytes = recv(data_socket, &len, sizeof(len), 0);
        
        if (bytes == 0) 
        {
            cout<<"断开连接"<<endl;
            return false;
        }
        if (bytes != sizeof(len)) {
            // 可能只收到了部分长度头，保存到buffer待下次处理
            buffer.insert(buffer.end(), (char*)&len, (char*)&len + bytes);
            return false;
        }
        
        expected_len = ntohl(len);
        buffer.reserve(expected_len);
    }
    
    // 接收消息
    if (buffer.size() < expected_len) {
        size_t remaining = expected_len - buffer.size();
        vector<char> temp_buf(min(remaining, (size_t)BUFFER_SIZE));
        
        ssize_t bytes = recv(data_socket, temp_buf.data(), temp_buf.size(), 0);
        if (bytes <= 0) throw runtime_error("接收失败");
        
        buffer.insert(buffer.end(), temp_buf.begin(), temp_buf.begin() + bytes);
        
        if (buffer.size() < expected_len) {
            return false;  //小于长度
        }
    }
    
    // 解析消息
    try {
        auto parsed = json::parse(string(buffer.begin(), buffer.end()));
        type = parsed["type"];
        from_id = parsed["from_id"];
        to_id = parsed["to_id"];
        message = parsed["message"];
        
        // 处理缓冲区
        if (buffer.size() > expected_len) {
            vector<char> remaining(buffer.begin() + expected_len, buffer.end());
            buffer = std::move(remaining);
            expected_len = 0;  
        } else {
            buffer.clear();
            expected_len = 0;
        }
    } catch (const json::parse_error& e) {
        buffer.clear();
        expected_len = 0;
        throw runtime_error("JSON解析失败: " + string(e.what()));
    }
    return true;
}
//创建传输文件套接字
int TCP::new_transfer_socket(int data_socket) {

  int  transfer_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(transfer_socket == -1) {
        cerr << "传输套接字创建失败" << endl;
        return -1;
    }

    int transfer_port = generate_port(); 
    

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(transfer_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(transfer_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "传输套接字绑定失败" << endl;
        close(transfer_socket);
        return -1;
    }

    if(listen(transfer_socket, 1) == -1) { 
        cerr << "传输套接字监听失败" << endl;
        close(transfer_socket);
        return -1;
    }

    send(data_socket, &transfer_port, sizeof(transfer_port), 0);
    
    int transfer_conn = accept(transfer_socket, nullptr, nullptr);
    if(transfer_conn == -1) {
        cerr << "传输连接接受失败" << endl;
        close(transfer_socket);
        return -1;
    }

    cout << "传输套接字已建立，端口：" << transfer_port << endl;
    return transfer_conn; 
}
int TCP::new_notice_socket(int data_socket) {

  int notice_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(notice_socket == -1) {
        cerr << "传输套接字创建失败" << endl;
        return -1;
    }

    int notice_port = generate_port(); 
    

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(notice_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(notice_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "传输套接字绑定失败" << endl;
        close(notice_socket);
        return -1;
    }

    if(listen(notice_socket, 1) == -1) { 
        cerr << "传输套接字监听失败" << endl;
        close(notice_socket);
        return -1;
    }

    send(data_socket, &notice_port, sizeof(notice_port), 0);
    
    int transfer_conn = accept(notice_socket, nullptr, nullptr);
    if(transfer_conn == -1) {
        cerr << "传输连接接受失败" << endl;
        close(notice_socket);
        return -1;
    }

    cout << "传输套接字已建立，端口：" << notice_port << endl;
    return transfer_conn; 
}
//生成随机id
int Random_id(){
    
    std::srand(static_cast<unsigned int>(std::time(0)));
    int random_number = std::rand() % 900000 + 100000;

    return random_number;
 }

//创建服务器套接字
TCP::TCP():pool(10) {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {  
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CONNECTIONS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    Threadpool pool(4);
    std::cout << "Server listening on port " << PORT << "...\n";
 }

//与客户端建立连接
void TCP::start(DATA &redis_data) {
    int client_socket,epoll_fd,nfds;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    struct epoll_event ev,events[MAX_EVENTS];

    epoll_fd = epoll_create1(0);
    if(epoll_fd == -1)
    {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }
    //fcntl(server_socket,F_SETFL,O_NONBLOCK);

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_socket;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_socket,&ev) == -1)
    {
        perror("epoll_ctl failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
       
        nfds = epoll_wait(epoll_fd,events,MAX_EVENTS,-1);
        if(nfds == -1)
        {
            perror("epoll_wait failed");
            exit(EXIT_FAILURE);
        }
        
        
        for(int i = 0;i < nfds;i++)
        {
            cout<<"nfds==" <<nfds<<endl;
            if(events[i].data.fd == server_socket)
            {
            client_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if(client_socket == -1)
            {
            perror("Accept failed");
            continue;
            }
              std::cout << "有新的客户端连接\n";
              //fcntl(client_socket,F_SETFL,NONBLOCK);
            
            ev.events = EPOLLIN | EPOLLET;  // 监听可读事件和边缘触发
            ev.data.fd = client_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev) == -1) {
                    perror("epoll_ctl failed for client_socket");
                    exit(EXIT_FAILURE);
                }
            }else{
            client_socket = events[i].data.fd;
            // string command = rec_m(client_socket);
            char buffer[1024];
            int bytes = recv(client_socket,buffer, sizeof(buffer) - 1, 0);
        if(bytes > 0)
        {
        buffer[bytes] = '\0';
       
        }
        else if (bytes == 0) {
                    // 客户端断开连接
                    std::cout << "客户端已断开连接\n";
                    close(client_socket);  
                } 
        // string recived_message = string(buffer);
        string data(buffer,bytes);
            redisContext* cn = redis_data.data_create();
            redis_data.c = cn;
            //redisContext* c = data.data_create();
            LOGIN login(this);
            
            if(data  =="login")
            {
                cout<<"当前操作：登陆"<<endl;
            
                //在这里创建数据套接字，下面都用数据套接字传送消息
                int data_socket = new_socket(client_socket);
                
            pool.enqueue([this, data_socket,&redis_data, &login](){  
            if (login.login_user(data_socket, redis_data)) {
            cout<<"用户已成功登陆"<<endl;
            startHeartbeatMonitor();
            this->make_choice(data_socket,redis_data);
           // stopHeartbeatMonitor();
             close(data_socket);//进入登陆后的选项  
            }
           // close(data_socket);
        });
            }else if(data == "register")
            {
            cout<<"当前操作：注册"<<endl;
            int data_socket = new_socket(client_socket);
            pool.enqueue([data_socket,&redis_data,&login](){
            //cout << "Registering user..." << endl;
            login.register_user(data_socket,redis_data);
            });
           
            //close(data_socket);
            }else if(data  == "deregister")
            {
                cout<<"当前操作：注销"<<endl;
                int data_socket = new_socket(client_socket);
                pool.enqueue([data_socket ,&redis_data,&login](){
             login.deregister_user(data_socket,redis_data);
            });
            //close(data_socket);
            }else if(data == "quit")
            {
                cout<<"当前操作：退出"<<endl;
                close(client_socket);
                break;
            }else if (data == "") {
            //std::cout << "Client disconnected\n";
            cout << "No bytes received\n";
            }

            }
   
    }
    }
  
    close(epoll_fd); 
    //close(client_socket);
 }
//生成随机端口
int TCP::generate_port(){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distr(PASV_PORT_MIN, PASV_PORT_MAX);
    return distr(gen);
    }
//在登陆成功后创建数据套接字来发送消息
int TCP::new_socket(int client_socket){
    int data_socket;
    data_socket = socket(AF_INET,SOCK_STREAM,0);
    if(data_socket == -1)
    {
        cerr<<"数据套接字创建失败"<<endl;
        return -1;
    }

    int data_port = generate_port();
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if(bind(data_socket,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1)
    {
        cerr<<"Binding failed"<<endl;
        close(data_socket);
        return -1;
    }

    if(listen(data_socket,5) == -1){
        cerr<<"Listen failed"<<endl;
        close(data_socket);
        return -1;
    }

    send(client_socket,&data_port,sizeof(data_port),0);
    cout<<"data socket created on port"<<data_port<<endl;
   
    int data_connection_socket = accept(data_socket, NULL, NULL);
    if (data_connection_socket == -1) {
        perror("Accept failed");
        close(data_socket);
        return -1;
    }
    return data_connection_socket;}
//登陆后已知用户id寻找其socket
int TCP:: find_socket(const string& user_id) {
    for (const auto& pair : logged_users) {
        if (pair.second == user_id) {
          // cout<<"找到"<<user_id<<"对应的socket"<<endl;
            return pair.first; 
        }
    }
    return -1; 
 }
//已知id寻找socket
string TCP::find_user_id(int socket) {
    for (const auto& pair : logged_users) {
        if (pair.first == socket) {
            //cout << "找到socket " << socket << " 对应的user_id" << pair.second<<endl;
            return pair.second; // Return the user_id corresponding to the socket
        }
    }
    return ""; // Return empty string if socket not found
}
void TCP::remove_user(int data_socket)
{
    auto it = logged_users.find(data_socket);
    if(it != logged_users.end())
    {
        string user_id = it->second;
        logged_users.erase(it);
        user_count--;
        
    }
}
  // 启动心跳监测线程
void TCP ::startHeartbeatMonitor() {
        monitoring_ = true;
        monitor_thread_ = std::thread([this]() {
            while (monitoring_) {
                checkHeartbeats();
                cout<<"检查心跳监测"<<endl;
                std::this_thread::sleep_for(std::chrono::seconds(10)); // 每10秒检查一次
            }
        });
        monitor_thread_.detach();
    }

    // 停止心跳监测
void TCP::stopHeartbeatMonitor() {
        monitoring_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }
void TCP:: checkHeartbeats() {
        std::unique_lock lock(heartbeat_mutex_);
        auto now = std::chrono::steady_clock::now();
        
        for (auto it = client_last_heartbeat_.begin(); it != client_last_heartbeat_.end(); ) {
            auto duration = now - it->second;
            if (duration > std::chrono::minutes(1)) {
                std::cout << "客户端 " << it->first << " 心跳超时，关闭连接" << std::endl;
                close(it->first);
                remove_user(it->first);  // 清理用户数据
                it = client_last_heartbeat_.erase(it);
            } else {
                ++it;
            }
        }
    }
//关闭所有在线的客户端数据套接字
TCP:: ~TCP(){
     std::cerr << "TCP对象被析构，socket=" << server_socket << std::endl;
        for(auto& user : logged_users){
            int close_socket = user.first;
            close(close_socket);
            cout<<"已关闭套接字"<<close_socket<<endl;
        }
     //stopHeartbeatMonitor();
        close(server_socket);
    }
void TCP:: notice_sender_thread(int notice_socket, atomic<bool>& running,DATA &redis_data,string user_id) {
    while (running) {
        this_thread::sleep_for(chrono::seconds(3)); 
        if (notice_socket == -1) break;
        string notice;
        //notice = "有新消息";
        notice_message(redis_data,user_id,notice);
        //检查有没有新消息,传为message
        if (!notice.empty()) {
            // 2. 通过专用套接字发送
            if (send(notice_socket, notice.c_str(), notice.size(), 0) <= 0) {
                cerr << "通知发送失败，连接可能已断开" << endl;
                break;
            }
        }
        
        
        
    }
    close(notice_socket); // 退出时清理
}
void TCP::notice_message(DATA& redis_data, string user_id,string &notice) {
        
    vector<string> message;
    vector<string> fromids;
     vector<pair<string, string>> unreadFiles;      
    redis_data.get_messages_2(user_id, fromids, message);
    string a = redis_data.check_add_request_and_revise(user_id);
    //redis_data.process_and_mark_unread_files(user_id, unreadFiles);
    unordered_map<string, string> latest_messages;
   // redis_data.get_unread_files(user_id, unreadFiles);
    // if (!unreadFiles.empty()) {
    //     for (const auto& file : unreadFiles) {
    //         cout<<"用户"<<user_id<<"有新文件"<<endl;
    //         notice += "有来自" + redis_data.get_username_by_id(file.first) + 
    //                    "(" + file.first + ")" + "的新文件" + file.second + "\n";
    //     }
    // } 
    if(!a.empty()&&a!="null")
    {
       // cout<<"a is"<<a<<endl;
        notice += "\033[0;33m有新的好友申请，请前往查看\033[0m";
    }
    if (!message.empty()) {
               // notice = "\033[0;33m有新消息\033[0m"; 
    for (size_t i = 0; i < fromids.size(); i++) {
         latest_messages[fromids[i]] = message[i];
     } 
    for (const auto& pair : latest_messages) {
    notice += redis_data.get_username_by_id(pair.first) +  "(" + pair.first + ")" + ": " + pair.second + "\n";
            }        
    }
        vector<string> group_ids = redis_data.get_groups_by_user(user_id);
    if (!group_ids.empty()) {
    for (const auto& group_id : group_ids) {
        vector<pair<string, string>> applications= redis_data.get_group_applications(group_id);
        //群申请
         if (!applications.empty() && (redis_data.is_group_owner(group_id, user_id) || redis_data.is_admin(user_id, group_id))) {
                for (const auto& app : applications) {
                    notice += "群" +  redis_data.get_group_name(group_id) +  
                              "(" + group_id + ")" + "有来自" +  redis_data.get_username_by_id(app.first) + 
                              "(" + app.first + ")" + "的申请消息\n";
                }
            } 
        //群文件
        // vector<string> unread_file = redis_data.get_unread_file_group(group_id, user_id);
        // if (!unread_file.empty()) {
        // notice += "群" +  redis_data.get_group_name(group_id)  + 
        //                    "(" + group_id + ")" + "有新文件\n";
        //     }
        //群消息
        vector<pair<string, string>> group_messages = redis_data. get_unnotice_messages(user_id,group_id);;
            if (!group_messages.empty()) {
                notice += "群" + redis_data.get_group_name(group_id)  + 
                           "(" + group_id + ")" + "有新消息\n";
            }
            
    }
    
    }
    }

//登陆注册注销的函数
void LOGIN::register_user(int data_socket,DATA &redis_data){
    char buffer1[1024] = {0};
    char buffer2[1024] = {0};
    int user_id;
   
    int bytes_rec1 = recv(data_socket, buffer1, sizeof(buffer1), 0); 
    if(bytes_rec1 < 0)
    {
        cout<<"注册时用户名接收失败"<<endl;
        perror("recv error");
        return;
    }
    string username(buffer1, bytes_rec1);
    cout<<"注册时接收到用户名"<<username<<endl;
    
    if(redis_data.check_username_duplicate(username))
    {
        const char* message = "该用户名已被注册!";
        send(data_socket, message, strlen(message), 0);
        return;
    }

    int bytes_rec2= recv(data_socket, buffer2, 1024, 0);
    if(bytes_rec2 < 0)
    {
        cout<<"注册时密码接收失败"<<endl;
     
    }

    string password(buffer2, bytes_rec2);
    cout<<"注册时接收到密码"<<username<<endl;
    user_id = Random_id();
    string user_id_str = to_string(user_id);

    if(redis_data.add_user(user_id_str,username,password))
    {
     string message = "注册成功啦！用户ID: \033[1;36m" + user_id_str + "\033[0m"; 
    cout<<"用户注册成功"<<endl;
    
    send(data_socket, message.c_str(), message.length(), 0);
    }else{
    const char* message = "注册失败,再试一次吧TAT！";
    send(data_socket, message, strlen(message), 0);
    }
   cout <<"已关闭数据套接字"<<endl;
   close(data_socket);
 }
bool LOGIN ::login_user(int data_socket,DATA &redis_data){
    char buffer[1024] = {0};
    char buffer2[1024] = {0};
    string username;
    int bytes_rec1 = recv(data_socket, buffer, 1024, 0); 
    if(bytes_rec1 < 0)
    {
        cout<<"登陆时用户id接收失败"<<endl;
        return false;
   
    }else{
    
        cout<<"登陆时接收到用户id"<<buffer<<endl;
     }
    buffer[bytes_rec1] = '\0';
    string user_id(buffer,bytes_rec1);
   

    //检查用户是否存在
    username =redis_data.get_username_by_id(user_id);
    if(username == "")
    {
    cout << user_id<<"用户不存在于数据库中" << endl;
    const char* message = "用户不存在!";
    send(data_socket, message, strlen(message), 0);
    return false;
    }
    
    cout << "用户名: " << username << endl;
    

    int bytes_rec2= recv(data_socket, buffer2, 1024, 0);
    if(bytes_rec2 < 0)
    {
        cout<<"登陆时密码接收失败"<<endl;
         return false;
    }
    cout<<"登陆时接收到密码"<<buffer2<<endl;
    buffer2[bytes_rec2] = '\0'; 
    std::string password(buffer2,bytes_rec2);
    //检查密码是否正确
    
    if (redis_data.check_password(password,user_id)) {
        if(server->find_socket(user_id) != -1)
        {
        string message = "请勿重复登陆";   
        send(data_socket, message.c_str(), message.size(), 0);
        close(data_socket);
        return false;
        }
    string message = "用户:"+username+ "登陆成功啦！";
    send(data_socket, message.c_str(), message.size(), 0);
    
     
    if (server->logged_users.find(data_socket) == server->logged_users.end()) {
            
        server->logged_users[data_socket] = user_id;
        server->user_count++;
        cout << "套接字为：" << data_socket<<"的客户端" << " 已经成功登陆id为"<<user_id <<"的账号"<< std::endl;
           
     }
    return true;
    } else {
    const char* message = "密码错误TAT";
    send(data_socket, message, strlen(message), 0);
    cout<<"用户输入密码不正确，登陆失败"<<endl;
    return false;
    }
     close(data_socket);
     cout <<"已关闭数据套接字"<<endl;
     return false;
 }

//注销函数
void LOGIN::deregister_user(int data_socket,DATA &redis_data){
    char buffer[1024] = {0};
    char buffer2[1024] = {0};
   //接收用户id
    int bytes_rec1 = recv(data_socket, buffer, 1024, 0); 
    if(bytes_rec1 < 0)
    {
        cout<<"注销时用户id接收失败"<<endl;
    }
    buffer[bytes_rec1] = '\0';
    string user_id(buffer, bytes_rec1);
    memset(buffer, 0, sizeof(buffer));
    cout<<"注销用户的id为:"<<user_id<<endl;
    
    //确认用户是否存在
    string username;
    username = redis_data.get_username_by_id(user_id);
    if(username!="")
    {
    cout << "注销时用户存在" << endl;
    }else{
    const char* message = "注销时用户不存在!";
    send(data_socket, message, strlen(message), 0);
    cout << "注销时用户不存在" << endl;
    return;
    }

    //接收用户密码
    int bytes_rec2= recv(data_socket, buffer2, 1024, 0);
    if(bytes_rec2 < 0)
     {
        cout<<"注销时密码接收失败"<<endl;
     }
    buffer2[bytes_rec2] = '\0'; 
    string password(buffer2, bytes_rec2);

    //比较密码是否正确
    if(redis_data.check_password(password,user_id))
    {
        cout<<"注销时用户输入密码与数据库中密码相同"<<endl;
        if(redis_data.delete_user(user_id)&&redis_data.delete_message_logs(user_id)&&redis_data.delete_file_records(user_id))
        {
        vector<string>friends_id;
        redis_data.see_all_friends(user_id,friends_id);
        if(friends_id.size() != 0)
        {
            for(int i = 0;i < friends_id.size();i++)
            {
                redis_data.rdelete_friend(user_id,friends_id[i]);
            }
        }

    //删除所有群聊部分，包括群主退出群解散，管理员退出，成员退出
    vector<string>group_ids;
    group_ids = redis_data.get_groups_by_user(user_id);
    if(group_ids.size() != 0)
    {
        for(int i = 0;i < group_ids.size();i++)
        {
            if(redis_data.is_group_owner(group_ids[i],user_id))
            {
                redis_data.delete_group(group_ids[i]);
                redis_data.clear_group_files(group_ids[i]);
                redis_data.clear_group_request(group_ids[i]);
                redis_data.clear_group_messages(group_ids[i]);
                continue;
            }
            if(redis_data.is_admin(user_id,group_ids[i]))
            {
                redis_data.remove_group_admin(group_ids[i],user_id);
            }
            redis_data.remove_group_member(group_ids[i], user_id);
            //是群主的话解散群聊
            
         
        }
        }
        cout<<"已成功注销用户相关信息"<<endl;
        const char* message = "注销成功啦";
        send(data_socket,message,strlen(message),0);
        }else{
        const char* message = "注销失败";
        send(data_socket,message,strlen(message),0);
        return;
        }
    }else{
        cout<<"注销时用户密码输入错误"<<endl;
    const char* message = "密码错误TAT";
    send(data_socket,message,strlen(message),0);
        return;
 }
  
  
    
    cout <<"已关闭数据套接字"<<endl;
     close(data_socket);
 }

//登陆成功之后选择后续操作
void TCP::make_choice(int data_socket,DATA &redis_data){
    
    FRI friends(this);
    GRO group(this);
    //建立通知套接字
    int notice_socket = new_notice_socket(data_socket);
    atomic<bool> running(true);
    
   std::thread notice_thread(
        [this, notice_socket, &running,&redis_data,data_socket]() {
            this->notice_sender_thread(notice_socket, running,redis_data,this->find_user_id(data_socket));
        }
    );
    notice_thread.detach();
    //发送实时消息给客户端
   // start_notice_thread(data_socket,redis_data,find_user_id(data_socket));
    string type,to_id,from_id,message;
    recived_message(redis_data,find_user_id(data_socket),data_socket);
    while(1)
    {
        if(!rec_m(type,from_id,to_id,message,data_socket))
        {
            break;
        }
        if(type == "send_add_friends_request")
        {
            cout<<"接收到命令：添加好友"<<endl;
            friends.send_add_request(*this,data_socket,to_id,from_id,message,redis_data);
        }else if(type  == "delete_friend")
        {
            cout<<"接收到命令:删除好友"<<endl;
            friends.delete_friend(*this,data_socket,to_id,from_id,redis_data);
        }else if(type  == "quit")
        {
            cout<<"接收到命令：退出登陆"<<endl;
            running = false;
            remove_user(data_socket);
           // close(data_socket);
            break;
        }else if(type == "check_add_friends_request")
        {
            cout<<"接收到命令：查看该用户收到的好友申请"<<endl;
            int m = 0;
            friends.check_add_friends_request(*this,data_socket,from_id,redis_data,m);
        }else if(type == "approve_friends_requests")
        {
            cout<<"接收到命令：通过好友申请"<<endl;
            friends.add_friend(*this,data_socket,to_id,from_id,redis_data);
             //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "see_all_friends")
        {
            cout<<"接收到命令:查看所有好友"<<endl;
            friends.see_all_friends(*this,data_socket,from_id,redis_data);
            recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "refuse_friends_requests")
        {
            cout<<"接收到命令:拒绝好友申请"<<endl;
            friends.refuse_friend_request(*this,data_socket,to_id,from_id,redis_data);
           // recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "send_message")
        {
           cout<<"接收到命令：发送消息"<<endl;
           friends.send_message(*this,data_socket,from_id,to_id,message,redis_data);
            recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "nothing")
        {
            recived_message(redis_data,find_user_id(data_socket),data_socket);
            //continue;
        }else if(type == "friend_open_block")
        {
            cout<<"接收到命令，打开聊天框"<<endl;
            friends.open_block(*this,data_socket,from_id,to_id,redis_data);
           // recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "shield_friend")
        {
            cout<<"接收到命令：屏蔽好友";
            friends.shield_friend(*this,data_socket,from_id,to_id,redis_data);
        }else if(type == "cancel_shield_friend")
        {
            cout<<"接收到命令：取消屏蔽好友";
            friends.cancel_shield_friend(*this,data_socket,from_id,to_id,redis_data);
        }else if(type == "send_message_no")
        {
            cout<<"接收到命令：发送消息2"<<endl;
           friends.send_message(*this,data_socket,from_id,to_id,message,redis_data);
            
        }else if(type == "new_message")
        {
             cout<<"接收到命令：查看新消息"<<endl;
             friends.new_message(*this,data_socket,from_id,to_id,redis_data);
        }else if(type == "send_file")
        {
            cout<<"接收到命令：发送文件"<<endl;
            friends.send_file(*this,data_socket,from_id,to_id,message,redis_data);
         recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "accept_file")
        {
            cout<<"接收到命令：接收文件"<<endl;
            friends.accept_file(*this,data_socket,from_id,to_id,message,redis_data);
             recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "is_friends")
        {
            cout<<"收到命令：私聊好友"<<endl;
            friends.is_friends(*this,data_socket,from_id,to_id,message,redis_data);
            //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "generate_group")
        {
            cout<<"收到命令：创建群聊"<<endl;
            group.generate_group(*this,data_socket,from_id,to_id,message,redis_data);
          
        }else if(type == "delete_group")
        {
            cout<<"收到命令：删除群聊"<<endl;
            group.delete_group(*this,data_socket,from_id,to_id,message,redis_data);
           
        }else if(type == "get_all_my_group")
        {   cout<<"收到命令：查看所有群聊"<<endl;
            group.see_all_group(*this,data_socket,from_id,to_id,message,redis_data);
            recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "see_all_member")
        {
            cout<<"收到命令：查看群所有成员"<<endl;
            group.see_all_member(*this,data_socket,from_id,to_id,message,redis_data);
             recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "add_admin")
        {
            group.add_admin(*this,data_socket,from_id,to_id,message,redis_data);
             recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "open_group")
        {
            group.open_group(*this,data_socket,from_id,to_id,message,redis_data);
            //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "quit_group")
        {
            group.quit_group(*this,data_socket,from_id,to_id,message,redis_data);
            recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "delete_admin")
        {
              group.delete_admin(*this,data_socket,from_id,to_id,message,redis_data);
               recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "add_member")
        {
             group.add_member(*this,data_socket,from_id,to_id,message,redis_data);
              recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "delete_member")
        {
            group.delete_member(*this,data_socket,from_id,to_id,message,redis_data);
             recived_message(redis_data,find_user_id(data_socket),data_socket);
            //group.quit_group(data_socket,from_id,to_id,message,redis_data);
        }else if(type == "send_add_group")
        {
            group.send_add_group(*this,data_socket,from_id,to_id,message,redis_data);
        }else if(type == "see_add_group")
        {
            group.see_add_group(*this,data_socket,from_id,to_id,message,redis_data);
            //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type ==  "approve_group_requests")
        {
            group.add_group_member(*this,data_socket,from_id,to_id,message,redis_data);
             recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "refuse_group_requests")
        {
             group.refuse_group_member(*this,data_socket,from_id,to_id,message,redis_data);
              recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type ==  "send_message_group")
        {
            group.add_group_message(*this,data_socket,from_id,to_id,message,redis_data);
        }else if(type == "receive_group_new")
        {
            group.receive_group_message(*this,data_socket,from_id,to_id,message,redis_data);
        }else if(type == "group_open_block")
        {
            group.open_group_block(*this,data_socket,from_id,to_id,message,redis_data);
        }else if(type ==  "send_file_group")
        {
            cout<<"收到命令：上传群文件"<<endl;
            group.send_file_group(*this,data_socket,from_id,to_id,message,redis_data);
             recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "accept_file_group")
        {
            cout<<"收到命令：下载群文件"<<endl;
             group.accept_file_group(*this,data_socket,from_id,to_id,message,redis_data);
              recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "heart")
        {
            cout<<"接收到客户端心跳监测"<<endl;
             updateHeartbeat(data_socket);
        }
        else{
            cout<<"接收到命令：未知命令"<<endl;
            cout<<"未知命令为"<<type<<endl;
            break;
        }
     }
 }

void FRI::accept_file(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data) {
    vector<string> result;
    string response;
    redis_data.get_files(to_id, from_id, result); // 获取文件列表
    
    if (result.empty()) {
        cout << "无文件" << endl;
        string response = "NO_FILES";
        send(data_socket, response.c_str(), response.size(), 0);
        return;
    }
    response += "有"+to_string(result.size())+"个文件\n";
    for(int i = 0;i < result.size();i++)
    {
        response +=to_string(i+1)+"."+ result[i] + "\n";
    }
    
    send(data_socket, response.c_str(), response.size(), 0);
    
    char choice_buf[16];
    int bytes = recv(data_socket, choice_buf, sizeof(choice_buf), 0);
    if(bytes <= 0) {
        cerr << "接收选择失败" << endl;
        return;
    }
    choice_buf[bytes] = '\0';
    int choice = stoi(choice_buf) - 1;

    if(choice < 0 || choice >= result.size()) {
        send(data_socket, "INVALID_CHOICE", 14, 0);
        return;
    }

    
    int transfer_socket = client.new_transfer_socket(data_socket);
    if(transfer_socket == -1) {
        cerr << "创建传输套接字失败" << endl;
        return;
    }

    thread([&client,transfer_socket,to_id,from_id,result,choice]()
    {
    string filename = to_id + ":" + from_id + ":" + result[choice];
    ifstream file(filename, ios::binary);
    if(!file) {
        send(transfer_socket, "FILE_NOT_FOUND", 14, 0);
        close(transfer_socket);
        return;
    }

    // 先发送文件名
    string base_name = result[choice];
    send(transfer_socket, base_name.c_str(), base_name.size(), 0);
    char ack_buf[4];
    int ack_bytes = recv(transfer_socket, ack_buf, sizeof(ack_buf), 0);
    if (ack_bytes <= 0 || string(ack_buf, ack_bytes) != "ACK") {
    cerr << "客户端未确认或确认失败" << endl;
    close(transfer_socket);
    return;
    }
    // 发送文件内容
    char buffer[BUFFER_SIZE];
    while(!file.eof()) {
        file.read(buffer, BUFFER_SIZE);
        int bytes_read = file.gcount();
        if(send(transfer_socket, buffer, bytes_read, 0) != bytes_read) {
            cerr << "文件发送中断" << endl;
            break;
        }
    }
    file.close();

    close(transfer_socket);
    cout<<"发送成功"<<endl;
   
}).detach();
     redis_data.revise_file_status(to_id,from_id,result[choice]);
}
void FRI::is_friends(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    string t = redis_data.get_username_by_id(to_id);
    if(t == "")
    {
        string recover = "用户不存在";
        cout<<recover<<endl;
        client.send_m(data_socket,"other",recover);
        return;
    }
    if(!redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
        string a= "你与该用户还不是好友";
         client.send_m(data_socket,"other",a);
        return;
    }
    if(redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
         string a= "你已经屏蔽该用户";
         client.send_m(data_socket,"other",a);
        return;
    }
    string a= "成功";
    client.send_m(data_socket,"other",a);
}
//给登陆的用户一直发送实时消息通知
void TCP::recived_message(DATA &redis_data, string user_id, int data_socket) {
    string type = "accept";
    string data1;
    vector<string> fromids;
    vector<string> messages;
    string message;
    unordered_map<string, string> latest_messages;
    
    // 绿色ANSI颜色代码
    const string GREEN = "\033[36m";
    const string RESET = "\033[0m";
    
    // 有没有人通过我的好友请求
    if (redis_data.check_recived_recover(user_id, type, data1)) {
        message = "有用户通过了你的好友请求\n" + data1 + "\n";
    }
    
    type = "refuse";
    string data2;
    // 有没有人拒绝我的好友请求
    if (redis_data.check_recived_recover(user_id, type, data2)) {
        message.append("\n");
        message.append("有用户拒绝了你的好友申请\n");
        message.append(data2);
    }
    
    // 有没有未读消息
    if (redis_data.get_messages(user_id, fromids, messages)) {
        string result_message;
        if (fromids.empty() || messages.empty()) {
            result_message = "没有未读消息\n";
        } else {
            for (size_t i = 0; i < fromids.size(); i++) {
                latest_messages[fromids[i]] = messages[i];
            }
            result_message = "有" + to_string(latest_messages.size()) + "位联系人的未读消息:";
            
            for (const auto& pair : latest_messages) {
                result_message += "\n来自 " + GREEN + redis_data.get_username_by_id(pair.first) + RESET + 
                                "(" + pair.first + ")" + ": " + pair.second + "\n";
            }
        }
        message += result_message;
    }
    
    // 有没有好友申请
    string request = redis_data.check_add_request(user_id);
    if (request != "null") {
        message += "有新的好友申请\n";
    }
    
    //有没有未读文件
    vector<pair<string, string>> unreadFiles;
    redis_data.get_unread_files(user_id, unreadFiles);
    if (!unreadFiles.empty()) {
        for (const auto& file : unreadFiles) {
            message += "有来自" + GREEN + redis_data.get_username_by_id(file.first) + RESET + 
                       "(" + file.first + ")" + "的新文件" + file.second + "\n";
        }
    } 
    
    // 有没有加群申请和群未读消息
    vector<string> group_ids = redis_data.get_groups_by_user(user_id);
    if (!group_ids.empty()) {
        for (const auto& group_id : group_ids) {
            vector<pair<string, string>> applications = redis_data.get_unread_applications(group_id);
            if (!applications.empty() && (redis_data.is_group_owner(group_id, user_id) || redis_data.is_admin(user_id, group_id))) {
                for (const auto& app : applications) {
                    message += "群" + GREEN + redis_data.get_group_name(group_id) + RESET + 
                              "(" + group_id + ")" + "有来自" + GREEN + redis_data.get_username_by_id(app.first) + RESET + 
                              "(" + app.first + ")" + "的申请消息\n";
                }
            }

            vector<pair<string, string>> group_messages = redis_data.get_unread_messages(user_id, group_id);
            if (!group_messages.empty()) {
                message += "群" + GREEN + redis_data.get_group_name(group_id) + RESET + 
                           "(" + group_id + ")" + "有新消息\n";
            }
            
            vector<string> unread_file = redis_data.get_unread_file_group(group_id, user_id);
            if (!unread_file.empty()) {
                message += "群" + GREEN + redis_data.get_group_name(group_id) + RESET + 
                           "(" + group_id + ")" + "有新文件\n";
            }
        }
    }
    send_m(data_socket,"other",message);
    //send(data_socket, message.c_str(), message.size(), 0);
}

void FRI::send_file(TCP &client, int data_socket, string from_id, string to_id, string message, DATA &redis_data) {
    int transfer_socket = client.new_transfer_socket(data_socket);
    if (transfer_socket == -1) {
        cerr << "无法创建传输套接字" << endl;
        return;
    }

    // 启动独立线程处理文件接收（非阻塞）
    std::thread([&client, transfer_socket, data_socket, from_id, to_id, message, &redis_data]() {
        string full_path = message;
        string filename;
        this_thread::sleep_for(chrono::milliseconds(150));
        // 提取文件名
        size_t t = full_path.find_last_of('/');
        if (t != string::npos) {
            filename = full_path.substr(t + 1);
        } else {
            filename = full_path;
        }
        string the_file = filename;
        filename = from_id + ":" + to_id + ":" + filename;
        cout<<"filename is"<<filename<<endl;

        // 打开文件
        ofstream file(filename, ios::binary);
        if (!file) {
            cerr << "上传失败，无法创建文件" << endl;
            string error_msg = "UPLOAD_ERROR";
            send(data_socket, error_msg.c_str(), error_msg.size(), 0);
            close(transfer_socket);
            return;
        }

        // 接收文件内容
        char buffer[BUFFER_SIZE];
        int total_received = 0;
        bool transfer_complete = false;

        while (true) {
            int bytes_recv = recv(transfer_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_recv <= 0) {
                if (bytes_recv == 0) {
                    transfer_complete = true; // 正常关闭
                }
               // cout<<"我在发送"<<endl;
                break;
            }
            file.write(buffer, bytes_recv);
            total_received += bytes_recv;
        }
        file.close();

        // 处理传输结果
        if (transfer_complete) {
            // Redis 操作（加锁保证线程安全）
            bool redis_ok = false;
            {
                
                std::lock_guard<std::mutex> lock(redis_mutex);
                redis_ok = redis_data.add_file(from_id, to_id, the_file);
            }

            if (redis_ok) {
                cout << "文件接收成功: " << filename << endl;
                string ack = "SUCCESS";
                send(transfer_socket, ack.c_str(), ack.size(), 0);
            } else {
                remove(filename.c_str()); // Redis 失败，删除文件
                cerr << "Redis记录失败，已删除文件" << endl;
                string err = "REDIS_ERROR";
                send(transfer_socket, err.c_str(), err.size(), 0);
            }
        } else {
            remove(filename.c_str()); // 传输不完整，删除文件
            cerr << "文件接收不完整，已删除" << endl;
            string err = "TRANSFER_ERROR";
            send(transfer_socket, err.c_str(), err.size(), 0);
        }

        // 关闭套接字
        close(transfer_socket);
    }).detach(); // 分离线程，自动释放资源
}
//实时发送新消息
void FRI :: new_message(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data)
{
     string recover;
    // while(1)
    // {
    vector<string> messages;
    vector<string> fromids;
    vector<string> message;
    string username =redis_data.get_username_by_id(to_id);
    //redis_data.get_id_messages(from_id,to_id, messages);
    
      if(!redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
        string a= "你与该用户还不是好友";
         client.send_m(data_socket,"other",recover);
        cout<<a<<endl;
        return;
    }
    if(!redis_data.is_friend(to_id,from_id))
    {
        string a= "对方已把你拉黑";
        client.send_m(data_socket,"other",recover);
        cout<<a<<endl;
        return;
    }
     redis_data.get_id_messages(from_id,to_id, messages);
     redis_data.get_messages_2(from_id,fromids,message); 
     if(message.empty())
     {
         //cout<<"没有新消息"<<endl;

     }else{
        recover = "\033[0;33m有新消息\033[0m"; 
        client.send_m(data_socket,"other",recover);
    
     }
     if(messages.empty())
     {
        //cout<<"没有新消息"<<endl;
        // continue;
        recover = "没有新消息";
        client.send_m(data_socket,"other",recover);
        return;
     }else {
     for (int i = 0;i < messages.size();i++) {
        //recover += username+":" +messages[i]  + "\n";
 recover += std::string("\033[1;36m") + "[" + username + "]" + "\033[0m" + ":" + messages[i] + "\n";
        //cout << ":" << messages[i] << endl;
    }
    // break;
 // }
 }
    //client.send_m()
    client.send_m(data_socket,"other",recover);
}
void FRI::cancel_shield_friend(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data)
{
     if(redis_data.is_friend(to_id,from_id) &&redis_data.is_friend(from_id,to_id))
    {
        const char* a= "你未屏蔽该用户";
        client.send_m(data_socket,"other",a);
        return;
    }
    redis_data.add_friends(to_id,from_id);
    const char*recover = "已成功解除屏蔽好友";
    cout<<recover<<endl;
     client.send_m(data_socket,"other",recover);
}
//屏蔽好友
void FRI::shield_friend(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data)
{
    if(!redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
        string a= "你与该用户还不是好友";
         client.send_m(data_socket,"other",a);
        return;
    }
    if(redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
         string a= "你已经屏蔽该用户";
         client.send_m(data_socket,"other",a);
        return;
    }
    
    redis_data.rdelete_friend(to_id,from_id);
    string recover = "已成功屏蔽好友";
    //cout<<recover<<endl;
   client.send_m(data_socket,"other",recover);
}
//打开聊天框，发送历史记录
void FRI::open_block(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data)
{
    vector<string> message1;
    vector<string> message2;
    vector<string> messages;
    string recover;

    string username1 =redis_data.get_username_by_id(from_id);
    string username2 =redis_data.get_username_by_id(to_id);
   
    string username =redis_data.get_username_by_id(to_id);
    if(username == "")
    {
    //cout << user_id<<"用户不存在于数据库中" << endl;
    string a= "用户不存在";
    client.send_m(data_socket,"other",a);
    return ;
    }
      if(!redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
        string a= "你与该用户还不是好友";
       client.send_m(data_socket,"other",a);
        cout<<a<<endl;
        return;
    }
    redis_data.see_all_other_message(to_id, from_id, message1);
    if(from_id != to_id)
    {
    redis_data.see_all_my_message(from_id, to_id, message2);
    }
    if(message1.size() == 0 && message2.size() == 0)
    {
        recover = "暂无聊天记录";
        client.send_m(data_socket,"other",recover);
        return;
    }
    messages.insert(messages.end(), message1.begin(), message1.end());
    messages.insert(messages.end(), message2.begin(), message2.end());

    std::sort(messages.begin(), messages.end(), [](const std::string& a, const std::string& b) {
        size_t pipe_pos1 = a.rfind('|'); 
        size_t pipe_pos2 = b.rfind('|');
        
        std::string timestamp_a = a.substr(pipe_pos1 + 1);
        std::string timestamp_b = b.substr(pipe_pos2 + 1);
        
        return std::stoll(timestamp_a) < std::stoll(timestamp_b); 
    });
  if(messages.size() > 50) {
        messages.erase(messages.begin(), messages.end() - 50);
    }
    for (auto& msg : messages) {
        size_t pipe_pos = msg.rfind('|'); 
        std::string timestamp = msg.substr(pipe_pos + 1); 
        
        if (std::find(message1.begin(), message1.end(), msg) != message1.end()) {
            msg = "\033[0;36m["+username2 + "]:\033[0m" + msg.substr(0, pipe_pos); 
        }
        else {
            msg = msg.substr(0, pipe_pos) + ":[" + username1+"]"; 
        }
    }
    for (auto& msg : messages) {
        size_t pipe_pos = msg.rfind('|'); 
        msg = msg.substr(0, pipe_pos); 
    }

    for (const auto& msg : messages) {
        recover += msg + "\n";
        //cout << ":" << msg << endl;
    }
    
    client.send_m(data_socket,"other",recover);
    //cout<<"发送成功"<<endl;
   
}
//选择添加好友操作
void FRI::send_add_request(TCP &client,int data_socket,string to_id,string from_id,string message,DATA &redis_data){
    int bytes;
    char buffer[1024];
    string t = redis_data.get_username_by_id(to_id);
    if(t == "")
    {
        string recover = "要添加的用户不存在";
        cout<<recover<<endl;
        client.send_m(data_socket,"other",recover);
        return;
    }
    if(redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
        string a= "你与该用户已经是好友了";
         client.send_m(data_socket,"other",a);
        return;
    }
    if(redis_data.check_add_friend_request_duplicata(to_id,from_id))
    {
        string recover = "已向该用户发送过好友申请，请不要重复发送";
        client.send_m(data_socket,"other",recover);
        return;
    }
    string status = "unread";
    if(!redis_data.add_friends_request(from_id,to_id,message,status))
    {
        string recover = "将好友申请添加到好友申请表中失败";
        cout<<recover<<endl;
        client.send_m(data_socket,"other",recover);
        return;
    }

    string recover = "已成功发送好友申请";
    cout<<recover<<endl;
    client.send_m(data_socket,"other",recover);
    
    }
void FRI:: delete_friend(TCP &client,int data_socket,string to_id,string from_id,DATA &redis_data){

    if(!redis_data.is_friend(to_id,from_id) ||!redis_data.is_friend(from_id,to_id))
    {
        string a= "你与该用户还不是好友";
        client.send_m(data_socket,"other",a);
        return;
    }
    redis_data.rdelete_friend(to_id,from_id);
    redis_data.rdelete_friend(from_id,to_id);
    string recover = "已成功删除好友";
    //cout<<recover<<endl;
   client.send_m(data_socket,"other",recover);
}
//选择查看好友申请操作
void FRI:: check_add_friends_request(TCP &client,int data_socket,string from_id,DATA &redis_data,int &m){
    string serialized_data;
    serialized_data=redis_data.check_add_request(from_id);
    if(serialized_data.empty() || serialized_data == "[]" || serialized_data == "null")
    {
        cout<<"该用户无好友申请"<<endl;
        serialized_data = "无好友申请";
         client.send_m(data_socket,"other",serialized_data);
        m = 1;
        return;
    }
    cout<<serialized_data<<endl;
     client.send_m(data_socket,"other",serialized_data);
    cout<<"发送该用户的好友申请给客户端"<<endl;

 }
void FRI::add_friend(TCP &client,int data_socket,string to_id,string from_id,DATA &redis_data){
    string status ="accept";
    if(redis_data.check_dealed_request(to_id,from_id,status))
    {
        
         string d = "已同意过该好友申请，请勿重复处理";
        client.send_m(data_socket,"other",d);
        return; 
    }
    string other_status = "refuse";
    if(redis_data.check_dealed_request(to_id,from_id,other_status))
    {
         string d = "已拒绝过该好友申请，请勿重复处理";
        client.send_m(data_socket,"other",d);
        return;  
    }
    //将to_id添加到from_id的好友列表
    redis_data.add_friends(to_id,from_id);
    redis_data.add_friends(from_id,to_id);
    //status ="accept";
    if(redis_data.revise_status(to_id,from_id,status))
    {
        //cout<<"111"<<endl;
        //初始化好友消息列表
        //redis_data.add_message_log(from_id,to_id);
        //string message = "you are friends now ovo!";
        // if(!redis_data.add_message_log(from_id,to_id,message))
        // {
        //     cout<<"失败"<<endl;

        // }
        string d = "添加好友成功！";
        cout<<d<<endl;
      client.send_m(data_socket,"other",d);
        return;
    }
    string d = "添加好友失败！";
   client.send_m(data_socket,"other",d);
}
//选择查看好友列表操作
void FRI::see_all_friends(TCP &client, int data_socket, string from_id, DATA &redis_data) { 
    vector<string> buffer;
    redis_data.see_all_friends(from_id, buffer);

    std::ostringstream oss; // 使用字符串流来格式化输出
    oss << "user_id   username    online_status\n";
    
    for (const auto& friend_id : buffer) {
        string friend_name = redis_data.get_username_by_id(friend_id);
        string online_status = "在线";
        if (client.find_socket(friend_id) == -1) {
            online_status = "离线";
        }

        // 使用流操作符 << 来格式化输出
        oss << friend_id << "    " 
            << std::setw(10) << std::left << friend_name << "    " 
            << (online_status == "在线" ? "\033[32m" : "\033[37m") 
            << std::setw(10) << std::left << online_status << "\033[0m\n";
    }

    string friend_list = oss.str(); // 获取最终的格式化字符串
    client.send_m(data_socket, "other", friend_list);
}
//拒绝好友申请
void FRI::refuse_friend_request(TCP &client,int data_socket,string to_id,string from_id,DATA &redis_data){
    string status ="accept";
    if(redis_data.check_dealed_request(to_id,from_id,status))
    {
        string d = "已同意过该好友申请，请勿重复处理";
        client.send_m(data_socket,"other",d);
    }
    string other_status = "refuse";
    if(redis_data.check_dealed_request(to_id,from_id,other_status))
    {
         string d = "已拒绝过该好友申请，请勿重复处理";
       client.send_m(data_socket,"other",d);
        return; 
    }
    //string status ="refuse";
    if(redis_data.revise_status(to_id,from_id,other_status))
    {
        const char* d = "已拒绝该好友申请";
        client.send_m(data_socket,"other",d);
        return;
    }
    string d = "拒绝好友申请失败！";
   client.send_m(data_socket,"other",d);
}
//向好友发送消息
void FRI:: send_message(TCP &client,int data_socket,string from_id,string to_id,string message,DATA &redis_data){
   
  if(redis_data.add_message_log(from_id,to_id,message))
           {
            string a= "消息发送成功";
           //send(data_socket,a.c_str(),a.size(),0);
           }else{
            string a= "消息发送失败";
           // send(data_socket,a.c_str(),a.size(),0);
           }
}
int GRO::generateNumber() {
    // 生成随机数的种子
    std::srand(std::time(0));
    
    // 生成一个五位数
    int randomNumber = std::rand() % 90000 + 10000;
    
    return randomNumber;
}
void GRO::generate_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data) {
    vector<string> ids;
    stringstream ss(to_id);
    string id;
    string response;
    int group = generateNumber();
    string group_id = to_string(group);

    while (getline(ss, id, ':')) {
        if (!id.empty()) {
            if(!redis_data.is_friend(id,from_id) && !redis_data.is_friend(from_id,id))
            {
                response+= "你与"+id+"还不是好友,无法添加他到群聊\n";            
            }else if(!redis_data.is_friend(id,from_id)&&redis_data.is_friend(from_id,id))
            {
                response +="你已屏蔽"+id+"，无法添加他到群聊\n";
            }else{
            ids.push_back(id);
            //cout<<id<<endl;
            }
        }
    }
    if(ids.size() < 2)
    {
        response+= "群聊最少三人\n"; 
       client.send_m(data_socket,"other",response);
        return;
    }
    if (!redis_data.generate_group(from_id, message, group_id)) {
        cerr << "群组初始化失败" << endl;
        client.send_m(data_socket,"other",response);
        return;
    }

    bool all_success = true;
    for (auto& member_id : ids) {
        if (!redis_data.add_group_member(group_id, member_id)) {
            cerr << "添加成员 " << member_id << " 失败" << endl;
            all_success = false;
        }
    }
    if (all_success) {
        response += "创建群聊成功，群号: " + group_id;
        client.send_m(data_socket,"other",response);
    } else {
       client.send_m(data_socket,"other",response);;
    }
}
void GRO::delete_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    if(!redis_data.is_group_owner(message,from_id))
    {
        string response = "只有群主可以解散群聊";
       client.send_m(data_socket,"other",response);
       return;
    }
                            
    if(redis_data.delete_group(message)&&redis_data.clear_group_files(message)&&redis_data.clear_group_request(message)&&redis_data.clear_group_messages(message))
    {
        string response = "删除群聊成功";
        client.send_m(data_socket,"other",response);
    }//删除群聊基本信息
    
}
void GRO::see_all_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    vector<string> groups;
    string recover = "group_id      group_name";
    groups = redis_data.get_groups_by_user(from_id);
    if(groups.size() == 0)
    {
        string response = "用户暂无加入的群";
        client.send_m(data_socket,"other",response);
        return;
    }
    for(int i = 0;i < groups.size();i++)
    {
        recover += "\n"+groups[i]+"      "+redis_data.get_group_name(groups[i]);
    }
    client.send_m(data_socket,"other",recover);
}
void GRO::see_all_member(TCP &client, int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    vector<string> members; 
    members = redis_data.get_all_members(message);
    
    if(members.empty())  
    {
        client.send_m(data_socket, "other", "该群组没有成员");
        return;
    }
    
    std::ostringstream oss;
 
    oss << "\033[1;36m"  // 青色加粗
        << std::setw(15) << std::left << "user_id" << "    "
        << std::setw(15) << std::left << "username" << "    "
        << std::setw(10) << std::left << "status" << "    "
        << std::setw(10) << std::left << "class"
        << "\033[0m" << endl;  // 重置颜色
    
    for (const auto& member_id : members) {
        string member_name = redis_data.get_username_by_id(member_id);
        string online_status = (client.find_socket(member_id) == -1) ? "离线" : "在线";
        string member_class = "成员";
        
        if(redis_data.is_group_owner(message, member_id)) {
            member_class = "\033[1;31m群主\033[0m";  // 红色加粗
        } else if(redis_data.is_admin(member_id, message)) {
            member_class = "\033[1;33m管理员\033[0m";  // 黄色加粗
        } else {
            member_class = "\033[37m成员\033[0m";  // 白色
        }
        
        // 格式化输出
        oss << std::setw(15) << std::left << member_id << "    "
            << std::setw(15) << std::left << member_name << "    "
            << (online_status == "在线" ? "\033[32m" : "\033[37m")  // 在线绿色，离线白色
            << std::setw(10) << std::left << online_status << "\033[0m" << "    "
            << member_class << "\n";
    }
    
    client.send_m(data_socket, "other", oss.str());
}
void GRO::add_admin(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    string response;
    string group_id = message;
     if(!redis_data.is_in_group(message,to_id))
    {
        response = "该用户不在该群聊内\n";
        //send(data_socket, response.c_str(), response.size(), 0);
        //return;
    }
    if(!redis_data.is_group_owner(message,from_id))
    {
        response += "只有群主可以添加管理员\n";
        client.send_m(data_socket,"other", response);
        return;
    }
    
    if(redis_data.is_admin(to_id,group_id))
    {
    response = "该用户已经是管理员了";
     client.send_m(data_socket,"other", response);
    return;
    }
    if(redis_data.get_admin_count(group_id) > 5)
    {
    response = "管理员上限为5人";
     client.send_m(data_socket,"other", response);
    return;
    }
    if(redis_data.add_admins(to_id,group_id))
    {
    response = "添加该用户为管理员成功";
     client.send_m(data_socket,"other", response);
    }
    
 
}
void GRO::open_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    if(!redis_data.is_group_exists(message))
    {
        string response = "该群不存在";
        client.send_m(data_socket,"other", response);
       return;
    }
    if(!redis_data.is_in_group(message,from_id))
    {
        string response = "你不在该群聊内";
        client.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.is_group_owner(message,from_id))
    {
       string response = "群主";
        client.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.is_admin(from_id,message))
    {
        string response = "管理员";
        client.send_m(data_socket,"other", response);
        return;
    }
    string response = "成员";
    client.send_m(data_socket,"other", response);
}
void GRO::quit_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    if(redis_data.is_group_owner(message,from_id))
    {
        if(redis_data.delete_group(message)&&redis_data.clear_group_files(message)&&redis_data.clear_group_request(message)&&redis_data.clear_group_messages(message))
        {
        string response = "已解散群聊";
         client.send_m(data_socket,"other", response);
        return;
        }

    }
    if(redis_data.is_admin(from_id,message))
    {
        redis_data.remove_group_admin(message,from_id);
    }
    if(redis_data.remove_group_member(message,from_id))
    {
        string response = "已成功退出";
         client.send_m(data_socket,"other", response);
        return;
    }
}
void GRO::delete_admin(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    if(!redis_data.is_in_group(message,to_id))
    {
     string response = "该用户不在该群聊内\n";
       client.send_m(data_socket,"other", response);
        return;
    }
    if(!redis_data.is_group_owner(message,from_id))
    {
       string response = "只有群主可以移除管理员\n";
        client.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.is_admin(to_id,message))
    {
        redis_data.remove_group_admin(message,to_id);
    }else{
        string response = "该用户不是管理员";
       client.send_m(data_socket,"other", response);
        return;
    }
    
        string response = "已成功移除";
      client.send_m(data_socket,"other", response);
        
    
}
void GRO::add_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
   
     string username =redis_data.get_username_by_id(to_id);
    if(username == "")
    {
    //cout << user_id<<"用户不存在于数据库中" << endl;
    string a= "用户不存在";
    client.send_m(data_socket,"other", a);
    return ;
    }
    if(redis_data.is_in_group(message,to_id))
    {
        string response = "用户已在该群聊内";
        client.send_m(data_socket,"other", response);
        return;
    }
    if(!redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
        string a= "你与该用户不是好友";
        client.send_m(data_socket,"other", a);
        cout<<a<<endl;
        return;
    }
    if(!redis_data.add_group_member(message,to_id))
    {
    string response = "邀请失败";
    client.send_m(data_socket,"other", response);
            return;
 }
    string response = "已成功邀请";
  client.send_m(data_socket,"other", response);
}
void GRO::send_add_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
   if(!redis_data.is_group_exists(to_id))
    {
        string response = "该群不存在";
        client.send_m(data_socket,"other", response);
       return;
    }
     if(redis_data.is_in_group(to_id,from_id))
    {
        string response = "你已在该群聊内";
         client.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.apply_to_group(from_id,to_id,message))
    {
        string response = "已成功发送申请";
         client.send_m(data_socket,"other", response);
       
    } 
}
void GRO::see_add_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{

   vector<pair<string, string>> applications = redis_data.get_unread_applications(message);
   string resultStr;
   if(applications.size() == 0)
   {
    resultStr = "无申请消息"; 
    client.send_m(data_socket,"other", resultStr);
     return;
   }
   resultStr+="有"+to_string(applications.size())+"条新的申请\n";
 for (const auto& application : applications) {
    resultStr += "有来自：" + redis_data.get_username_by_id(application.first)+"("+application.first+")" + "的申请，验证消息为： " + application.second + "\n";
 }
 client.send_m(data_socket,"other", resultStr);
}
void GRO::add_group_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    string status = "accept";
    string other_status = "refuse"; 
    if(redis_data.is_in_group(message,to_id))
    {
        string response = "用户已在该群聊内";
        client.send_m(data_socket,"other", response);
        return;
    }
     if(redis_data.check_group_status(to_id, message, status))
    {
        string response = "该申请已被同意，请勿重复处理";
       client.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.check_group_status(to_id, message, other_status))
    {
        string response = "该申请已被拒绝，请勿重复处理";
        client.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.revise_group_status(to_id,message,status))
    {
        redis_data.add_group_member(message,to_id);
        string response = "已同意该申请";
       client.send_m(data_socket,"other", response);
       
    }
}
void GRO::refuse_group_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    string status = "accept";
    string other_status = "refuse"; 
    if(redis_data.is_in_group(message,to_id))
    {
        string response = "用户已在该群聊内";
       client.send_m(data_socket,"other", response);
        return;
    }
     if(redis_data.check_group_status(to_id, message, status))
    {
        string response = "该申请已被同意，请勿重复处理";
        client.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.check_group_status(to_id, message, other_status))
    {
        string response = "该申请已被拒绝，请勿重复处理";
       client.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.revise_group_status(to_id,message,other_status))
    {
        //redis_data.add_group_member(message,to_id);
        string response = "已拒绝该申请";
       client.send_m(data_socket,"other", response);
       
    }
}
void GRO::add_group_message(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    string time = getCurrentTimestamp();
    //发送消息的时候还在群聊里面，更新一边最后时间
    if(redis_data.add_group_message(from_id,message, to_id,time)&&redis_data.set_last_read_time(from_id,to_id,time))
    {
         string a= "消息发送成功";
         cout<<a<<endl;
    }
}
void GRO::receive_group_message(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    //每次调用接收函数的时候也在群聊里面，更新最后在线时间

     if(!redis_data.is_in_group(to_id,from_id))
    {
        string response = "你已被踢出群聊";
        client.send_m(data_socket,"other", response);
        return;
    }else{
    vector<pair<string, string>> messages = redis_data.get_unread_messages(from_id, to_id);
    string result;
         if(messages.size() == 0)
    {
        string result= "没有新消息";
        client.send_m(data_socket,"other", result);
        return;
         //如果没有接收到消息，就不更新时间戳
    }
    for (const auto& [fromid, content] : messages) {
    result += "[" + fromid + "] " + content + "\n";
    }
     //cout << result << endl;
    client.send_m(data_socket,"other", result);
    }
    string time = getCurrentTimestamp();
    redis_data.set_last_read_time(from_id,to_id,time);
}
void GRO::open_group_block(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
    vector<tuple<string, string, string>> messages = redis_data.get_read_messages(from_id, to_id);
    if(messages.size() == 0)
    {
        string result= "群聊暂无消息";
        client.send_m(data_socket,"other", result);
        return;
    }
     sort(messages.begin(), messages.end(), 
        [](const auto& a, const auto& b) {
            return get<2>(a) < get<2>(b);  
        });

    
    stringstream ss;
    for (const auto& [fromid, message, timestamp] : messages) {
        ss << "[" <<redis_data.get_username_by_id( fromid )<< "] " << fromid << ": " << message << "\n";
    }

    string result = ss.str();
    if (!result.empty()) {
        result.pop_back();  // 移除最后一个换行符
    }
    client.send_m(data_socket,"other",result);
    //cout<<"已发送"<<result<<endl;
}
void GRO::send_file_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{
     int transfer_socket = client.new_transfer_socket(data_socket);
    if(transfer_socket == -1)
    {
        cerr<<"无法创建传输套接字"<<endl;
        return;
    }
    
    
      std::thread([&client, transfer_socket, data_socket, from_id, to_id, message, &redis_data]() {
        cout<<"进入发送文件线程"<<endl;
        string full_path = message;
        string filename;
        this_thread::sleep_for(chrono::milliseconds(150));
        // 提取文件名
        size_t t = full_path.find_last_of('/');
        if (t != string::npos) {
            filename = full_path.substr(t + 1);
        } else {
            filename = full_path;
        }
        string the_file = filename;
        filename = from_id + ":" + to_id + ":" + filename;
        cout<<"filename is"<<filename<<endl;

        // 打开文件
        ofstream file(filename, ios::binary);
        
        if (!file) {
            cerr << "上传失败，无法创建文件" << endl;
            string error_msg = "UPLOAD_ERROR";
            send(transfer_socket, error_msg.c_str(), error_msg.size(), 0);
            close(transfer_socket);
            return;
        }
  cout<<"开始发送文件"<<endl;
        // 接收文件内容
        char buffer[BUFFER_SIZE];
        int total_received = 0;
        bool transfer_complete = false;

        while (true) {
            int bytes_recv = recv(transfer_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_recv <= 0) {
                if (bytes_recv == 0) {
                    transfer_complete = true; // 正常关闭
                }
               //cout<<"我在发送"<<endl;
                break;
            }
            file.write(buffer, bytes_recv);
            total_received += bytes_recv;
        }
        file.close();

        // 处理传输结果
        if (transfer_complete) {
           
        if(redis_data.add_file_group(to_id,from_id,the_file)&&redis_data.add_file_to_unread_list(to_id,from_id,the_file))
       {
        cout << "文件接收成功: " << filename << endl;
       }
        string ack = "SUCCESS";
        send(transfer_socket, ack.c_str(), ack.size(), 0);

        } else {
            remove(filename.c_str()); // 传输不完整，删除文件
            cerr << "文件接收不完整，已删除" << endl;
            string err = "TRANSFER_ERROR";
            send(transfer_socket, err.c_str(), err.size(), 0);
        }

        // 关闭套接字
        close(transfer_socket);
    }).detach(); // 分离线程，自动释放资源
}
void GRO::accept_file_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data) {
    vector<string> result;
    string response;
   result =  redis_data.get_unread_file_group(to_id,from_id); // 获取文件列表
    
    if (result.empty()) {
        cout << "无文件" << endl;
        string response = "NO_FILES";
        send(data_socket, response.c_str(), response.size(), 0);
        return;
    }
    response += "有"+to_string(result.size())+"个文件\n";
    for(int i = 0;i < result.size();i++)
    {
        size_t split_pos = result[i].find('|');
        string send_fileid = result[i].substr(0, split_pos);      
        string filename = result[i].substr(split_pos + 1);   
        response +=to_string(i+1)+"."+ result[i] + "\n";
    }
    
    send(data_socket, response.c_str(), response.size(), 0);
    
    char choice_buf[16];
    int bytes = recv(data_socket, choice_buf, sizeof(choice_buf), 0);
    if(bytes <= 0) {
        cerr << "接收选择失败" << endl;
        return;
    }
    choice_buf[bytes] = '\0';
    int choice = stoi(choice_buf) - 1;

    if(choice < 0 || choice >= result.size()) {
        send(data_socket, "INVALID_CHOICE", 14, 0);
        return;
    }

    
    int transfer_socket = client.new_transfer_socket(data_socket);
    if(transfer_socket == -1) {
        cerr << "创建传输套接字失败" << endl;
        return;
    }

   
  

    thread([&client,transfer_socket,to_id,from_id,result,choice,&redis_data]()
    {
    size_t split_pos = result[choice].find('|');
    string send_fileid = result[choice].substr(0, split_pos);    
    string filename = result[choice].substr(split_pos + 1);       

    string local_filename = send_fileid + ":" + to_id + ":" + filename;
    
    ifstream file(local_filename, ios::binary);
    if(!file) {
        send(transfer_socket, "FILE_NOT_FOUND", 14, 0);
        cout<<"meizhaodao"<<endl;
        close(transfer_socket);
        return;
    }

    // 先发送文件名
    string base_name = filename;
    send(transfer_socket, base_name.c_str(), base_name.size(), 0);
    char ack_buf[4];
    int ack_bytes = recv(transfer_socket, ack_buf, sizeof(ack_buf), 0);
    if (ack_bytes <= 0 || string(ack_buf, ack_bytes) != "ACK") {
    cerr << "客户端未确认或确认失败" << endl;
    close(transfer_socket);
    return;
    }
    // 发送文件内容
    char buffer[BUFFER_SIZE];
    while(!file.eof()) {
        file.read(buffer, BUFFER_SIZE);
        int bytes_read = file.gcount();
        if(send(transfer_socket, buffer, bytes_read, 0) != bytes_read) {
            cerr << "文件发送中断" << endl;
            break;
        }
    }
    file.close();

   
    close(transfer_socket);
    this_thread::sleep_for(chrono::milliseconds(150));
    redis_data.remove_unread_file(to_id,from_id,result[choice]);
}).detach();

}
void GRO::delete_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data)
{   if(!redis_data.is_in_group(message,to_id))
    {
        string response = "该用户不在群聊内\n";
        client.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.is_group_owner(message,to_id))
    {
        string response = "不可以踢出群主\n";
        client.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.is_admin(from_id,message)&&redis_data.is_admin(to_id,message))
    {
        string response = "不可以踢出管理员\n";
       client.send_m(data_socket,"other", response);
        return;
    }
    
    if(redis_data.is_group_owner(message,from_id)||redis_data.is_admin(from_id,message))
    {
         if(redis_data.remove_group_member(message,to_id))
    {
         string response = "已成功将该用户踢出群聊";
         cout<<response<<endl;
        client.send_m(data_socket,"other", response);
        return;
    }
    }
    
}
//删除空格以便存入message消息库
string delete_space(const std::string& message) {
    std::string result = message;
    for (char& c : result) {
        if (c == ' ') {
            c = '_';
        }
    }
    return result;
}
//删除从redis取出来的字符串中的下划线
string delete_line(const std::string& message) {
    std::string result = message;
    for (char& c : result) {
        if (c == '_') {
            c = ' ';
        }
    }
    return result;
}
//获取时间戳
string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    return std::to_string(seconds); 
}

int main() {
    DATA redis_data;
    TCP server;
    server.start(redis_data);
    return 0;
}
