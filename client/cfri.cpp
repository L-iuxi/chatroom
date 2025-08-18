#include "cfri.hpp"
  //登陆成功后进入选择
void FRI:: make_choice(TCP &client,LOGIN &login){
    //心跳
    client.new_heartbeat_socket() ;
    //开始接收线程
    client.connect_notice_socket();
    //只作为离线消息接受一次
    client.recv_server_(client.data_socket);
    GRO group;
   
    while(1)
    {
    int command = -1; 
    string b;
    main_page(login.getuser_id(),login.getusername());
   while (!(cin >> command)) {
    clear_screen() ;
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
            cin.clear();
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            main_page(login.getuser_id(), login.getusername()); 
        }
        
   cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
    // cin>>command;
    // cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    // cin.clear();
    string type = "";
    string to_id = "";
    string from_id = "";
    string message= "";
    switch(command){
        case 1:
         cout<<"当前操作：私聊某好友"<<endl;
         see_all_friends(client,login.getuser_id());
        choose_command(client,login);
      
        break;
        case 2:
        cout<<"当前操作：管理好友"<<endl;
        manage_friends(client,login);
       
        break;
        case 3:
        cout<<"当前操作:查看好友列表"<<endl;
        see_all_friends(client,login.getuser_id());
       // client.recv_server(client.data_socket);
        break;
        case 4:
        cout<<"当前操作：查看群聊"<<endl;
        group.see_all_group(client,login); 
        //client.recv_server(client.data_socket);
        break;
        case 5:
        cout<<"当前操作：群聊管理"<<endl;
        manage_group(group,client,login);
       //  client.recv_server(client.data_socket);
        break;
          case 6:
         cout<<"当前操作：进入群聊"<<endl;
          group.see_all_group(client,login); 
         open_group(group,client,login);
         //client.recv_server(client.data_socket);
        break;
        
        case -1:
        cout<<"当前操作：退出"<<endl;
        message = "quit";
        type = "quit";
        from_id = login.getuser_id();
        client.send_m(type,from_id,to_id,message); 
        close(client.data_socket);
        close(client.heart_socket);
        close(client.notice_socket);
        break;
        default:  
        clear_screen() ;
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
            break;
    }   
    if(command == -1)
    {
        break;
    }
 }
 }
