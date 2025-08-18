#include "redis.hpp"
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
thread_local redisContext* DATA::c = nullptr;
//连接redis
redisContext* DATA:: data_create() {
        if (!DATA::c) {
            struct timeval timeout = {1, 500000};
            DATA::c = redisConnectWithTimeout("127.0.0.1", 6379,timeout);
            
            if (!DATA::c || c->err) {
                std::cerr << "Redis连接错误: " 
                         << (c ? c->errstr : "无法分配连接上下文") 
                         << std::endl;
                if (DATA::c) redisFree(DATA::c);
                DATA::c = nullptr;
            }
        }
        return DATA::c;
    }

//检查用户名是否重复
bool DATA::check_username_duplicate(string username)
 {
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return "";
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
   redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
bool DATA::remove_friends_request(string from_id, string to_id) {
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
    
    string key = "request:" + to_id;
    
    // 获取整个列表
    redisReply *reply = (redisReply*) redisCommand(c, "LRANGE %s 0 -1", key.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_ARRAY) {
        cout << "读取列表失败" << endl;
        return false;
    }

    bool found = false;
    string item_to_remove;
    
    // 查找匹配的项（只需要匹配from_id）
    for (size_t i = 0; i < reply->elements; ++i) {
        string item = reply->element[i]->str;
        size_t pos = item.find("|");
        if (pos != string::npos) {
            string current_from_id = item.substr(0, pos);
            if (current_from_id == from_id) {
                found = true;
                item_to_remove = item;
                break;
            }
        }
    }

    freeReplyObject(reply);
    
    if (!found) {
        cout << "未找到匹配项" << endl;
        return false; 
    }

    // 执行 LREM 命令，删除匹配的元素
    redisReply *deleteReply = (redisReply*) redisCommand(c, "LREM %s 0 %s", key.c_str(), item_to_remove.c_str());

    if (deleteReply == nullptr) {
        cout << "Redis 删除命令执行失败" << endl;
        return false;
    }

    bool success = false;
    if (deleteReply->type == REDIS_REPLY_INTEGER) {
        if (deleteReply->integer > 0) {
            success = true;
        } else {
            cout << "删除失败，未找到匹配项。" << endl;
        }
    } else {
        cout << "返回的结果类型不正确，预期为整数类型。" << endl;
    }

    freeReplyObject(deleteReply);
    return success;
}
//发送在线好友请求
string DATA::check_add_request_and_revise(string to_id) {
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return "";
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return "";
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return;
    redisCommand(c, "SADD user:%s:friends %s", from_id.c_str(), to_id.c_str());
}
//删除某人的全部好友列表
bool DATA::delete_friends(string user_id) {
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
    // redisReply* selectReply = (redisReply*)redisCommand(c, "SELECT 0");
    // freeReplyObject(selectReply);
    redisReply* reply = (redisReply*)redisCommand(c, "SISMEMBER user:%s:friends %s", to_id.c_str(), from_id.c_str());

    if (reply == nullptr) {
       cout<<"fromid is 111"<< from_id<<"to_id is 222"<<to_id<<endl;
        return false;
    }
     if (reply->type == REDIS_REPLY_ERROR) {  // 显式检查错误
        cout << "Redis error: " << reply->str << endl;
        freeReplyObject(reply);
        return false;
    }
    bool isFriend = (reply->integer > 0);
    if(!isFriend )
    {
         cout<<"fromid is 1"<<from_id<<"to_id is 2"<<to_id<<endl;
         cout<<reply->integer<<endl;
    }
     freeReplyObject(reply);
    return isFriend;
}


void DATA::rdelete_friend(string to_id,string from_id){    
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return;
    redisCommand(c, "SREM user:%s:friends %s", from_id.c_str(), to_id.c_str());
   // redisCommand(c, "SREM user:%s:friends %s", to_id.c_str(), from_id.c_str());
    }
void DATA::see_all_friends(string from_id,vector<string>&buffer){
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return ;
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
   redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
                     //   remove_friends_request(from_id,to_id,stored_message, stored_status);
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
bool DATA::add_message_log(string from_id, string to_id, string m) {
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) {
        cerr << "无法连接Redis" << endl;
        return false;
    }

    // 清理消息内容并添加状态和时间戳
    string message = delete_space(m);
    string timestamp = getCurrentTimestamp();
    message += "|send|" + timestamp;

    // 构造Redis键名（不需要加引号）
    const string list_key = "messages:" + from_id + ":" + to_id;
    
    // 使用二进制安全接口
    const char* argv[3] = {
        "RPUSH",          // Redis命令
        list_key.c_str(), // 键名
        message.c_str()   // 消息内容
    };
    
    // 各参数的长度
    size_t argvlen[3] = {
        5,                      // "RPUSH"的长度
        list_key.size(),         // 键名长度
        message.size()           // 消息长度
    };

    // 执行命令
    cout << "执行命令: RPUSH " << list_key << " " << message << endl;
    redisReply* reply = (redisReply*)redisCommandArgv(c, 3, argv, argvlen);

    // 处理结果
    bool success = false;
    if (reply) {
        if (reply->type == REDIS_REPLY_INTEGER) {
            cout << "消息添加成功，当前列表长度: " << reply->integer << endl;
            success = true;
        } else if (reply->type == REDIS_REPLY_ERROR) {
            cerr << "Redis错误: " << reply->str << endl;
        }
        freeReplyObject(reply);
    } else {
        cerr << "Redis命令执行失败" << endl;
    }

    return success;
}
bool DATA::add_message_log_unread(string from_id,string to_id,string m){
  redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
   // string message = delete_space(m);
    string timestamp = getCurrentTimestamp();
   // message += "|unread|" + timestamp;
    string key = "messages:" + from_id + ":" + to_id;
    string message = delete_space(m) + "|unread|" + getCurrentTimestamp();

    const char* argv[3] = {
        "RPUSH",
        key.c_str(),
        message.c_str()
    };
    size_t argvlen[3] = {
        5,  // "RPUSH" 的长度
        key.size(),
        message.size()
    };

    redisReply* reply = (redisReply*)redisCommandArgv(c, 3, argv, argvlen);
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
bool DATA::see_all_other_message(string from_id, string to_id, vector<string>& messages) {
    redisContext* c = data_create();
    if (!c) return false;
    string list_key = "messages:" + from_id + ":" + to_id;
    
    redisReply* reply = (redisReply*)redisCommand(c, "LRANGE %s %d %d", 
        list_key.c_str(), -100, -1);
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }

    for (size_t i = 0; i < reply->elements; i++) {
        string message = reply->element[i]->str;
        size_t pipe_pos1 = message.find('|');
        size_t pipe_pos2 = message.find('|', pipe_pos1 + 1);
        
        if (pipe_pos1 != string::npos && pipe_pos2 != string::npos) {
            string content = message.substr(0, pipe_pos1);
            string status = message.substr(pipe_pos1 + 1, pipe_pos2 - pipe_pos1 - 1);
            string timestamp = message.substr(pipe_pos2 + 1);
            
            // 格式化为 from_id:to_id:content|timestamp
            string formatted_message = from_id + ":" + to_id + ":" + delete_space(content) + "|" + timestamp;
            messages.push_back(formatted_message);
        }
    }

    freeReplyObject(reply);
    return true;
}

bool DATA::see_all_my_message(string from_id, string to_id, vector<string>& messages) {
    redisContext* c = data_create();
    if (!c) return false;
    string list_key = "messages:" + from_id + ":" + to_id;
    
    redisReply* reply = (redisReply*)redisCommand(c, "LRANGE %s %d %d", 
        list_key.c_str(), -100, -1);
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }

    for (size_t i = 0; i < reply->elements; i++) {
        string message = reply->element[i]->str;
        size_t pipe_pos1 = message.find('|');
        size_t pipe_pos2 = message.find('|', pipe_pos1 + 1);
        
        if (pipe_pos1 != string::npos && pipe_pos2 != string::npos) {
            string content = message.substr(0, pipe_pos1);
            string status = message.substr(pipe_pos1 + 1, pipe_pos2 - pipe_pos1 - 1);
            string timestamp = message.substr(pipe_pos2 + 1);
            
            if (status == "unread") {
                string new_message = content + "|read|" + timestamp;
                redisCommand(c, "LSET %s %d %s", list_key.c_str(), i, new_message.c_str());
            }
            
            // 格式化为 from_id:to_id:content|timestamp
            string formatted_message = from_id + ":" + to_id + ":" + delete_space(content) + "|" + timestamp;
            messages.push_back(formatted_message);
        }
    }

    freeReplyObject(reply);
    return true;
}
//获取未读的message和其fromid
bool DATA::get_messages( const string& toid,
                        vector<string>& fromids,
                        vector<string>& messages) {
      redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
       redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
     
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;   
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
bool DATA::revise_file_status(string from_id, string to_id, string filename) {
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
    std::string key = "file:" + from_id + ":" + to_id;

    // 1. 获取整个列表
    redisReply* lrange_reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", key.c_str());
    if (!lrange_reply || lrange_reply->type != REDIS_REPLY_ARRAY) {
        if (lrange_reply) freeReplyObject(lrange_reply);
        return false;
    }

    // 2. 遍历查找匹配的记录
    bool found = false;
    for (size_t i = 0; i < lrange_reply->elements; i++) {
        if (!lrange_reply->element[i] || lrange_reply->element[i]->type != REDIS_REPLY_STRING) {
            continue;
        }

        std::string entry = lrange_reply->element[i]->str;
        size_t pipe_pos = entry.find('|');
        
        // 检查格式是否正确（至少有一个|分隔符）
        if (pipe_pos == std::string::npos) {
            continue;
        }

        std::string current_filename = entry.substr(0, pipe_pos);
        std::string status = entry.substr(pipe_pos + 1);

        // 3. 匹配文件名且状态为unread
        if (current_filename == filename && status != "read") {
            // 4. 更新状态为read
            std::string new_entry = current_filename + "|read";
            redisReply* lset_reply = (redisReply*)redisCommand(
                c, "LSET %s %d %s", key.c_str(), (int)i, new_entry.c_str()
            );
            
            found = (lset_reply != nullptr && lset_reply->type == REDIS_REPLY_STATUS);
            if (found) {
                cout << "已将文件 [" << filename << "] 状态更新为read" << endl;
            }
            
            if (lset_reply) freeReplyObject(lset_reply);
            break;
        }
    }

    freeReplyObject(lrange_reply);
    return found;
}
//寻找是否有未读文件并返回
bool DATA::get_unread_files(string to_id, vector<pair<string, string>>& result) {
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return vector<string>();
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return "";
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return vector<string>();
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
bool DATA::check_group_apply(string group_id, string to_id) {
    // 获取加群申请列表
    redisReply* reply = (redisReply*)redisCommand(
        c,
        "LRANGE group:apply:%s 0 -1", 
        group_id.c_str()
    );

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "获取加群申请失败: " << (reply ? reply->str : "无响应") << endl;
        if (reply) freeReplyObject(reply);
        return false;
    }

    // 遍历每一条申请记录
    bool found = false;
    for (int i = 0; i < reply->elements; ++i) {
        string apply_value = reply->element[i]->str;
        
        // 判断申请记录是否包含 `to_id`
        size_t pos = apply_value.find(":");
        if (pos != string::npos) {
            string applicant_id = apply_value.substr(0, pos);
            if (applicant_id == to_id) {
                found = true;
                break;
            }
        }
    }

    freeReplyObject(reply);
    return found;
}

bool DATA::remove_group_application(string applicant_id,string group_id) {
    redisContext* c = data_create();
    if (!c) return false;

    // 1. 获取所有申请记录
    redisReply* reply = (redisReply*)redisCommand(
        c, "LRANGE group:apply:%s 0 -1", group_id.c_str());
    
    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        if (reply) freeReplyObject(reply);
        return false;
    }

    // 2. 查找匹配记录并删除
    bool removed = false;
    for (size_t i = 0; i < reply->elements; ++i) {
        string record = reply->element[i]->str;
        size_t colon_pos = record.find(':');
        
        // 检查申请人ID是否匹配
        if (colon_pos != string::npos && 
            record.substr(0, colon_pos) == applicant_id) {
            
            // 删除这条记录
            redisReply* del_reply = (redisReply*)redisCommand(
                c, "LREM group:apply:%s 0 %s", 
                group_id.c_str(), record.c_str());
                
            if (del_reply && del_reply->integer > 0) {
                removed = true;
            }
            if (del_reply) freeReplyObject(del_reply);
        }
    }
    freeReplyObject(reply);

    return removed;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
    // 值格式：from_id|message
    string value = from_id + "|" + message;
    
    // 1. 开始事务
    redisReply* reply = (redisReply*)redisCommand(c, "MULTI");
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "开启事务失败" << endl;
        if (reply) freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);

    // 2. 添加消息命令
    reply = (redisReply*)redisCommand(
        c,
        "ZADD chat:%s %s %s",
        group_id.c_str(),
        timestamp.c_str(),
        value.c_str()
    );
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "添加消息命令失败" << endl;
        if (reply) freeReplyObject(reply);
        redisCommand(c, "DISCARD"); // 丢弃事务
        return false;
    }
    freeReplyObject(reply);

    // 3. 修剪消息命令
    reply = (redisReply*)redisCommand(
        c,
        "ZREMRANGEBYRANK chat:%s 0 -101",
        group_id.c_str()
    );
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "修剪消息命令失败" << endl;
        if (reply) freeReplyObject(reply);
        redisCommand(c, "DISCARD"); // 丢弃事务
        return false;
    }
    freeReplyObject(reply);

    // 4. 执行事务
    reply = (redisReply*)redisCommand(c, "EXEC");
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        cerr << "执行事务失败: " << (reply ? reply->str : "无响应") << endl;
        if (reply) freeReplyObject(reply);
        return false;
    }

    // 5. 检查执行结果
    if (reply->type != REDIS_REPLY_ARRAY || reply->elements != 2) {
        cerr << "事务执行结果异常" << endl;
        freeReplyObject(reply);
        return false;
    }

    freeReplyObject(reply);
    return true;
}
//设置某用户在某群的最后已读时间
bool DATA::set_last_read_time(string user_id,string group_id,string timestamp) {
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return vector<pair<string, string>>();
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
vector<tuple<string, string, string>> DATA::get_read_messages(
    string user_id, 
    string group_id, 
    int count = 50
    ) {
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return vector<tuple<string, string, string>> ();
    // 1. 获取用户最后阅读时间
    string last_read = "0";
    redisReply* reply = (redisReply*)redisCommand(
        c, "GET user:last_read:%s:%s", 
        user_id.c_str(), group_id.c_str()
    );
    if (reply && reply->type == REDIS_REPLY_STRING) {
        last_read = reply->str;
    }
    freeReplyObject(reply);

    // 2. 获取最新的50条消息（按时间倒序）
    reply = (redisReply*)redisCommand(
        c, "ZREVRANGE chat:%s 0 %d WITHSCORES",
        group_id.c_str(), count - 1
    );

    vector<tuple<string, string, string>> messages;  // 改为三元组
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i += 2) {
            if (i+1 >= reply->elements) break;

            string full_message = reply->element[i]->str;
            string timestamp = reply->element[i+1]->str;

            // 解析 from_id 和 content
            size_t sep = full_message.find('|');
            if (sep == string::npos) {
                cerr << "消息格式错误: " << full_message << endl;
                continue;
            }

            string from_id = full_message.substr(0, sep);
            string content = full_message.substr(sep + 1);
            
            messages.emplace_back(from_id, content, timestamp);  // 只保存三个字段
        }
    }

    if (reply) freeReplyObject(reply);
    return messages;
}
// 获取用户未读消息
vector<pair<string, string>> DATA::get_unread_messages(string user_id, string group_id) {
   redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return vector<pair<string, string>>();
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
  redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return vector<string>();
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
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
bool DATA::add_unread_message(string user_id,string message) {
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
    string key = "unread_notice:" + user_id;

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(c, "RPUSH %s %s", key.c_str(), message.c_str())
    );

    bool success = (reply != nullptr) && 
                  (reply->type == REDIS_REPLY_INTEGER) && 
                  (reply->integer >= 1);  

    if (!success) {
        std::cerr << "[REDIS ERROR] 消息插入失败: ";
        if (reply && reply->type == REDIS_REPLY_ERROR) {
            std::cerr << reply->str;  // 打印Redis错误信息
        } else {
            std::cerr << "未知错误";
        }
        std::cerr << std::endl;
    }

    if (reply) freeReplyObject(reply);
    return success;
}

