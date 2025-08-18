#include "fri.hpp"

void FRI::accept_file(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg) {
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
    //cout<<"要转化"<<choice_buf<<endl;
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
    string filename = result[choice];
    cout<<result[choice]<<endl;
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
       // cout<<"我在发送"<<endl;
        if(send(transfer_socket, buffer, bytes_read, 0) != bytes_read) {
            cerr << "文件发送中断" << endl;
            break;
        }
    }
    file.close();
    shutdown(transfer_socket, SHUT_WR);
     char ack[16] = {0};
        int h = recv(transfer_socket, ack, sizeof(ack) - 1, 0);
        if (h > 0) {
            ack[h] = '\0';
            cout << "\n"; // 换行避免覆盖进度条
            if (string(ack) == "SUCCESS") {
                cout << "\033[32m文件发送成功\033[0m" << endl;
            } else {
                cout << "\033[31m错误: " << ack << "\033[0m" << endl;
            }
        }
    close(transfer_socket);
    cout<<"发送成功"<<endl;
    redis_data.revise_file_status(to_id,from_id,result[choice]);
    cout<<"退出发送"<<endl;
}).detach();
}
void FRI::is_friends(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    string t = redis_data.get_username_by_id(to_id);
    if(t == "")
    {
        string recover = "用户不存在";
        cout<<recover<<endl;
        msg.send_m(data_socket,"other",recover);
        return;
    }
    if(!redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
        string a= "你与该用户还不是好友";
         msg.send_m(data_socket,"other",a);
        return;
    }
    if(redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
         string a= "你已经屏蔽该用户";
         msg.send_m(data_socket,"other",a);
        return;
    }
    string a= "成功";
    msg.send_m(data_socket,"other",a);
}
//给登陆的用户一直发送实时消息通知
void TCP::recived_messages(DATA &redis_data, string user_id, int data_socket,MSG &msg) {
    string a= redis_data.get_unred_messages(user_id,"|");
    if(a == ""||a.empty())
    {
        const char* b= "无未读消息";
        msg.send_m(data_socket,"other",b);
       // cout<<"已发送"<<endl;
        return;
    }
        msg.send_m(data_socket,"other",a);
        redis_data.delete_notice_messages(user_id);
}
void FRI::send_file(TCP &client, int data_socket, string from_id, string to_id, string message, DATA &redis_data,MSG &msg) {
    if(!redis_data.is_friend(from_id,to_id) &&!redis_data.is_friend(to_id,from_id))
    {
        string notice = RED_TEXT("你已被删除！");
    msg.send_m(data_socket,"other", notice);
    return;
    }
    if(redis_data.is_friend(from_id,to_id) &&!redis_data.is_friend(to_id,from_id))
   {
    string notice = RED_TEXT("你已被对方屏蔽!");
    msg.send_m(data_socket,"other", notice);
    return;
   }
    string notice = "friend";
    msg.send_m(data_socket,"other", notice);
    int transfer_socket = client.new_transfer_socket(data_socket);
    if (transfer_socket == -1) {
        cerr << "无法创建传输套接字" << endl;
        return;
    }
    cout<<"准备进入线程。。。"<<endl;
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
          time_t now = time(nullptr);
        tm* local_time = localtime(&now);
        char timebuf[32];
        strftime(timebuf, sizeof(timebuf), "_%Y%m%d_%H%M%S", local_time);
        string the_file = filename;
        filename = from_id + ":" + to_id + ":" + filename+timebuf;
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
            
            bool redis_ok = false;
            {
                redis_ok = redis_data.add_file(from_id, to_id, filename);
            }

            if (redis_ok) {
                cout << "文件接收成功: " << filename << endl;
                string ack = "SUCCESS";
                string notice =YELLOW_TEXT("有来自"+from_id+"的新文件") ;
                client.send_notice(from_id,to_id,notice,redis_data);
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
void FRI :: new_message(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data,MSG &msg)
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
         msg.send_m(data_socket,"other",recover);
        cout<<a<<endl;
        return;
    }
    if(!redis_data.is_friend(to_id,from_id))
    {
        string a= "对方已把你拉黑";
        msg.send_m(data_socket,"other",recover);
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
        msg.send_m(data_socket,"other",recover);
    
     }
     if(messages.empty())
     {
        //cout<<"没有新消息"<<endl;
        // continue;
        recover = "没有新消息";
        msg.send_m(data_socket,"other",recover);
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
    //msg.send_m()
    msg.send_m(data_socket,"other",recover);
}
void FRI::cancel_shield_friend(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data,MSG &msg)
{
    if(from_id == to_id)
    {
         string a= RED_TEXT("你不能解除屏蔽自己");
         msg.send_m(data_socket,"other",a);
        return;
    }
     if(!redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
        string a= RED_TEXT("你与该用户不是好友");
        msg.send_m(data_socket,"other",a);
        return;
    }
     if(redis_data.is_friend(to_id,from_id) &&redis_data.is_friend(from_id,to_id))
    {
        string a= RED_TEXT("你未屏蔽该用户");
        msg.send_m(data_socket,"other",a);
        return;
    }
    redis_data.add_friends(to_id,from_id);
    string notice = GREEN_TEXT("用户" + from_id + "已解除对你的屏蔽!");
    client.send_notice(from_id,to_id,notice,redis_data);
    string recover = GREEN_TEXT("已成功解除屏蔽好友");
    //cout<<recover<<endl;
     msg.send_m(data_socket,"other",recover);
}
//屏蔽好友
void FRI::shield_friend(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data,MSG &msg)
{
    if(from_id == to_id)
    {
         string a= RED_TEXT("你不能屏蔽自己");
         msg.send_m(data_socket,"other",a);
        return;
    }
    if(!redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
        string a= RED_TEXT("你与该用户还不是好友");
         msg.send_m(data_socket,"other",a);
        return;
    }
    if(redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
         string a= RED_TEXT("你已经屏蔽该用户");
         msg.send_m(data_socket,"other",a);
        return;
    }
    
    redis_data.rdelete_friend(to_id,from_id);
    string notice = RED_TEXT("用户" + from_id + "已将你屏蔽!");
    client.send_notice(from_id,to_id,notice,redis_data);
    string recover = GREEN_TEXT("已成功屏蔽好友");
    //cout<<recover<<endl;
   msg.send_m(data_socket,"other",recover);
}
void FRI::store_chat_Pair(string first, string second) {
     cout<<"尝试持有锁"<<endl;
    std::lock_guard<std::mutex> lock(g_chat_pairs_mutex);
    chat_pairs[first] = second;
    std::cout << "已存储: \"" << first << "\" -> \"" << second << "\"" << std::endl;
     cout<<"已经释放锁"<<endl;
}
void FRI::printChatPairsTable() {
    if(group_pairs.empty()) {
        cout << "聊天关系表为空" << endl;
        return;
    }

    cout << "\n========== 聊天关系表 ==========" << endl;
    cout << "| " << setw(15) << left << "用户A" << " | " << setw(15) << "用户B" << " |" << endl;
    cout << "|-----------------|-----------------|" << endl;
    
    // 使用set避免重复打印双向关系
    set<pair<string, string>> printed;
    for(const auto& pair : chat_pairs) {
        if(printed.find({pair.second, pair.first}) == printed.end()) {
            cout << "| " << setw(15) << left << pair.first 
                 << " | " << setw(15) << pair.second << " |" << endl;
            printed.insert({pair.first, pair.second});
        }
    }
    cout << "===================================" << endl;
}
// 检查a对应的b是否也对应a
bool FRI:: check_chat(string a,string b) {
   //  printChatPairsTable();
   cout<<"获取锁"<<endl;
    std::lock_guard<std::mutex> lock(g_chat_pairs_mutex);
    auto it_a = chat_pairs.find(a);
    if (it_a == chat_pairs.end() || it_a->second != b) {
    
      cout<<"释放锁"<<endl;
        return false;
    }
    
    auto it_b = chat_pairs.find(b);
    if (it_b == chat_pairs.end() || it_b->second != a) {
       cout<<"释放锁"<<endl;
        return false;
    }
    
   cout << "确认对应: \"" << a << "\" ↔ \"" << b << "\"" << std::endl;
    cout<<"释放锁"<<endl;
   return true;
}
bool FRI::delete_chat_pair(string first) {
    // cout<<"尝试持有锁"<<endl; 
    // std::lock_guard<std::mutex> lock(pairs_mutex);
    auto it = chat_pairs.find(first);
    if (it != chat_pairs.end()) {
        chat_pairs.erase(it);
        std::cout << "已删除键为 \"" << first << "\" 的键值对" << std::endl;
    //    cout<<"已经释放锁"<<endl;
        return true;
    }
    
    std::cout << "未找到键为 \"" << first << "\" 的键值对" << std::endl;
    return false;
}
//打开聊天框，发送历史记录

void FRI::open_block(TCP &client, int data_socket, string from_id, string to_id, DATA &redis_data,MSG &msg) {
    store_chat_Pair(from_id, to_id);
    vector<string> all_messages;
    
    string username1 = redis_data.get_username_by_id(from_id);
    string username2 = redis_data.get_username_by_id(to_id);
    
    if(username2 == "") {
        string a = "用户不存在";
        msg.send_m(data_socket, "other", a);
        return;
    }
    
    if(!redis_data.is_friend(to_id, from_id) && !redis_data.is_friend(from_id, to_id)) {
        string a = "你与该用户还不是好友";
        msg.send_m(data_socket, "other", a);
        return;
    }
    
    // 获取双方的消息
    redis_data.see_all_other_message(to_id, from_id, all_messages); // 对方发给我的
    if(from_id != to_id) {
        redis_data.see_all_my_message(from_id, to_id, all_messages); // 我发给对方的
    }
    
    if(all_messages.empty()) {
        string recover = "暂无聊天记录";
        msg.send_m(data_socket, "other", recover);
        return;
    }
    
    // 按时间戳排序
   std::stable_sort(all_messages.begin(), all_messages.end(), 
    [](const string& a, const string& b) {
        size_t pipe_pos_a = a.rfind('|');
        size_t pipe_pos_b = b.rfind('|');
        return std::stoll(a.substr(pipe_pos_a + 1)) < 
               std::stoll(b.substr(pipe_pos_b + 1));
    });
    
    // 只保留最新的50条
    if(all_messages.size() > 50) {
        all_messages.erase(all_messages.begin(), all_messages.end() - 50);
    }
    
    // 格式化消息
     string recover;
    for (auto& msg_str : all_messages) {
        // 解析 from_id:to_id:content|timestamp
        size_t first_colon = msg_str.find(':');
        size_t second_colon = msg_str.find(':', first_colon + 1);
        size_t pipe_pos = msg_str.find('|', second_colon + 1);
        
        string sender_id = msg_str.substr(0, first_colon);
        string receiver_id = msg_str.substr(first_colon + 1, second_colon - first_colon - 1);
        string content = msg_str.substr(second_colon + 1, pipe_pos - second_colon - 1);
        string timestamp = msg_str.substr(pipe_pos + 1);
        
        string sender_name = (sender_id == from_id) ? username1 : username2;
        
        // 根据发送者决定显示格式
        if (sender_id == from_id) {
            recover +=  content + "[" + sender_name + "]\n";
        } else {
            recover += "\033[0;36m[" + sender_name + "]:\033[0m" + content + "\n";
        }
    }
    
    msg.send_m(data_socket, "other", recover);
   
}
//选择添加好友操作
void FRI::send_add_request(TCP &client,int data_socket,string to_id,string from_id,string message,DATA &redis_data,MSG &msg){
    int bytes;
    char buffer[1024];
    string t = redis_data.get_username_by_id(to_id);
    if(t == "")
    {
        string recover = RED_TEXT( "要添加的用户不存在");
        cout<<recover<<endl;
        msg.send_m(data_socket,"other",recover);
        return;
    }
    if(redis_data.is_friend(to_id,from_id) &&redis_data.is_friend(from_id,to_id))
    {
        string a = RED_TEXT("你与该用户已经是好友了");
        msg.send_m(data_socket,"other",a);
        return;
    }
    if(redis_data.check_add_friend_request_duplicata(to_id,from_id))
    {
        string recover = RED_TEXT("已向该用户发送过好友申请，请不要重复发送");
        msg.send_m(data_socket,"other",recover);
        return;
    }
    string status = "unread";
    if(!redis_data.add_friends_request(from_id,to_id,message,status))
    {
        string recover = "将好友申请添加到好友申请表中失败";
        cout<<recover<<endl;
        msg.send_m(data_socket,"other",recover);
        return;
    }

    string recover =  GREEN_TEXT("已成功发送好友申请");
    string notice =YELLOW_TEXT("收到来自用户"+from_id+"的好友申请") ;
    client.send_notice(from_id,to_id,notice,redis_data);
    cout<<recover<<endl;
    msg.send_m(data_socket,"other",recover);
    
    }
void FRI:: delete_friend(TCP &client,int data_socket,string to_id,string from_id,DATA &redis_data,MSG &msg){

    if(from_id == to_id)
    {
        {
        string a= RED_TEXT("不能删除自己！");
        msg.send_m(data_socket,"other",a);
        return;
    }
    }
    if(!redis_data.is_friend(to_id,from_id) ||!redis_data.is_friend(from_id,to_id))
    {
        string a= RED_TEXT("你与该用户还不是好友");
        msg.send_m(data_socket,"other",a);
        return;
    }
    redis_data.rdelete_friend(to_id,from_id);
    redis_data.rdelete_friend(from_id,to_id);
    string recover = GREEN_TEXT("已成功删除好友");
    //cout<<recover<<endl;
   msg.send_m(data_socket,"other",recover);
}
//选择查看好友申请操作
void FRI:: check_add_friends_request(TCP &client,int data_socket,string from_id,DATA &redis_data,int &m,MSG &msg){
    string serialized_data;
    serialized_data=redis_data.check_add_request(from_id);
    if(serialized_data.empty() || serialized_data == "[]" || serialized_data == "null")
    {
        cout<<"该用户无好友申请"<<endl;
        serialized_data = "无好友申请";
        msg.send_m(data_socket,"other",serialized_data);
        m = 1;
        return;
    }
    cout<<serialized_data<<endl;
     msg.send_m(data_socket,"other",serialized_data);
    cout<<"发送该用户的好友申请给客户端"<<endl;

 }
void FRI::add_friend(TCP &client,int data_socket,string to_id,string from_id,DATA &redis_data,MSG &msg){
    string status ="accept";
    if(redis_data.check_dealed_request(to_id,from_id,status))
    {
        
        string d=RED_TEXT("已同意过该好友申请，请勿重复处理");
        msg.send_m(data_socket,"other",d);
        return; 
    }
    string other_status = "refuse";
    if(redis_data.check_dealed_request(to_id,from_id,other_status))
    {
        string d=RED_TEXT("已拒绝过该好友申请，请勿重复处理");
        msg.send_m(data_socket,"other",d);
        return;  
    }
    //将to_id添加到from_id的好友列表
    
    //status ="accept";
    if(redis_data.revise_status(to_id,from_id,status))
    {
    redis_data.remove_friends_request(to_id,from_id);
    redis_data.add_friends(to_id,from_id);
    redis_data.add_friends(from_id,to_id);
    string d=GREEN_TEXT("添加好友成功！");
    cout<<d<<endl;
    string notice =YELLOW_TEXT(from_id+"已同意你的好友申请") ;
    client.send_notice(from_id,to_id,notice,redis_data);
    msg.send_m(data_socket,"other",d);
    return;
    }
    string d = "添加好友失败！";
    msg.send_m(data_socket,"other",d);
}
//选择查看好友列表操作
void FRI::see_all_friends(TCP &client, int data_socket, string from_id, DATA &redis_data,MSG &msg) { 
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
    msg.send_m(data_socket, "other", friend_list);
}
//拒绝好友申请
void FRI::refuse_friend_request(TCP &client,int data_socket,string to_id,string from_id,DATA &redis_data,MSG &msg){
    string status ="accept";
    if(redis_data.check_dealed_request(to_id,from_id,status))
    {
        string d=RED_TEXT("已同意过该好友申请，请勿重复处理");
        msg.send_m(data_socket,"other",d);
        return;
    }
    string other_status = "refuse";
    if(redis_data.check_dealed_request(to_id,from_id,other_status))
    {
        string d=RED_TEXT("已拒绝过该好友申请，请勿重复处理");
        msg.send_m(data_socket,"other",d);
        return; 
    }
    //string status ="refuse";
    if(redis_data.revise_status(to_id,from_id,other_status))
    {
        redis_data.remove_friends_request(to_id, from_id);
        const char* d = "已拒绝该好友申请";
        string notice =YELLOW_TEXT(from_id+"已拒绝你的好友申请") ;
        client.send_notice(from_id,to_id,notice,redis_data);
        msg.send_m(data_socket,"other",d);
        return;
    }
    string d = "拒绝好友申请失败！";
   msg.send_m(data_socket,"other",d);
}
//向好友发送消息
void FRI:: send_message(TCP &client,int data_socket,string from_id,string to_id,string message,DATA &redis_data,MSG &msg){
    if(!redis_data.is_friend(from_id,to_id) &&!redis_data.is_friend(to_id,from_id))
    {
        string notice = RED_TEXT("你已被删除！");
    msg.send_m(data_socket,"other", notice);
    return;
    }
    if(redis_data.is_friend(from_id,to_id) &&!redis_data.is_friend(to_id,from_id))
   {
    string notice = RED_TEXT("你已被对方屏蔽!");
    msg.send_m(data_socket,"other", notice);
    return;
   }
    //  int b_socket =client.find_socket(to_id);
    //     if(b_socket == -1)
    //     {
    //         ;
    //     }else{}
    if(check_chat(from_id,to_id))
    {
        int b_socket =client.find_socket(to_id);
        if(b_socket != -1)
        {
        cout<<"找到对方的socket"<<b_socket<<endl;
        string notice = "对方正在聊天框内和你聊天";
        cout<<notice<<endl;
        string type = redis_data.get_username_by_id(from_id);
       // send(b_socket,message.c_str(),message.size(),0);
        msg.send_m(b_socket,type, message);
       
        redis_data.add_message_log(from_id,to_id,message);
        return;
        }
    }
        
  if(redis_data.add_message_log_unread(from_id,to_id,message))
           {
            string notice =YELLOW_TEXT("有一条来自"+from_id+"的新消息") ;
            client.send_notice(from_id,to_id,notice,redis_data);
           }else{
            string a= "消息发送失败";
           }
}
void FRI:: quit_chat(TCP &client,int data_socket,string from_id,string to_id,DATA &redis_data,MSG &msg)
{
    string message = "quit"; 
    msg.send_m(data_socket,"other", message);
    delete_chat_pair(from_id);
}