void FRI:: choose_command(TCP &client, LOGIN &login)
{
    
    string to_id;
    cout<<"请选择好友"<<endl;
    cin>>to_id;
    string type = "is_friends";
    string from_id = login.getuser_id();
    string message = "0";
    string buffer;
    client.send_m(type,from_id,to_id,message);
     client.rec_m(type,buffer);
     if(buffer != "成功")
     {
        PRINT_RED(buffer+"\n");
        return;
     }
    clear_screen() ;
    while(1)
    {
    int a;
    print_block();
    //cin.ignore();
    // cin>>a;
    while (!(cin >> a)) {
    clear_screen() ;
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
            cin.clear();
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
             print_block();
        }
           cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
    switch(a)
    {
        case 1:
        
        cout<<"当前操作：发送消息"<<endl;
        open_block(client,login,to_id);
        break;
        case 2:
        cout<<"当前操作：发送文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        send_file_to_friends(client, login, to_id);
        break;
        case 3:
        cout<<"当前操作：接收文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        accept_file(client,login,to_id);
        break;
        case -1:
        break;
        default: 
         clear_screen() ; 
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
            break;
    }
    if(a == -1)
    {
    clear_screen() ;
    break;
    }
 }
}
void FRI::accept_file(TCP &client, LOGIN &login,string to_id) {
   
    //暂停心跳监测
    //client.pause_heartbeat();
    const string download_dir = "../download";
    string type = "accept_file";
    string from_id = login.getuser_id();
    string message = "0";
    mkdir(download_dir.c_str(), 0777);

    client.send_m(type, from_id, to_id, message);

    char buffer1[BUFFER_SIZE];
    int bytes = recv(client.data_socket, buffer1, BUFFER_SIZE, 0);
    if (bytes <= 0) {
        cout << "服务器无响应" << endl;
        return;
    }
    buffer1[bytes] = '\0';
    string response(buffer1);
    if(response == "NO_FILES")
    {
        cout<<"\033[31m没有文件\033[0m"<<endl;
        return;
    }
    // 3. 显示文件列表并让用户选择
    cout << "可下载文件列表:\n" << buffer1;
    cout << "请输入要下载的文件序号: ";
    
    int choice;
    cin >> choice;
    
    
    string choice_str = to_string(choice);
    send(client.data_socket, choice_str.c_str(), choice_str.size(), 0);
    //client.resume_heartbeat();
    client.connect_transfer_socket();
    this->is_transfer = true;
  
  
    thread([this,&client,download_dir](){
    char buffer[BUFFER_SIZE];
    char filename_buf[256];
    int name_len = recv(client.transfer_socket, filename_buf, sizeof(filename_buf)-1, 0);
    if(name_len <= 0) {
        cerr << "\033[31m接收文件名失败\033[0m" << endl;
        close(client.transfer_socket);
        return;
    }
    filename_buf[name_len] = '\0';
    string filename = string(filename_buf);

    string save_path = download_dir + "/" + filename;
    ofstream outfile(save_path, ios::binary);
    if(!outfile) {
        cerr << "\033[31m无法创建文件:\033[0m " << save_path << endl;
        close(client.transfer_socket);
        return;
    }
    const char* ack_msg = "ACK";
    send(client.transfer_socket, ack_msg, strlen(ack_msg), 0);
    int total_received = 0;
    while(true) {
        int bytes_recv = recv(client.transfer_socket, buffer, BUFFER_SIZE, 0);
        if(bytes_recv <= 0) break;
        outfile.write(buffer, bytes_recv);
        total_received += bytes_recv;
    }
    outfile.close();

    cout<<"total_rec"<<total_received<<endl;
    
        cout << "\033[32m文件下载成功: " << " (" << total_received << " bytes)\033[0m" << endl;
        string ack = "SUCCESS";
        send(client.transfer_socket, ack.c_str(), ack.size(), 0);
     
    close(client.transfer_socket);
    this->is_transfer = false;
    
}).detach();
    
}
void FRI::send_file_to_friends(TCP &client, LOGIN &login, string to_id) {
    string filepath;
    string from_id = login.getuser_id();

    cout << "请输入文件名: ";
    cin >> filepath;

    // 检查文件是否存在
    ifstream test_file(filepath, ios::binary | ios::ate);
    if (!test_file) {
        cerr << "\033[31m文件 " << filepath << " 不存在\033[0m" << endl;
        return;
    }
    // 检查是否是目录
    if (std::filesystem::is_directory(filepath)) {
        cerr << "\033[31m" << filepath << " 是目录而不是文件\033[0m" << endl;
        return;
    }
    // 获取文件总大小
    streampos file_size = test_file.tellg();
    test_file.close();

    this->is_transfer = true;
    client.send_m("send_file", from_id, to_id, filepath);
    string type;
    string buffer;
   client.rec_m(type,buffer);
   if(buffer != "friend")
   {
    this->is_transfer = false;
    cout<<buffer<<endl;
    return;
   }
    client.connect_transfer_socket();

    std::thread([this, &client, from_id, to_id, filepath, file_size]() {
        ifstream file(filepath, ios::binary);
        if (!file) {
            cerr << "\033[31m文件打开失败: " << filepath << "\033[0m" << endl;
            this->is_transfer = false;
            return;
        }

        // 进度显示参数
        int total_chunks = (static_cast<int>(file_size) + BUFFER_SIZE - 1) / BUFFER_SIZE;
        int current_chunk = 0;
        time_t start_time = time(nullptr);

        char buffer[BUFFER_SIZE];
        while (!file.eof()) {
            file.read(buffer, BUFFER_SIZE);
            int bytes_read = file.gcount();
            current_chunk++;

            // 发送文件数据
            if (send(client.transfer_socket, buffer, bytes_read, 0) != bytes_read) {
                cerr << "\033[31m文件传输中断\033[0m" << endl;
                break;
            }

            // 计算并显示进度
            float progress = 100.0f * current_chunk / total_chunks;
            time_t now = time(nullptr);
            double elapsed = difftime(now, start_time);
            double speed = (current_chunk * BUFFER_SIZE) / (elapsed * 1024); // KB/s
            
            // 进度条显示
            // cout << "\r[";
            // int pos = 50 * progress / 100;
            // for (int i = 0; i < 50; ++i) {
            //     if (i < pos) cout << "=";
            //     else if (i == pos) cout << ">";
            //     else cout << " ";
            // }
            // cout << "] " << fixed << setprecision(1) << progress << "% "
            //      << "(" << bytes_read * current_chunk / 1024 << "KB/" 
            //      << file_size / 1024 << "KB) "
            //      << speed << "KB/s";
            // cout.flush();
        }

        file.close();
        shutdown(client.transfer_socket, SHUT_WR);
        //cout<<"准备接收ack"<<endl;
        // 接收ACK
        char ack[16] = {0};
        int h = recv(client.transfer_socket, ack, sizeof(ack) - 1, 0);
        if (h > 0) {
            ack[h] = '\0';
            cout << "\n"; // 换行避免覆盖进度条
            if (string(ack) == "SUCCESS") {
                cout << "\033[32m文件发送成功\033[0m" << endl;
            } else {
                cout << "\033[31m错误: " << ack << "\033[0m" << endl;
            }
        }
       // cout<<"接收ack"<<endl;
        close(client.transfer_socket);
        this->is_transfer = false;
    }).detach();
}
void FRI:: manage_friends(TCP &client,LOGIN &login)
{
    while(1)
    {
    int command = 0; 
    string b;
    main_page2(login.getuser_id(),login.getusername());
   
     while (!(cin >> command)) {
    clear_screen() ;
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
            cin.clear();
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            main_page2(login.getuser_id(),login.getusername());
        }
        
   cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
    string type = "";
    string to_id = "";
    string from_id = "";
    string message= "";
    switch(command){
        case 1:
        
        cout<<"当前操作：添加好友"<<endl;
        add_friend(client,login.getuser_id());
        //heartbeat_received = false;
        break;
        case 2:
        cout<<"当前操作:删除好友"<<endl;
        see_all_friends(client,login.getuser_id());
        delete_friend(client,login.getuser_id());
        break;
        case 3:
        cout<<"当前操作：屏蔽好友"<<endl;
        see_all_friends(client,login.getuser_id());
        shidld_friend(client,login);
        break;
        case 4:
        cout<<"当前操作：取消屏蔽好友"<<endl;
        cancel_shidld_friend(client,login);

        break;
        case 5:
        cout<<"当前操作：查看好友申请"<<endl;
        check_add_friends_request(client,login);
        break;
        case -1:
        cout<<"当前操作：退出"<<endl;
       
        return;
        default:  
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
            break;
        break;
    }   
    if(command == -1||heartbeat_received == false)
    {
        break;
    }
 }
}
void FRI:: cancel_shidld_friend(TCP &client,LOGIN &login)
{

   // char buffer[BUFFER_SIZE];
    string type = "cancel_shield_friend";
    string from_id = login.getuser_id();
    string to_id;
    string message = "0";
    cout<<"请输入想要取消屏蔽的好友id"<<endl;
    cin>>to_id;

    client.send_m(type,from_id,to_id,message);
     string buffer; 
     client.rec_m(type,buffer);
   cout<<buffer<<endl;
}
void FRI:: shidld_friend(TCP &client,LOGIN &login)
{
    //char buffer[BUFFER_SIZE];
    string type = "shield_friend";
    string from_id = login.getuser_id();
    string to_id;
    string message = "0";
    cout<<"请输入想要屏蔽的好友id"<<endl;
    cin>>to_id;

    client.send_m(type,from_id,to_id,message);
    
     string buffer; 
     client.rec_m(type,buffer);
   
     cout<<buffer<<endl;
 }
