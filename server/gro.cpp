#include "gro.hpp"
int GRO::generateNumber() {
    // 生成随机数的种子
    std::srand(std::time(0));
    
    // 生成一个五位数
    int randomNumber = std::rand() % 90000 + 10000;
    
    return randomNumber;
}
void GRO::generate_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg) {
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
       msg.send_m(data_socket,"other",response);
        return;
    }
    if (!redis_data.generate_group(from_id, message, group_id)) {
        cerr << "群组初始化失败" << endl;
        msg.send_m(data_socket,"other",response);
        return;
    }

    bool all_success = true;
    for (auto& member_id : ids) {
        if (!redis_data.add_group_member(group_id, member_id)) {
            cerr << "添加成员 " << member_id << " 失败" << endl;
            all_success = false;
        }
        string notice =YELLOW_TEXT("你已被拉入群聊"+group_id) ;
        client.send_notice(from_id,member_id,notice,redis_data);
    }
    if (all_success) {
        group_id = GREEN_TEXT(group_id);
        response += "创建群聊成功，群号: " + group_id;
        msg.send_m(data_socket,"other",response);
    } else {
       msg.send_m(data_socket,"other",response);;
    }
}
void GRO::delete_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    if(!redis_data.is_group_owner(message,from_id))
    {
        string response = "只有群主可以解散群聊";
       msg.send_m(data_socket,"other",response);
       return;
    }
                            
    if(redis_data.delete_group(message)&&redis_data.clear_group_files(message)&&redis_data.clear_group_request(message)&&redis_data.clear_group_messages(message))
    {
        string response = "删除群聊成功";
        msg.send_m(data_socket,"other",response);
    }//删除群聊基本信息
    
}
void GRO::see_all_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    vector<string> groups;
    string recover = "group_id      group_name";
    groups = redis_data.get_groups_by_user(from_id);
    if(groups.size() == 0)
    {
        string response = "用户暂无加入的群";
        msg.send_m(data_socket,"other",response);
        return;
    }
    for(int i = 0;i < groups.size();i++)
    {
        recover += "\n"+groups[i]+"      "+redis_data.get_group_name(groups[i]);
    }
    msg.send_m(data_socket,"other",recover);
}
void GRO::see_all_member(TCP &client, int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    vector<string> members; 
    members = redis_data.get_all_members(message);
    
    if(members.empty())  
    {
        msg.send_m(data_socket, "other", "该群组没有成员");
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
    
    msg.send_m(data_socket, "other", oss.str());
}
void GRO::add_admin(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
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
        msg.send_m(data_socket,"other", response);
        return;
    }
    
    if(redis_data.is_admin(to_id,group_id))
    {
    response = "该用户已经是管理员了";
     msg.send_m(data_socket,"other", response);
    return;
    }
    if(redis_data.get_admin_count(group_id) > 5)
    {
    response = "管理员上限为5人";
     msg.send_m(data_socket,"other", response);
    return;
    }
    if(redis_data.add_admins(to_id,group_id))
    {
    response = "添加该用户为管理员成功";
     string notice =YELLOW_TEXT("你已被设置为管理员"+group_id) ;
        client.send_notice(from_id,to_id,notice,redis_data);
     msg.send_m(data_socket,"other", response);
    }
    
 
}
void GRO::open_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    if(!redis_data.is_group_exists(message))
    {
        string response = "该群不存在";
        msg.send_m(data_socket,"other", response);
       return;
    }
    if(!redis_data.is_in_group(message,from_id))
    {
        string response = "你不在该群聊内";
        msg.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.is_group_owner(message,from_id))
    {
       string response = "群主";
        msg.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.is_admin(from_id,message))
    {
        string response = "管理员";
        msg.send_m(data_socket,"other", response);
        return;
    }
    string response = "成员";
    msg.send_m(data_socket,"other", response);
}
void GRO::quit_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    if(redis_data.is_group_owner(message,from_id))
    {
        if(redis_data.delete_group(message)&&redis_data.clear_group_files(message)&&redis_data.clear_group_request(message)&&redis_data.clear_group_messages(message))
        {
         string response = "已解散群聊";
         msg.send_m(data_socket,"other", response);
        return;
        }

    }
    if(redis_data.is_admin(from_id,message))
    {
        redis_data.remove_group_admin(message,from_id);
    }
    if(redis_data.remove_group_member(message,from_id))
    {
        string response = RED_TEXT( "已成功退出");
        cout<<"已发送"<<endl;
        msg.send_m(data_socket,"other", response);
        return;
    }
}
void GRO::delete_admin(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    if(!redis_data.is_in_group(message,to_id))
    {
       string response = RED_TEXT( "该用户不在该群聊内\n");
       msg.send_m(data_socket,"other", response);
        return;
    }
    if(!redis_data.is_group_owner(message,from_id))
    {
       string response = RED_TEXT( "只有群主可以移除管理员");
        msg.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.is_admin(to_id,message))
    {
        redis_data.remove_group_admin(message,to_id);
    }else{
        string response = RED_TEXT( "你不是管理员");
       msg.send_m(data_socket,"other", response);
        return;
    }
    string a =RED_TEXT("群主已撤销你的管理员"+message);
        client.send_notice(from_id,to_id,a,redis_data);
        string response = GREEN_TEXT("已成功移除");
      msg.send_m(data_socket,"other", response);
        
    
}
void GRO::add_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
   
     string username =redis_data.get_username_by_id(to_id);
    if(username == "")
    {
    //cout << user_id<<"用户不存在于数据库中" << endl
    string a =RED_TEXT("用户不存在");
    msg.send_m(data_socket,"other", a);
    return ;
    }
    if(redis_data.is_in_group(message,to_id))
    {
        string response =RED_TEXT("用户已在该群聊内");
        msg.send_m(data_socket,"other", response);
        return;
    }
    if(!redis_data.is_friend(to_id,from_id) &&!redis_data.is_friend(from_id,to_id))
    {
        string a =RED_TEXT("你与该用户不是好友");
        msg.send_m(data_socket,"other", a);
        cout<<a<<endl;
        return;
    }
    if(!redis_data.add_group_member(message,to_id))
    {
    string response = "邀请失败";
    msg.send_m(data_socket,"other", response);
            return;
 }
    string response = "已成功邀请";
    string notice =GREEN_TEXT("你的好友"+from_id+"已邀请你进入群"+message) ;
        client.send_notice(from_id,to_id,notice,redis_data);
  msg.send_m(data_socket,"other", response);
}
void GRO::send_add_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
   if(!redis_data.is_group_exists(to_id))
    {
        string response =RED_TEXT("该群不存在");
        msg.send_m(data_socket,"other", response);
       return;
    }
     if(redis_data.is_in_group(to_id,from_id))
    {
        string response =RED_TEXT("你已在该群聊内");
         msg.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.apply_to_group(from_id,to_id,message))
    {
        string response =GREEN_TEXT("已成功发送申请");
         msg.send_m(data_socket,"other", response);

    //告诉所有管理员
       vector<string> members = redis_data.get_all_members(to_id);
       for(int i = 0;i < members.size();i++)
       {
        if(redis_data.is_admin(members[i],to_id)||redis_data.is_group_owner(to_id,members[i]))
        {
        string notice =YELLOW_TEXT("群"+to_id+"有新的申请消息");
        client.send_notice(from_id,members[i],notice,redis_data);
        }
       }
    } 
}
void GRO::see_add_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{

   vector<pair<string, string>> applications = redis_data.get_unread_applications(message);
   string resultStr;
   if(applications.size() == 0)
   {
    resultStr = "无申请消息"; 
    msg.send_m(data_socket,"other", resultStr);
     return;
   }
   resultStr+="有"+to_string(applications.size())+"条新的申请\n";
 for (const auto& application : applications) {
    resultStr += "有来自：" + redis_data.get_username_by_id(application.first)+"("+application.first+")" + "的申请，验证消息为： " + application.second + "\n";
 }
 msg.send_m(data_socket,"other", resultStr);
}
void GRO::add_group_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    string status = "accept";
    string other_status = "refuse"; 
    if(!redis_data.check_group_apply(message,to_id))
    {
        string response = RED_TEXT("没有该条申请");
       msg.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.is_in_group(message,to_id))
    {
         string response =RED_TEXT("用户已在该群聊内");
        msg.send_m(data_socket,"other", response);
        return;
    }
     if(redis_data.check_group_status(to_id, message, status))
    {
        string response =RED_TEXT("该申请已被被同意，请勿重复处理");
       msg.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.check_group_status(to_id, message, other_status))
    {
        string response =RED_TEXT("该申请已被拒绝，请勿重复处理") ;
        msg.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.revise_group_status(to_id,message,status))
    {
        redis_data.add_group_member(message,to_id);
        string notice =GREEN_TEXT("你已被批准加入群"+message);
        redis_data.remove_group_application(to_id,message);
        client.send_notice(from_id,to_id,notice,redis_data);
        string response = "已同意该申请";
       msg.send_m(data_socket,"other", response);
       
    }
}
void GRO::store_chat_Pair(string first, string second) {
    group_pairs[first] = second;
    std::cout << "已存储: \"" << first << "\" -> \"" << second << "\"" << std::endl;
}
void GRO::printChatPairsTable() {
    if(group_pairs.empty()) {
        cout << "聊天关系表为空" << endl;
        return;
    }

    cout << "\n========== 聊天关系表 ==========" << endl;
    cout << "| " << setw(15) << left << "用户A" << " | " << setw(15) << "用户B" << " |" << endl;
    cout << "|-----------------|-----------------|" << endl;
    
    // 使用set避免重复打印双向关系
    set<pair<string, string>> printed;
    for(const auto& pair : group_pairs) {
        if(printed.find({pair.second, pair.first}) == printed.end()) {
            cout << "| " << setw(15) << left << pair.first 
                 << " | " << setw(15) << pair.second << " |" << endl;
            printed.insert({pair.first, pair.second});
        }
    }
    cout << "===================================" << endl;
}
// 检查a对应的b是否也对应a
bool GRO:: check_chat(string a,string b) {
    // printChatPairsTable();
    auto it_a = group_pairs.find(a);
    if (it_a == group_pairs.end() || it_a->second != b) {
        //cout << "键 \"" << a << "\" 不指向值 \"" << b << "\"" << std::endl;
        return false;
    }
    
  // cout << "确认对应: \"" << a << "\" ↔ \"" << b << "\"" << std::endl;
    return true;
}
bool GRO::delete_chat_pair(string first) {
    auto it = group_pairs.find(first);
    if (it != group_pairs.end()) {
        group_pairs.erase(it);
      //  std::cout << "已删除键为 \"" << first << "\" 的键值对" << std::endl;
        return true;
    }
    //std::cout << "未找到键为 \"" << first << "\" 的键值对" << std::endl;
    return false;
}
void GRO::refuse_group_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    string status = "accept";
    string other_status = "refuse"; 
    if(!redis_data.check_group_apply(message,to_id))
    {
        string response = RED_TEXT("没有该条申请");
       msg.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.is_in_group(message,to_id))
    {
        string response = RED_TEXT("用户已在该群聊内");
       msg.send_m(data_socket,"other", response);
        return;
    }
     if(redis_data.check_group_status(to_id, message, status))
    {
        string response = RED_TEXT("该申请已被同意，请勿重复处理");
        msg.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.check_group_status(to_id, message, other_status))
    {
        string response = RED_TEXT("该申请已被拒绝，请勿重复处理");
       msg.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.revise_group_status(to_id,message,other_status))
    {
    string notice =RED_TEXT("你的加群申请已经被驳回"+message);
    redis_data.remove_group_application(to_id,message);
    client.send_notice(from_id,to_id,notice,redis_data);
    string response = "已拒绝该申请";
    msg.send_m(data_socket,"other", response);
       
    }
}
void GRO::add_group_message(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    //判断一下再不在群聊，不在发不在群聊内
    if(!redis_data.is_in_group(to_id,from_id))
    {
        string message =RED_TEXT("你不在群聊内") ;
        msg.send_m(data_socket,"other", message);
        return;
    }
    string time = getCurrentTimestamp();
    //发送消息的时候还在群聊里面，更新一边最后时间
    
    vector<string> members = redis_data.get_all_members(to_id);
    for(int i = 0;i < members.size();i++)
    {
        if(check_chat(members[i],to_id)&&members[i] != from_id)
        {
        int b_socket =client.find_socket(members[i]);
        string notice = "对方正在聊天框内和你聊天";
       // cout<<notice<<endl;
        string type = redis_data.get_username_by_id(from_id);
        //msg.send_m(a_socket,"other", notice);
        msg.send_m(b_socket,type, message);
            //此时用户点开了群聊界面,直接给他发过去
        }else if(from_id != members[i])
    {
        string notice =YELLOW_TEXT("有来自群"+to_id+"的新消息") ;
        client.send_notice(from_id,members[i],notice,redis_data);
        //此时用户在线，给他一个通知
    }   
    }
  
    if(redis_data.add_group_message(from_id,message, to_id,time)&&redis_data.set_last_read_time(from_id,to_id,time))
    {
         string a= "消息发送成功";
         cout<<a<<endl;
    }
}
void GRO::receive_group_message(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    //每次调用接收函数的时候也在群聊里面，更新最后在线时间

    // redis_data.set_last_read_time(from_id,to_id,time);
}
void GRO::open_group_block(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    store_chat_Pair(from_id,to_id);
    vector<tuple<string, string, string>> messages = redis_data.get_read_messages(from_id, to_id,50);
    if(messages.size() == 0)
    {
        string result= "群聊暂无消息";
        msg.send_m(data_socket,"other", result);
        return;
    }
     sort(messages.begin(), messages.end(), 
        [](const auto& a, const auto& b) {
            return get<2>(a) < get<2>(b);  
        });

    
    stringstream ss;
    for (const auto& [fromid, message, timestamp] : messages) {
        ss << "[" <<redis_data.get_username_by_id( fromid )<< "] " << ": " << message << "\n";
    }

    string result = ss.str();
    if (!result.empty()) {
        result.pop_back();  // 移除最后一个换行符
    }
    msg.send_m(data_socket,"other",result);
    
    //cout<<"已发送"<<result<<endl;
}
void GRO::quit_chat(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
    delete_chat_pair(from_id);
    string notice = "quit"; 
    msg.send_m(data_socket,"other", notice);
} 
void GRO::send_file_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{
  if(!redis_data.is_in_group(to_id,from_id))
    {
        string message =RED_TEXT("你不在群聊内") ;
        msg.send_m(data_socket,"other", message);
        return;
    }else{
        string message = "group" ;
        msg.send_m(data_socket,"other", message);
    }
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
        time_t now = time(nullptr);
        tm* local_time = localtime(&now);
        char timebuf[32];
        strftime(timebuf, sizeof(timebuf), "_%Y%m%d_%H%M%S", local_time);
        string the_file = filename;
        filename = from_id + ":" + to_id + ":" + filename + timebuf;
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
           
        if(redis_data.add_file_group(to_id,from_id,filename)&&redis_data.add_file_to_unread_list(to_id,from_id,filename))
       {
        cout << "文件接收成功: " << filename << endl;
       }
        string ack = "SUCCESS";
        send(transfer_socket, ack.c_str(), ack.size(), 0);
       //告诉在线的成员
        vector<string> members = redis_data.get_all_members(to_id);
    for(int i = 0;i < members.size();i++)
    {
        
         if(from_id != members[i])
        {
        string notice =YELLOW_TEXT("有来自群"+to_id+"的新文件") ;
        client.send_notice(from_id,members[i],notice,redis_data);
        //此时用户在线，给他一个通知
        }   
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
void GRO::accept_file_group(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg) {
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
      for (int i = 0; i < result.size(); i++) {
        size_t split_pos = result[i].find('|');
        string send_fileid = result[i].substr(0, split_pos);      
        string filename = result[i].substr(split_pos + 1);   

        // 发送包含时间戳的文件名
        response += to_string(i + 1) + ". " + result[i] + "\n";
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
    send(data_socket, "VALID_CHOICE", 12, 0);
    
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
    
    ifstream file(filename, ios::binary);
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
 
    while(true) {
    file.read(buffer, BUFFER_SIZE);
    int bytes_read = file.gcount();
    if(bytes_read == 0) break;  // 文件读取完毕
    
    int sent_bytes = send(transfer_socket, buffer, bytes_read, 0);
    if(sent_bytes != bytes_read) {
        cerr << "文件发送中断" << endl;
        break;
    }
}
// 优雅关闭连接
shutdown(transfer_socket, SHUT_WR);  
    this_thread::sleep_for(chrono::milliseconds(150));
    redis_data.remove_unread_file(to_id,from_id,result[choice]);
}).detach();

}
void GRO::delete_member(TCP &client,int data_socket, string from_id, string to_id, string message, DATA& redis_data,MSG &msg)
{   if(!redis_data.is_in_group(message,to_id))
    {
        string response = "该用户不在群聊内\n";
        msg.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.is_group_owner(message,to_id))
    {
        string response = "不可以踢出群主\n";
        msg.send_m(data_socket,"other", response);
        return;
    }
    if(redis_data.is_admin(from_id,message)&&redis_data.is_admin(to_id,message))
    {
        string response = "不可以踢出管理员\n";
       msg.send_m(data_socket,"other", response);
        return;
    }
    
    if(redis_data.is_group_owner(message,from_id)||redis_data.is_admin(from_id,message))
    {
         if(redis_data.remove_group_member(message,to_id))
    {
         string response = "已成功将该用户踢出群聊";
         cout<<response<<endl;
        msg.send_m(data_socket,"other", response);
        return;
    }
    }
    
}