bool DATA::delete_notice_messages(string user_id) {
     redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return false;
    std::string key = "unread_notice:" + user_id;

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(c, "DEL %s", key.c_str())
    );

    bool success = (reply != nullptr) && 
                  (reply->type == REDIS_REPLY_INTEGER) && 
                  (reply->integer == 1);

    if (!success) {
        std::cerr << "[REDIS ERROR] 列表删除失败: ";
        if (reply && reply->type == REDIS_REPLY_ERROR) {
            std::cerr << reply->str;
        } else if (reply && reply->integer == 0) {
            std::cerr << "键不存在";
        } else {
            std::cerr << "未知错误";
        }
        std::cerr << std::endl;
    }

    if (reply) freeReplyObject(reply);
    return success;
}
string DATA::get_unred_messages(string user_id, const std::string& delimiter = "\n") {
    redisContext* c = data_create();  // 自动获取线程专用连接
    if (!c) return "";
    std::string key = "unread_notice:" + user_id;
    std::string result;

    redisReply* len_reply = static_cast<redisReply*>(
        redisCommand(c, "LLEN %s", key.c_str())
    );

    if (!len_reply || len_reply->type != REDIS_REPLY_INTEGER) {
        std::cerr << "[REDIS ERROR] 获取列表长度失败: ";
        if (len_reply && len_reply->type == REDIS_REPLY_ERROR) {
            std::cerr << len_reply->str;
        } else {
            std::cerr << "连接错误";
        }
        std::cerr << std::endl;
        if (len_reply) freeReplyObject(len_reply);
        return "";
    }

    long list_len = len_reply->integer;
    freeReplyObject(len_reply);

    if (list_len == 0) {
        return "";
    }

    redisReply* range_reply = static_cast<redisReply*>(
        redisCommand(c, "LRANGE %s 0 -1", key.c_str())
    );

    if (!range_reply || range_reply->type != REDIS_REPLY_ARRAY) {
        std::cerr << "[REDIS ERROR] 获取消息列表失败" << std::endl;
        if (range_reply) freeReplyObject(range_reply);
        return "";
    }

    // 4. 拼接所有消息
    for (size_t i = 0; i < range_reply->elements; ++i) {
        if (range_reply->element[i]->type == REDIS_REPLY_STRING) {
            if (!result.empty()) {
                result += delimiter;
            }
            result += range_reply->element[i]->str;
        }
    }


    return result;
}