//打开聊天框
void FRI:: send_message_no(TCP &client,string from_id,string to_id)
{
    //cin.ignore(numeric_limits<streamsize>::max(), '\n');
     // 等待发送消息的信号
    while (chat_active.load()) {
    string message;
    string type = "send_message_no";
    if (!chat_active.load()) break;  
    
    getline(cin, message);
    // if(message.size() > 256)
    // {
    //     cout<<"消息太长了！"<<endl;
    // }
    if (!chat_active.load()) break; 
    if(message == "-1")
    {
    message = "quit_chat";
    type = "quit_chat";
    client.send_m(type,from_id,to_id,message);
    chat_active = false;
    break;
    }
    client.send_m(type,from_id,to_id,message);
    }
   // cout<<"我结束了"<<endl;
}
void FRI ::receive_log(TCP& client,string from_id,string to_id)
{

    while(chat_active.load())
    {
    string type;
    string message;
    
    string buffer; 
    if (!chat_active.load()) break; 
    if(!client.rec_m(type,buffer))
    {
        chat_active = false;
        break;
    }
    
    if (!chat_active.load()) break; 
   
    if(buffer == "quit")
    {
         chat_active = false;
         clear_screen() ;
         break;
    } 
    cout<<endl;
    cout<<"\033[1;36m["<<type<<"]\033[0m"<<buffer<<endl;
  
        
 }
//cout<<"我也结束了"<<endl;
}
void FRI:: open_block(TCP &client,LOGIN &login,string to_id)
{
    string from_id,type,message;
    type = "friend_open_block";
    from_id = login.getuser_id();
    message = "0";
    client.send_m(type,from_id,to_id,message);
    
    string buffer; 
    client.rec_m(type,buffer);
        if(buffer == "你与该用户还不是好友")
        {
        cout<<buffer<<endl;
        return;
        }
        cout<<"-------------------------"<<endl;
        cout << buffer << endl;
        cout<<"-----以上为历史聊天记录-----"<<endl;
    
    
    chat_active = true; 
    thread receive_thread(std::bind(&FRI::receive_log, this, std::ref(client), from_id, to_id)); 
    thread send_thread(std::bind(&FRI::send_message_no, this, std::ref(client), from_id, to_id)); 

    receive_thread.join();
    send_thread.join(); 
}

//添加好友函数
void FRI:: add_friend(TCP& client,string from_user_id){
    string to_id;
    string from_id;
    string message;
    string type;
    int bytes;
    //char buffer[BUFFER_SIZE];
   string buffer;

    from_id = from_user_id;
    cout<<"请输入要添加的好友id OVO"<<endl;
    cin>>to_id;
    cout<<"请输入一条验证消息"<<endl;
    cin.ignore(); 
    getline(cin, message); 
    
    type = "send_add_friends_request";
    client.send_m(type,from_id,to_id,message);
   
   // bytes = recv(client.data_socket,buffer,BUFFER_SIZE,0);
    // if(bytes < 0)
    // {
    //     cout<<"发送好友申请后服务器无数据返回"<<endl;
    // }else{
    //     buffer[bytes] = '\0';
    //     string data = string(buffer);
    //     cout<<data<<endl;
    // }
    client.rec_m(type,buffer);
    cout<<buffer<<endl;
    

}
void FRI::delete_friend(TCP& client, string from_id) {
    string to_id;
    string message;
    string type;
    int bytes;
    int sure;
    string buffer;

    cout << "请输入要删除的好友id TAT" << endl;
    cin >> to_id;
    
    // Clear any error flags and ignore remaining input in case of previous errors
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    while (true) {
        cout << "确定要删除该好友吗TAT" << endl;
        PRINT_GREEN("【1】确定");
        PRINT_RED("【2】点错了");
        
        cin >> sure;
        
        if (cin.fail()) {
            // Input was not an integer
            cin.clear(); // Clear error flags
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Discard bad input
            cout << "请输入数字1或者2" << endl;
            continue;
        }
        
        if (sure == 1) {
            type = "delete_friend";
            message = "0";
            client.send_m(type, from_id, to_id, message);
            client.rec_m(type, buffer);
            cout << buffer << endl;
            break;
        } else if (sure == 2) {
            // User chose to cancel
            break;
        } else {
            cout << "请输入1或者2" << endl;
            // Clear any remaining input
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
    
    return;
}
void FRI::see_all_friends(TCP &client,string from_id){
    char buffer[BUFFER_SIZE];
    string to_id ="0";
    string message = "0";
    string type = "see_all_friends";
    client.send_m(type,from_id,to_id,message);
    string recover;
    
    client.rec_m(type,recover);
   
   cout<<recover<<endl;
}
//查看好友申请消息
void FRI:: check_add_friends_request(TCP &client,LOGIN &login){
    //char buffer[BUFFER_SIZE];
    vector<FriendRequest> requests;
    string serialized_data;
   // cout<<"当前操作：查看该用户id下的好友申请"<<endl;
    string type = "check_add_friends_request";
    string from_id = login.getuser_id();
    string to_id= "0" ;
    string message = "0";
    client.send_m(type,from_id,to_id,message);

    string buffer;
    client.rec_m(type,buffer);
    
    
    if(buffer == "无好友申请")
    {
        PRINT_RED("还没有好友申请哦\n");
        return;
    }
  
    serialized_data = buffer; 
    

    nlohmann::json j = nlohmann::json::parse(serialized_data);
        
    for (const auto& item : j) {
        FriendRequest req;
        req.from_id = item["from_id"];
        
        req.message = item["message"];
        req.status = item["status"];
        requests.push_back(req);
    }
    cout<<"**********************************************"<<endl;
    for (const auto& req : requests)
    {
        cout<<"用户"<<req.from_id<<endl<<"-----message:"<<req.message<<endl;
    }
    cout<<"**********************************************"<<endl;
        int command = -1;
        
    PRINT_GREEN("【1】同意某好友申请");
    PRINT_RED("【2】拒绝某好友申请");
        cout<<"【-1】.退出"<<endl;
         while(1)
        {
            cin>>command;
        if (cin.fail()) {
        cout<<"请输入1或者2"<<endl;
        cin.clear(); // 清除错误状态
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 清空缓冲区
        continue;
    }
        if(command != 1 &&command != 2 &&command != -1)
        {
            continue;

        cout<<"请输入1或者2"<<endl;
        }else {
                break;
        }
        }
        if(command == 1)
        {
            //char buffer2[BUFFER_SIZE]; 
            string buffer2;   
            cout<<"请输入想要通过的申请人id"<<endl;
            string to_id;
            cin>>to_id;
            string message = "0";
            
            type = "approve_friends_requests";
            client.send_m(type,from_id,to_id,message);
            client.rec_m(type,buffer2);
            cout<<buffer2<<endl;

        }else if(command == 2){
            string buffer2;
            cout<<"请输入想要拒绝的申请人id"<<endl;
            string to_id;
            cin>>to_id;
            string message = "0";
            type = "refuse_friends_requests";
            client.send_m(type,from_id,to_id,message);
            client.rec_m(type,buffer2);
            cout<<buffer2<<endl;
        }else if(command == -1)
        {
        return;
        }
}
//这个函数可以不用了
void FRI:: check_message(TCP &client,LOGIN &login){
    int command;
    string type,from_id,to_id,message;
    cout<<"请选择查看的消息类型"<<endl;
    cout<<"1.查看好友申请"<<endl<<"2.查看群申请"<<"-1.退出"<<endl;
    cin>>command;
    while(1)
        {
            if(command != 1 ||command != 2 ||command != -1)
            {
        cin>>command;

        cout<<"请输入1或者2"<<endl;
            }else {
                break;
            }
        }
    if(command == 1)
    {
        check_add_friends_request(client,login);
    }
    else if(command == 2)
    {
        //check_add_group_request(client,login);
    }else if(command = -1)
    {
        string from_id = login.getuser_id();
       string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
        return;
    }
    //client.rec_m(type,from_id,to_id,message);
   
}
void FRI::manage_group(GRO &group,TCP &client,LOGIN &login)
{
     while(1)
    {
    int command = 0; 
    string b;
    main_page3(login.getuser_id(),login.getusername());
    while (!(cin >> command)) {
    clear_screen() ;
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
           
             cin.clear();
              cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
          main_page3(login.getuser_id(),login.getusername());
        }
        
   cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
//  / / /
//     cin>>command;
//     cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//     cin.clear();
    string type = "";
    string to_id = "";
    string from_id = "";
    string message= "";
    switch(command){
        case 1:
        cout<<"当前操作：创建群聊"<<endl;
        group.generate_group(client,login); 
        break;
        case 2:
        cout<<"当前操作：删除群聊"<<endl;
        group.see_all_group(client,login);
        group.delete_group(client,login); 
        break;
        case 3:
        cout<<"当前操作：加入群聊"<<endl;
        group.send_add_group(client,login); 
        break;
        
        case -1:
        cout<<"当前操作：退出"<<endl;
        return;
        break;
        default:  
        clear_screen() ;
      cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
            
        break;
    }   
    if(command == -1)
    {
        break;
    }
 }
}
void FRI::open_group(GRO &group,TCP &client,LOGIN &login)
{
    
    string group_id;
    cout <<"请输入要进入的群号"<<endl;
    cin>>group_id;
    //string type = "open_group";
    string from_id = login.getuser_id();
    string to_id = "0";
    string message = group_id;
    int m = 0;
    string buffer;
    while(1)
    {
    string type = "open_group";
    if(m == 1)
    {
      //  cout<<m<<endl;
    break;
    }
    m = 0;
    client.send_m(type,from_id,to_id,message);
    client.rec_m(type,buffer);
   // cout << buffer << endl;
        if(buffer == "该群不存在")
        {
            cout<<"该群不存在"<<endl;
            return;
        }else if(buffer == "你不在该群聊内")
        {
            cout <<"你不在该群聊内"<<endl;

            return;
        }else if(buffer =="群主")
        {
             //cout<<"群主"<<endl;
            group.open_group_owner(client,login,m,group_id);
           
        }else if(buffer == "管理员")
        {
            group.open_group_admin(client,login,m,group_id);
        }else if(buffer== "成员")
        {
            group.open_group_member(client,login,m,group_id);
            
        }
    // }
}
}