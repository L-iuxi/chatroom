#include "client.hpp"
//对客户端发来的用户名进行切割
string extractUsername(const string& message) {
   
    size_t startPos = message.find(":");
    size_t endPos = message.find("登");
    // cout << "message: " << message << endl;
   // cout << "startPos: " << startPos << ", endPos: " << endPos << endl;
   // cout<<"切割后的字符串为"<<
    if (startPos != string::npos && endPos != string::npos && startPos < endPos) { 
       // cout<<"切割后的字符串为"<<message.substr(startPos + 1, endPos - startPos - 1)<<endl;
        //cout<<"111"<<endl;
        return message.substr(startPos + 1, endPos - startPos - 1); 
    }
    
    return "";
}
//创建客户端套接字
TCP::TCP(const char* SERVER_IP){
        
    struct sockaddr_in server_address;
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        perror("Invalid address or Address not supported");
        return;
    }

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        return ;
    }

    // const char *message = "Hello from client";
    // send(client_socket, message, strlen(message), 0);
    
    }
  
// 发送消息）
void TCP::send_m(string type, string from_sb, string to_sb, string message) {
    nlohmann::json j;
    j["type"] = type;
    j["from_id"] = from_sb;
    j["to_id"] = to_sb;
    j["message"] = message;
    
    string serialized_message = j.dump();
    
    // 准备发送缓冲区（4字节长度 + 数据）
    vector<char> send_buf(sizeof(uint32_t) + serialized_message.size());
    
    // 写入网络字节序的长度前缀
    uint32_t len = htonl(serialized_message.size());
    memcpy(send_buf.data(), &len, sizeof(len));
    
    // 写入JSON数据
    memcpy(send_buf.data() + sizeof(len), serialized_message.data(), serialized_message.size());
    
    if (send(this->data_socket, send_buf.data(), send_buf.size(), 0) != send_buf.size()) {
        cerr << "发送消息失败" << endl;
    }
}

// 接收消息
bool TCP::rec_m(string &type, string &message) {
   
    uint32_t len;
    ssize_t bytes = recv(this->data_socket, &len, sizeof(len), MSG_WAITALL);
    if (bytes != sizeof(len)) {
        if (bytes == 0)
        { 
        heartbeat_received = false;
       cout<<"与服务器连接已断开"<<endl;
       close(data_socket);

        }
    }
    
    len = ntohl(len); 
    
   
    vector<char> buf(len);
    bytes = recv(this->data_socket, buf.data(), len, MSG_WAITALL);
    if (bytes != len) {
        if (bytes == 0)
        { 
        heartbeat_received = false;
       cout<<"与服务器连接已断开"<<endl;
       close(data_socket);

        }
      
    }
   // cout<<"接收到数据"<<buf.data()<<endl;
    
    try {
        auto parsed = json::parse(string(buf.begin(), buf.end()));
        type = parsed["type"];
        message = parsed["message"];
        //cout<<"接收到message"<<message<<endl;
        return true;
    } catch (const json::parse_error& e) {
        throw runtime_error("JSON解析失败: " + string(e.what()));
    }
}
void TCP:: heartbeat()
{
    while(1)
    {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, []{ return !is_paused; });

    string type = "heart";
    string message = "ping";
    string to_id = "0";
    string from_id = "0";
    send_m(type,from_id,to_id,message);
    rec_m(type,message);
    if (message == "pong") {  
     //cout<<"收到心跳监测"<<endl;
    heartbeat_received = true;  
    }
    }
}
void TCP::new_socket(){
    int data_port;

    int bytes_rec = recv(this->client_socket,&data_port,sizeof(data_port),0);
    if(bytes_rec == -1)
    {
        cerr<<"建立数据套接字时接收数据失败"<<endl;
        return;
    }
    cout<<"接收到服务器发来的端口"<<data_port<<endl;

    this->data_socket = socket(AF_INET,SOCK_STREAM,0);
    if(this->data_socket == -1)
    {
        cerr<<"failed  to create datasocket" <<endl;
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(connect(this->data_socket,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1)
    {
        cerr << "Failed to connect to data port." << endl;
            close(this->data_socket);
            return;
    }
    this->heart_socket = socket(AF_INET,SOCK_STREAM,0);
    if(connect(this->heart_socket,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1)
    {
        cerr << "Failed to connect to data port." << endl;
            close(this->heart_socket);
            return;
    }
    cout << "Connected to data port: " << data_port << endl;
     
 }
void TCP::connect_transfer_socket() {
    int transfer_port;
    
    // 接收服务器发来的传输端口号
    if(recv(data_socket, &transfer_port, sizeof(transfer_port), 0) <= 0) {
        cerr << "接收传输端口失败" << endl;
        return;
    }

    // 创建传输套接字
    transfer_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(transfer_socket == -1) {
        cerr << "传输套接字创建失败" << endl;
        return;
    }

    // 连接服务器
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(transfer_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 根据实际情况修改

    if(connect(transfer_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "连接传输套接字失败" << endl;
        close(transfer_socket);
        return;
    }

    cout << "已连接到传输端口：" << transfer_port << endl;
}
//login类关于登陆注册注销的函数

void LOGIN::register_user(TCP& client){
    string usename;
    string password;
    cout<<"请输入用户名"<<endl;
    cin>>usename;
    send(client.data_socket, usename.c_str(), usename.length(), 0); 
    
    while(1)
    {
    cout<<"请输入密码"<<endl;
    cin>>password;
    if(password.size() < 6)
    {
        cout<<"密码必须大于六位"<<endl;
    }else{
        break;
    }
    }
    send(client.data_socket, password.c_str(), password.length(), 0);  


    char buffer[1024] = {0};
    int bytes_received = recv(client.data_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0'; 
        cout << buffer << endl;  
        //string data(buffer, bytes_received );
        // if(data.substr(0, 5) == "注册成功啦")
        // {
           
        // }
    } else {
        cout << "接收数据失败!" << endl;
    }
 } 
bool LOGIN::login_user(TCP& client){
    string user_id;
    string password;
    char buffer[1024] = {0};
   
    cout<<"请输入账号"<<endl;
    cin>>user_id;
    send(client.data_socket, user_id.c_str(), user_id.length(), 0);  
    
    cout<<"请输入密码"<<endl;
    cin>>password;
    send(client.data_socket, password.c_str(), password.length(), 0);  
    
    int bytes_received = recv(client.data_socket,buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
          
       // cout << "接收到服务器发来的字符串："<<buffer << endl;
        string recover = string(buffer,bytes_received);
        string last6 = recover.substr(recover.length() - 6);
        if(last6 == "啦！")
        {
        //cout<<"截取的字符串为"<<last6<<endl;
        this->username = extractUsername(recover);
        //cout<<"用户名为"<<this->username<<endl;
        this->user_id = user_id;
        this->passward = passward;  
            return true;
        }else 
        {
        //cout<<"截取的字符串为"<<last6<<endl;
        cout<<recover<<endl;
            return false;
        }
    } else {
        cout << "接收数据失败!" << endl;
    }
    return false;
 }
void TCP::recv_server(int data_socket)
    {
        char b[1024];
        int by = recv(data_socket,b,sizeof(b),0);
    if(by > 0 )
    {
        b[by] = '\0';
         cout<<"*************************************"<<endl;
         cout<<b<<endl;
         cout<<"*************************************"<<endl;
    }else{
        cout<<"无数据"<<endl;
    }
    }
   
void LOGIN::deregister_user(TCP& client){
    string user_id;
    string password;
    string username;
    char buffer[1024] = {0};
    cout<<"请输入账号"<<endl;
    cin>>user_id;
    if (user_id.size() == 0)
    {
    cout<<"不可以输入空字符串^ ^"<<endl;
    return;
    }
    send(client.data_socket, user_id.c_str(), user_id.length(), 0);  
    // cout<<"请输入用户名"<<endl;
    // cin>>username;
    // send(client.getClientSocket(), username.c_str(), username.length(), 0);  
    cout<<"请输入密码"<<endl;
    cin>>password;
    if (password.size() == 0)
    {
    cout<<"不可以输入空字符串^ ^"<<endl;
    return;
    }
    send(client.data_socket, password.c_str(), password.length(), 0);  
     buffer[1024] = {0};
    
   int bytes_received = recv(client.data_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  
        cout << buffer << endl;  
    } else {
        cout << "接收数据失败!" << endl;
    }
    
  }

//登陆成功后进入选择
void FRI:: make_choice(TCP &client,LOGIN &login){
    //const char* message = nullptr; 
    //int command; 

     client.recv_server(client.data_socket);
     GRO group;
    while(1)
    {
   
    int command = 0; 
    string b;
    main_page(login.getuser_id(),login.getusername());
   
   
    cin>>command;
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    cin.clear();
    string type = "";
    string to_id = "";
    string from_id = "";
    string message= "";
    switch(command){
        case 1:
         cout<<"当前操作：私聊某好友"<<endl;
        //open_block(client,login);
        choose_command(client,login);
        client.recv_server(client.data_socket);
        break;
        case 2:
        cout<<"当前操作：管理好友"<<endl;
        manage_friends(client,login);
         client.recv_server(client.data_socket);
        break;
        case 3:
        cout<<"当前操作:查看好友列表"<<endl;
        see_all_friends(client,login.getuser_id());
        client.recv_server(client.data_socket);
        break;
        case 4:
        cout<<"当前操作：查看群聊"<<endl;
        group.see_all_group(client,login); 
        client.recv_server(client.data_socket);
        break;
        case 5:
        cout<<"当前操作：群聊管理"<<endl;
        manage_group(group,client,login);
         client.recv_server(client.data_socket);
        break;
          case 6:
         cout<<"当前操作：进入群聊"<<endl;
         open_group(group,client,login);
         client.recv_server(client.data_socket);
        break;
        
        case -1:
        cout<<"当前操作：退出"<<endl;
        message = "quit";
        type = "quit";
        from_id = login.getuser_id();
        client.send_m(type,from_id,to_id,message);
      
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
    //加一个是不是好友的返回
    while(1)
    {
    int a;
    print_block();
    cin>>a;
    switch(a)
    {
        case 1:
        cout<<"当前操作：发送消息"<<endl;
        open_block(client,login,to_id);
        client.recv_server(client.data_socket);
        break;
        case 2:
        cout<<"当前操作：发送文件"<<endl;
        send_file_to_friends(
    client, 
    login, 
    to_id,
    [](const std::string& ack) {  // 回调函数
        if (ack == "SUCCESS") {
            std::cout << "\033[32m文件发送成功！\033[0m" << std::endl;
        } else {
            std::cerr << "\033[31m发送失败: " << ack << "\033[0m" << std::endl;
        }
    }
);
        client.recv_server(client.data_socket);
        break;
        case 3:
        cout<<"当前操作：接收文件"<<endl;
        accept_file(client,login,to_id);
        client.recv_server(client.data_socket);
        break;
    }
    if(a == -1)
    {
    string type = "nothing";
    string message = "0";
    string from_id = login.getuser_id();
    client.send_m(type,from_id,to_id,message);
    break;
    }
}
}
void FRI::accept_file(TCP &client, LOGIN &login,string to_id) {
    // string to_id;
    // cout << "请输入好友ID: ";
    // cin >> to_id;
    //暂停心跳监测
    client.pause_heartbeat();
    const string download_dir = "download";
    string type = "accept_file";
    string from_id = login.getuser_id();
    string message = "0";
    mkdir(download_dir.c_str(), 0777);

    client.send_m(type, from_id, to_id, message);

    char buffer[BUFFER_SIZE];
    int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    if (bytes <= 0) {
        cout << "服务器无响应" << endl;
        return;
    }
    buffer[bytes] = '\0';
    string response(buffer);
    if(response == "NO_FILES")
    {
        cout<<"\033[31m没有文件\033[0m"<<endl;
        return;
    }

    
    // 3. 显示文件列表并让用户选择
    cout << "可下载文件列表:\n" << buffer;
    cout << "请输入要下载的文件序号: ";
    
    int choice;
    cin >> choice;
    cin.ignore(); // 清除输入缓冲区
    
    string choice_str = to_string(choice);
    send(client.data_socket, choice_str.c_str(), choice_str.size(), 0);

    client.connect_transfer_socket();

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

    if(total_received > 0) {
        cout << "\033[32m文件下载成功: " << save_path << " (" << total_received << " bytes)\033[0m" << endl;
    } else {
        remove(save_path.c_str());
        cout << "\033[31m文件下载失败\033[0m" << endl;
    }
    client.resume_heartbeat();//恢复心跳监测
    close(client.transfer_socket);
    
    
}
void FRI:: send_file_to_friends(TCP &client, LOGIN &login, const string &to_id,AckCallback callback  ) {
    client.pause_heartbeat();
    string filepath;
    string from_id = login.getuser_id();

    cout << "请输入文件名: ";
    cin >> filepath;

    // 检查文件是否存在
    ifstream test_file(filepath);
    if (!test_file) {
        cerr << "\033[31m文件 " << filepath << " 不存在\033[0m" << endl;
        client.resume_heartbeat();
        return;
    }
    test_file.close();

    // 启动线程发送文件并等待 ACK
    std::thread([&client, from_id, to_id, filepath, callback]() {
        try {
            // 1. 发送文件元信息
            client.send_m("send_file", from_id, to_id, filepath);
            client.connect_transfer_socket();

            // 2. 传输文件内容
            ifstream file(filepath, ios::binary);
            if (!file) {
                cerr << "\033[31m文件打开失败: " << filepath << "\033[0m" << endl;
                return;
            }

            char buffer[BUFFER_SIZE];
            while (!file.eof()) {
                file.read(buffer, BUFFER_SIZE);
                int bytes_read = file.gcount();
                if (send(client.transfer_socket, buffer, bytes_read, 0) != bytes_read) {
                    cerr << "\033[31m文件传输中断\033[0m" << endl;
                    break;
                }
            }
            file.close();
            shutdown(client.transfer_socket, SHUT_WR);

            // 3. 持续接收，直到拿到 ACK
            char ack[16] = {0};
            while (true) {
                int h = recv(client.data_socket, ack, sizeof(ack) - 1, 0);
                if (h > 0) {
                    ack[h] = '\0';
                    callback(string(ack));  // 调用回调函数处理 ACK
                    break;  // 收到 ACK，退出循环
                }
                // 可加超时检测，避免无限等待
            }
        } catch (...) {
            callback("ERROR");  // 异常时回调
        }
        close(client.transfer_socket);
        client.resume_heartbeat();
    }).detach();  // 仍然 detach，但通过回调确保 ACK 被处理
}
void FRI:: manage_friends(TCP &client,LOGIN &login)
{
    while(1)
    {
    int command = 0; 
    string b;
    main_page2(login.getuser_id(),login.getusername());
   
   
    cin>>command;
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    cin.clear();
    string type = "";
    string to_id = "";
    string from_id = "";
    string message= "";
    switch(command){
        case 1:
        
        cout<<"当前操作：添加好友"<<endl;
        add_friend(client,login.getuser_id());
       heartbeat_received = false;
        break;
        case 2:
        cout<<"当前操作:删除好友"<<endl;
        delete_friend(client,login.getuser_id());
        break;
        case 3:
        cout<<"当前操作：屏蔽好友"<<endl;
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
        string from_id = login.getuser_id();
       string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
        return;
    
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
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
   
        // 等待发送消息的信号
    while (chat_active.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 避免占用过多CPU资源
    
    string message;
    string type = "send_message_no";
      
    //cout<<"请输入消息"<<endl;
     if (!chat_active.load()) break; 
    //cin.ignore(); 
    getline(cin, message);
    if(message == "-1")
    {
       chat_active = false;
        break;
    }
    client.send_m(type,from_id,to_id,message);
    }
    cout<<"我结束了"<<endl;
}
void FRI ::receive_log(TCP& client,string from_id,string to_id)
{

    while(chat_active.load())
    {
    string type = "new_message";
    string message = "0";
    client.send_m(type,from_id,to_id,message);
    // int bytes;
    // char buffer[BUFFER_SIZE];
    string buffer; 
    client.rec_m(type,buffer);
    
        if(buffer == "没有新消息")
        {
            this_thread::sleep_for(chrono::milliseconds(1500));
            continue;
        }else if(buffer == "对方已把你拉黑")
        {
            cout<<buffer<<endl;
            chat_active = false;
            //break;
        }else{
            cout<<"："<<buffer<<endl;
        }
        
 }
cout<<"我也结束了"<<endl;
}
void FRI:: open_block(TCP &client,LOGIN &login,string to_id)
{
    //暂停心跳监测
    
    string from_id,type,message;
    // cout<<"请选择好友"<<endl;
    // cin>>to_id;
    type = "friend_open_block";
    from_id = login.getuser_id();
    message = "0";
    client.send_m(type,from_id,to_id,message);
    //历史聊天记录
    
    string buffer; 
    client.rec_m(type,buffer);
  
        if(buffer == "你与该用户还不是好友")
        {
        cout<<buffer<<endl;

        type = "nothing";
        message = "0"; 
        client.send_m(type,from_id,to_id,message);
        return;
        }
        cout<<"-------------------------"<<endl;
       cout << buffer << endl;
        cout<<"-----以上为历史聊天记录-----"<<endl;
         cout << "正在加载聊天界面，请稍候..." << endl;
    
    // 等待3秒让用户有时间查看历史记录
    this_thread::sleep_for(chrono::seconds(3));

        //cout<<"data is"<<data<<endl;
    // }
    client.pause_heartbeat();
     chat_active = true; 
    thread receive_thread(std::bind(&FRI::receive_log, this, std::ref(client), from_id, to_id)); 
    thread send_thread(std::bind(&FRI::send_message_no, this, std::ref(client), from_id, to_id)); 

    receive_thread.join();
    send_thread.join();
   client.resume_heartbeat();//恢复心跳监测
 close(client.transfer_socket);
 
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
void FRI:: delete_friend(TCP& client,string from_id){
    string to_id;
    string message;
    string type;
    int bytes;
    int sure;
    //char buffer[BUFFER_SIZE];
    string buffer;
    cout<<"请输入要删除的好友id TAT"<<endl;
    cin>>to_id;
    cout<<"确定要删除该好友吗TAT"<<endl;
    cout<<"1.确定"<<endl<<"2.点错了> <"<<endl;
    cin>>sure;
    if(sure == 1)
    {
    type = "delete_friend";
    message = "0";
    client.send_m(type,from_id,to_id,message);
    client.rec_m(type,buffer);
    // bytes = recv(client.data_socket,buffer,BUFFER_SIZE,0);
    // if(bytes < 0)
    // {
    //     cout<<"发送好友申请后服务器无数据返回"<<endl;
    // }else{
    //     buffer[bytes] = '\0';
    //     string data = string(buffer);
        cout<<buffer<<endl;
    // }
    }
    return;
   
  
}
void FRI::see_all_friends(TCP &client,string from_id){
    char buffer[BUFFER_SIZE];
    string to_id ="0";
    string message = "0";
    string type = "see_all_friends";
    client.send_m(type,from_id,to_id,message);
    //client.send_m("nothing",from_id,to_id,message);
    
    //int bytes = recv(client.data_socket,buffer,BUFFER_SIZE,0);
    string recover;
    
    client.rec_m(type,recover) ;
    // if(bytes < 0)
    // {
    //      buffer[bytes] = '\0';
    //     cout<<"从服务器获取数据失败"<<endl;

    // }
   // buffer[bytes] = '\0';
   cout<<recover<<endl;
}
//查看好友申请消息
void FRI:: check_add_friends_request(TCP &client,LOGIN &login){
    //char buffer[BUFFER_SIZE];
    vector<FriendRequest> requests;
    string serialized_data;
    cout<<"当前操作：查看该用户id下的好友申请"<<endl;
    string type = "check_add_friends_request";
    string from_id = login.getuser_id();
    string to_id= "0" ;
    string message = "0";
    client.send_m(type,from_id,to_id,message);

    //int bytes = recv(client.data_socket,buffer,BUFFER_SIZE,0);
    string buffer;
    client.rec_m(type,buffer);
    // if(bytes < 0)
    // {
    //      buffer[bytes] = '\0';
    //     cout<<"从服务器获取数据失败"<<endl;
    //     return;
    // }
    
    if(buffer == "无好友申请")
    {
        cout<<buffer<<endl;
        return;
    }
   // buffer[bytes] = '\0';
   // cout<<buffer<<endl;
    serialized_data = buffer; 
    //vector<FriendRequest> requests;
    //memset(buffer, 0, sizeof(buffer));

    nlohmann::json j = nlohmann::json::parse(serialized_data);
        
    for (const auto& item : j) {
        FriendRequest req;
        req.from_id = item["from_id"];
        
        req.message = item["message"];
        req.status = item["status"];
        requests.push_back(req);
    }
    for (const auto& req : requests)
    {
        cout<<"接收到来自id"<<req.from_id<<"的好友申请"<<endl;
        cout<<":"<<req.message<<endl;
    }

    
        
        int command = -1;
        cout<<"请选择操作"<<endl<<"1.同意某好友申请"<<endl<<"2.拒绝某好友申请"<<endl<<"-1.退出"<<endl;
        cin>>command;
        // cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        // cin.clear();
         while(1)
        {
            if(command != 1 &&command != 2 &&command != -1)
            {
        cin>>command;

        cout<<"请输入1或者2"<<endl;
            }else {
                break;
            }
        }
        if(command == 1)
        {
            //char buffer2[BUFFER_SIZE]; 
            string buffer2;   
            cout<<"请输入想要通过的申请id"<<endl;
            string to_id;
            cin>>to_id;
            cout<<"请输入一条验证消息"<<endl;
            string message;
            cin>>message;
            type = "approve_friends_requests";
            client.send_m(type,from_id,to_id,message);

            // int byte = recv(client.data_socket,buffer2,BUFFER_SIZE,0);
            // if(byte < 0)
            // {
            //         cout<<"从服务器接收数据失败"<<endl;
            //         return;
            // }
            // buffer2[byte] = '\0';
            client.rec_m(type,buffer2);
            cout<<buffer2<<endl;
            //memset(buffer2, 0, sizeof(buffer2));

        }else if(command == 2){
          //  char buffer2[BUFFER_SIZE];
            string buffer2;
            cout<<"请输入想要拒绝的好友申请"<<endl;
            string to_id;
            cin>>to_id;
            string message = "0";
            type = "refuse_friends_requests";
            client.send_m(type,from_id,to_id,message);

            // int byte = recv(client.data_socket,buffer2,BUFFER_SIZE,0);
            // if(byte < 0)
            // {
            //         cout<<"从服务器接收数据失败"<<endl;
            // }
            // buffer2[byte] = '\0';
            client.rec_m(type,buffer2);
            cout<<buffer2<<endl;
            //  memset(buffer2, 0, sizeof(buffer2));
        }else if(command == -1)
        {
            string from_id = login.getuser_id();
       string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
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
   
   
    cin>>command;
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    cin.clear();
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
        group.delete_group(client,login); 
        break;
        case 3:
        cout<<"当前操作：加入群聊"<<endl;
        group.send_add_group(client,login); 
        break;
        
        case -1:
        cout<<"当前操作：退出"<<endl;
        string from_id = login.getuser_id();
       string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
        return;
    
        break;
    }   
    if(command == -1)
    {
        break;
    }
 }
}
void GRO::generate_group(TCP &client, LOGIN &login) {
    string type = "generate_group";
    string from_id = login.getuser_id();
    vector<string> to_ids;
    string message;
    //char buffer[BUFFER_SIZE];
   // string buffer;
    cout << "请输入想要添加到群聊中的好友id（输入-1结束）" << endl;
    cout << "好友应不少于两人" << endl;

    while(true) {
        string input;
        cin >> input;  // 读取输入
        
        if(input == "-1") {
            break;
        }
        
        to_ids.push_back(input + ":");  // 使用push_back安全添加元素
        
        // 显示当前已添加的好友数
        cout << "已添加 " << to_ids.size() << " 个好友" << endl;
        cout << "继续输入或输入-1结束: ";
    }

    if(to_ids.size() < 2) {
        cout << "用户小于三人，无法创建群聊" << endl;
        return;
    }
    cout<<"请输入群名"<<endl;
    cin>>message;
    // 构建to_id字符串（所有好友ID用逗号分隔）
    string to_id;
    for(size_t i = 0; i < to_ids.size(); ++i) {
        
        to_id += to_ids[i];
    }

    client.send_m(type, from_id, to_id, message);
  
    // int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    // if(bytes < 0) {
    //     cout << "发送好友申请后服务器无数据返回" << endl;
    // } else {
    //     buffer[bytes] = '\0';
    //     cout << buffer << endl;
    // }
    string buffer;
    client.rec_m(type,buffer);
    cout << buffer << endl;
}
void GRO::send_add_group(TCP &client, LOGIN &login) 
{
    string message;
    string from_id = login.getuser_id();
    string to_id ;
    string type = "send_add_group";

    cout<<"请输入要加入的群id"<<endl;
    cin>>to_id;
    cout<<"请输入一条验证消息"<<endl;
    cin>>message;
    //char buffer[BUFFER_SIZE];
    string buffer;
    client.send_m(type, from_id, to_id, message);
    client.rec_m(type,buffer);
    cout << buffer << endl;
    // int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    // if(bytes < 0) {
    //     cout << "发送群申请后服务器无数据返回" << endl;
    // } else {
    //     buffer[bytes] = '\0';
        // cout  << buffer << endl;
    // }
}
//删除群聊
void GRO::delete_group(TCP &client, LOGIN &login)
{
    string type = "delete_group";
    string from_id = login.getuser_id();
    //vector<string> to_ids;
    string message;
    //char buffer[BUFFER_SIZE];
    string buffer;
    string to_id = "0";
    cout<<"输入要删除的群号"<<endl;
    cin>>message;
   cout<<"确定要删除该群聊吗"<<endl;
   cout<<"1.确定"<<endl<<"2.点错了"<<endl;
   int command;
   cin>>command;
    while(1)
    {
        cin>>command;
        if(command == 1)
        {
            break;
        }else if(command == 2)
        {
            return;
        }
        cout<<"请输入1或者2"<<endl;
    }
 

    client.send_m(type, from_id, to_id, message);
    client.rec_m(type,buffer);
    cout << buffer << endl;
    // int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    // if(bytes < 0) {
    //     cout << "发送好友申请后服务器无数据返回" << endl;
    // } else {
    //     buffer[bytes] = '\0';
    //     cout << buffer << endl;
    // }
}
void GRO::see_all_group(TCP &client, LOGIN &login)
{
    string type = "get_all_my_group";
    string from_id = login.getuser_id();
    string message = "0";
    string to_id = "0";
    //char buffer[BUFFER_SIZE];
    string buffer;
    client.send_m(type, from_id, to_id, message);
    // int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    // if(bytes < 0) {
    //     cout << "发送好友申请后服务器无数据返回" << endl;
    // } else {
    //     buffer[bytes] = '\0';
    //     cout  << buffer << endl;
    // }
    client.rec_m(type,buffer);
    cout << buffer << endl;

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
            break;
    }
    m = 0;
    client.send_m(type,from_id,to_id,message);
    client.rec_m(type,buffer);
    //cout << buffer << endl;
        if(buffer == "该群不存在")
        {
            cout<<"该群不存在"<<endl;
            return;
        }else if(buffer == "你不在该群聊内")
        {
            cout <<"你不在该群聊内"<<endl;
        }else if(buffer =="群主")
        {
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
void GRO::open_group_owner(TCP &client,LOGIN &login,int &m,string group_id)
{

    int command = 0; 
    string b;
    main_page_owner(login.getuser_id(),login.getusername());
    cin>>command;
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    cin.clear();
    string type = "";
    string to_id = "";
    string from_id = "";
    string message= "";
    switch(command){
        case 1:
        cout<<"当前操作：在线聊天"<<endl;
        open_group_block(client,login,group_id);
        break;
        case 2:
        cout<<"当前操作：发送文件"<<endl;
        send_file_group(client,login,group_id);
        client.recv_server(client.data_socket);
        break;
         case 3:
        cout<<"当前操作：发送文件"<<endl;
        accept_file_group(client,login,group_id);
        client.recv_server(client.data_socket);
        break;
        case 4:
        cout<<"当前操作：退出群聊"<<endl;
        quit_group(client,login,group_id,m);
        client.recv_server(client.data_socket);
        if(m == 1)
        {
        string from_id = login.getuser_id();
       string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
        }
        break;
        case 5:
        cout<<"当前操作：查看群成员"<<endl;
        see_all_members(client,login,group_id);
         client.recv_server(client.data_socket);
        break;
        case 6:
        cout<<"当前操作：设置管理员"<<endl;
        manage_admin(client,login,group_id);
         client.recv_server(client.data_socket);
        break;
        case 7:
        cout<<"当前操作：管理成员"<<endl;
        manage_member(client,login,group_id);
         client.recv_server(client.data_socket);
        break;
        case 8:
        cout<<"当前操作：查看申请"<<endl;
        see_add_group(client,login,group_id);
         client.recv_server(client.data_socket);
        break;
        case -1:
        cout<<"当前操作：退出"<<endl;
        m = 1;
        string from_id = login.getuser_id();
       string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
        return;
    
        break;
    }   
    if(command == -1||heartbeat_received == false)
    {
        m =1 ;
    }
  
 
}
void GRO:: open_group_block(TCP &client,LOGIN &login,string group_id)
{
    
    string from_id,to_id,type,message;
    
    type = "group_open_block";
    from_id = login.getuser_id();
    to_id = group_id;
    message = "0";
    client.send_m(type,from_id,to_id,message);
    //历史聊天记录
    
    string buffer;
    client.rec_m(type,buffer);

    cout<<"-------------------------"<<endl;
    cout<<buffer<<endl;
    cout<<"-----以上为历史聊天记录-----"<<endl;
    
    group_active = true; 
    thread receive_thread(std::bind(&GRO::receive_group_message, this, std::ref(client), from_id, to_id)); 
    thread send_thread(std::bind(&GRO::send_message_group, this, std::ref(client), from_id, to_id)); 

    receive_thread.join();
    send_thread.join(); 
}
void GRO:: send_message_group(TCP &client,string from_id,string group_id)
{
    //cin.ignore(numeric_limits<streamsize>::max(), '\n');
   
        // 等待发送消息的信号
    while (group_active.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 避免占用过多CPU资源
        if (!group_active.load()) break;
    string message;
    string typ = "send_message_group";
    string to_id = group_id;
     if (!group_active.load()) break; 
    //cout<<"请输入消息"<<endl;
     if (!group_active.load()) break; 
    //cin.ignore(); 
    getline(cin, message);
    if(message == "-1")
    {
       group_active = false;
       //cout<<"退出"<<endl;
        break;
    }
    client.send_m(typ,from_id,to_id,message);
    }
   // cout<<"我结束了"<<endl;
}
void GRO::receive_group_message(TCP& client,string from_id,string group_id)
{

    while(group_active.load())
    {
    string type = "receive_group_new";
    string message = "0";
    string to_id = group_id;
    client.send_m(type,from_id,to_id,message);
    
    string buffer;
    client.rec_m(type,buffer);
    //cout<<"服务器无数据返回"<<endl;
    chat_active = false;
        if(buffer == "没有新消息")
        {
           // cout<<"该用户没有给你发新消息"<<endl;
             this_thread::sleep_for(chrono::milliseconds(1500));
             continue;
         // break;
        }else if(buffer == "你已被踢出群聊")
        {
             cout<<buffer<<endl;
            chat_active = false;
            //break;
        }
        
        cout<<"："<<buffer<<endl;
    }
 
//cout<<"我也结束了"<<endl;
}
void GRO::open_group_admin(TCP &client,LOGIN &login,int &m,string group_id)
{
    int command = 0; 
    string b;
    main_page_admin(login.getuser_id(),login.getusername());
   
    cin>>command;
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    cin.clear();
    string type = "";
    string to_id = "";
    string from_id = "";
    string message= "";
    switch(command){
        case 1:
        cout<<"当前操作：在线聊天"<<endl;
       open_group_block(client,login,group_id);
        break;
        case 2:
        cout<<"当前操作：发送文件"<<endl;
        send_file_group(client,login,group_id);
        break;
        case 3:
        cout<<"当前操作：接收文件"<<endl;
        accept_file_group(client,login,group_id);
        break;
        case 4:
        cout<<"当前操作：退出群聊"<<endl;
        quit_group(client,login,group_id,m);
        break;
        case 5:
        cout<<"当前操作：查看群成员"<<endl;
        see_all_members(client,login,group_id);
        break;
        case 6:
        cout<<"当前操作：管理成员"<<endl;
        manage_member(client,login,group_id);
        break;
        case -1:
        cout<<"当前操作：退出"<<endl;
        m =1;
        string from_id = login.getuser_id();
       string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
        return;
    
        break;
        if(m == 1)
        {
            break;
        }
    }   
   
}
void GRO::open_group_member(TCP &client,LOGIN &login,int &m,string group_id)
{

    int command = 0; 
    string b;
    main_page_admin(login.getuser_id(),login.getusername());
   
    cin>>command;
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    cin.clear();
    string type = "";
    string to_id = "";
    string from_id = "";
    string message= "";
    switch(command){
        case 1:
        cout<<"当前操作：在线聊天"<<endl;
       open_group_block(client,login,group_id);
        break;
        case 2:
        cout<<"当前操作：发送文件"<<endl;
        send_file_group(client,login,group_id);
        break;
        case 3:
        cout<<"当前操作：发送文件"<<endl;
        accept_file_group(client,login,group_id);
        break;
        case 4:
        cout<<"当前操作：退出群聊"<<endl;
        quit_group(client,login,group_id,m);
        break;
        case 5:
        cout<<"当前操作：查看群成员"<<endl;
        see_all_members(client,login,group_id);
    
        break;
        case -1:
        cout<<"当前操作：退出"<<endl;
        m =1;
        string from_id = login.getuser_id();
       string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
        return;
    
        break;
    }   

}
void GRO::manage_admin(TCP &client,LOGIN &login,string group_id)
{
    int command;
    cout<<"请输入操作"<<endl;
    cout<<"1.添加管理员"<<"2.删除管理员"<<endl;
    cin>>command;
        while(1)
        {
            if(command != 1 &&command != 2 &&command != -1)
            {
        cin>>command;
        cout<<"请输入1或者2"<<endl;
            }else {
                break;
            }
        }
        if(command == 1)
        {
     add_admin(client,login,group_id);
        }else if(command == 2)
        {
        delete_admin(client,login,group_id);
        }
}
void GRO::see_all_members(TCP &client,LOGIN &login,string group_id)
{
    string type = "see_all_member";
    string message = group_id;
    string from_id =  login.getuser_id();
    string to_id = "0";
     char buffer[BUFFER_SIZE];
    client.send_m(type,from_id,to_id,message);

    
    int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    if(bytes < 0) {
        cout << "服务器无数据返回" << endl;
    } else {
        buffer[bytes] = '\0';
        cout  << buffer << endl;
    }
}
void GRO::add_admin(TCP &client,LOGIN &login,string group_id)
{
    string type = "add_admin";
    string message = group_id;
    string from_id =  login.getuser_id();
    string to_id ;
    cout<<"请选择要把哪位用户添加为管理员"<<endl;
    cin>>to_id;
    //char buffer[BUFFER_SIZE];
    client.send_m(type,from_id,to_id,message);
    string buffer;
    client.rec_m(type,buffer);
    // int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    // if(bytes < 0) {
    //     cout << "服务器无数据返回" << endl;
    // } else {
    //     buffer[bytes] = '\0';
        cout  << buffer << endl;
    // }
}
void GRO::quit_group(TCP &client,LOGIN &login,string group_id,int &m)
{
    string type = "quit_group";
    string message = group_id;
    string from_id =  login.getuser_id();
    string to_id;
    //char buffer[BUFFER_SIZE];
    cout<<"确定要退出群聊吗TAT"<<endl;
    cout<<"1.确定"<<"2.点错啦> <"<<endl;
    int command;
    while(1)
    {
        cin>>command;
        if(command == 1)
        {
            break;
        }else if(command == 2)
        {
            return;
        }
        cout<<"请输入1或者2"<<endl;
    }
    client.send_m(type,from_id,to_id,message);
    string buffer;
    client.rec_m(type,buffer);
    // int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    // if(bytes < 0) {
    //     cout << "服务器无数据返回" << endl;
    // } else {
    //     buffer[bytes] = '\0';
        cout  << buffer << endl;
        m =1 ;
    // }
}
void GRO::delete_admin(TCP &client,LOGIN &login,string group_id)
{
    string type = "delete_admin";
    string message = group_id;
    string from_id =  login.getuser_id();
    string to_id ;
    cout<<"请选择要撤销的管理员id"<<endl;
    cin>>to_id;
    //char buffer[BUFFER_SIZE];
    client.send_m(type,from_id,to_id,message);
    string buffer;
    client.rec_m(type,buffer);
    // int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    // if(bytes < 0) {
    //     cout << "服务器无数据返回" << endl;
    // } else {
    //     buffer[bytes] = '\0';
        cout  << buffer << endl;
    // }
}
void GRO::manage_member(TCP &client,LOGIN &login,string group_id)
{
    int command;
    cout<<"请选择操作"<<endl<<"1.邀请成员进入群聊"<<"2.从群聊中移除成员"<<"-1.退出"<<endl;
    while(1)
    {
        cin>>command;
        if(command == 1 ||command == 2)
        {
            break;
        }else if(command == -1)
        {
            return;
        }
        cout<<"请输入1或者2"<<endl;
    }
    if(command == 1)
    {
        cout<<"请输入想要邀请的好友id"<<endl;
        string type ="add_member";
        string to_id;
        string from_id = login.getuser_id();
        string message = group_id;

        cin>>to_id;
        //char buffer[BUFFER_SIZE];
    client.send_m(type, from_id, to_id, message);
    //int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    // if(bytes < 0) {
    //     cout << "服务器无数据返回" << endl;
    // } else {
    //     buffer[bytes] = '\0';
    //     cout  << buffer << endl;
    // }
    string buffer;
    client.rec_m(type,buffer);
    }else if(command == 2)
    {
        
         cout<<"请输入想要移除的成员id"<<endl;
        string type ="delete_member";
        string from_id = login.getuser_id();
        string to_id ;
        string message = group_id;

        cin>>to_id;
        //char buffer[BUFFER_SIZE];
    client.send_m(type, from_id, to_id, message);
    string buffer;
    client.rec_m(type,buffer);
    // int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    // if(bytes < 0) {
    //     cout << "服务器无数据返回" << endl;
    // } else {
    //     buffer[bytes] = '\0';
    cout  << buffer << endl;
    // }
    }
}
void GRO::see_add_group(TCP &client,LOGIN &login,string group_id)
{
    string type = "see_add_group";
    string message = group_id;
    string from_id =  login.getuser_id();
    string to_id = "0";

   // char buffer[BUFFER_SIZE];
    client.send_m(type,from_id,to_id,message);
    string buffer;
    client.rec_m(type,buffer);
    //int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    // if(bytes < 0) {
    //     cout << "服务器无数据返回" << endl;
    // } else {
    //     buffer[bytes] = '\0';
        cout  << buffer << endl;
    // }

    int command = -1;
    cout<<"请选择操作"<<endl<<"1.同意某申请"<<endl<<"2.拒绝某申请"<<endl<<"-1.退出"<<endl;
    cin>>command;
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    cin.clear();
     while(1)
    {
        if(command != 1 &&command != 2 &&command != -1)
        {
        cin>>command;

        cout<<"请输入1或者2"<<endl;
            }else {
                break;
            }
        }
        if(command == 1)
        {
            //char buffer2[BUFFER_SIZE];    
            cout<<"请输入想要通过的申请id"<<endl;
            string to_id;
            cin>>to_id;
            string message = group_id;
            type = "approve_group_requests";
            client.send_m(type,from_id,to_id,message);
            string buffer2;
            client.rec_m(type,buffer2);
            // int byte = recv(client.data_socket,buffer2,BUFFER_SIZE,0);
            // if(byte < 0)
            // {
            //         cout<<"从服务器接收数据失败"<<endl;
            //         return;
            // }
            // buffer2[byte] = '\0';

            cout<<buffer2<<endl;
            //memset(buffer2, 0, sizeof(buffer2));

        }else if(command == 2){
            //char buffer2[BUFFER_SIZE];
            cout<<"请输入想要拒绝的申请"<<endl;
            string to_id;
            cin>>to_id;
            string message = group_id;
            type = "refuse_group_requests";
            client.send_m(type,from_id,to_id,message);
            string buffer2;
         client.rec_m(type,buffer2);
            // int byte = recv(client.data_socket,buffer2,BUFFER_SIZE,0);
            // if(byte < 0)
            // {
            //         cout<<"从服务器接收数据失败"<<endl;
            // }
            // buffer2[byte] = '\0';

            cout<<buffer2<<endl;
            // memset(buffer2, 0, sizeof(buffer2));
        }else if(command == -1)
        {
        string from_id = login.getuser_id();
        string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
        return;
        }
}
void GRO::send_file_group(TCP &client,LOGIN &login,string group_id)
{
    //暂停心跳监测
    client.pause_heartbeat();
    string filepath;
    string from_id = login.getuser_id();
    string type = "send_file_group";
    char buffer[BUFFER_SIZE];
    // cout<<"请输入好友id"<<endl;
    // cin>>to_id;
    string  to_id = group_id;
    cout<<"请输入文件名"<<endl;
    cin>>filepath;
    string message = filepath;
    ifstream test_file(filepath);
    if (!test_file) {
        cerr << "文件" << filepath << "不存在" << endl;
        close(client.transfer_socket);
        return;
        //continue;
    }
    test_file.close();
    client.send_m(type, from_id, to_id, message);
    client.connect_transfer_socket();
    string remote_path = filepath;
            
    ifstream file(filepath, ios::binary);
    if (!file) {
    cerr << " " << filepath << "该路径下文件不存在" << endl;
    close(client.transfer_socket);
    return;

    }
    while(!file.eof()) {
        file.read(buffer, BUFFER_SIZE);
        int bytes_read = file.gcount();
        if(send(client.transfer_socket, buffer, bytes_read, 0) != bytes_read) {
            cerr << "文件传输中断" << endl;
            break;
        }
    }
    file.close();
    shutdown(client.transfer_socket, SHUT_WR);    //  半关闭传输套接字(只关闭写端)
    char ack[16] = {0};
    int h = recv(client.data_socket, ack, sizeof(ack)-1, 0);
    if(h > 0) {
        ack[h] = '\0';
        if(string(ack) == "SUCCESS") {
            cout << "文件已成功上传" << endl;
        } else {
            cout << "上传失败: " << ack << endl;
        }
    } else {
        cout << "未收到服务器确认" << endl;
    }
    // 最后关闭transfer_socket
     client.resume_heartbeat();//恢复心跳监测
    close(client.transfer_socket);
}
void GRO::accept_file_group(TCP &client, LOGIN &login,string group_id) {
    // string to_id;
    // cout << "请输入好友ID: ";
    // cin >> to_id;
    //暂停心跳监测
    client.pause_heartbeat();
    const string download_dir = "download";
    string type = "accept_file_group";
    string from_id = login.getuser_id();
    string message = "0";
    string to_id = group_id;
    mkdir(download_dir.c_str(), 0777);

    client.send_m(type, from_id, to_id, message);

    char buffer[BUFFER_SIZE];
    int bytes = recv(client.data_socket, buffer, BUFFER_SIZE, 0);
    if (bytes <= 0) {
        cout << "服务器无响应" << endl;
        return;
    }
    buffer[bytes] = '\0';
    string response(buffer);
    if(response == "NO_FILES")
    {
        cout<<"没有文件"<<endl;
        return;
    }

    
    // 3. 显示文件列表并让用户选择
    cout << "可下载文件列表:\n" << buffer;
    cout << "请输入要下载的文件序号: ";
    
    int choice;
    cin >> choice;
    cin.ignore(); // 清除输入缓冲区
    
    string choice_str = to_string(choice);
    send(client.data_socket, choice_str.c_str(), choice_str.size(), 0);

    client.connect_transfer_socket();

    char filename_buf[256];
    int name_len = recv(client.transfer_socket, filename_buf, sizeof(filename_buf)-1, 0);
    if(name_len <= 0) {
        cerr << "接收文件名失败" << endl;
        close(client.transfer_socket);
        return;
    }
    filename_buf[name_len] = '\0';
    string filename = string(filename_buf);
    const char* ack_msg = "ACK";
    send(client.transfer_socket, ack_msg, strlen(ack_msg), 0);
    string save_path = download_dir + "/" + filename;
    ofstream outfile(save_path, ios::binary);
    if(!outfile) {
        cerr << "无法创建文件: " << save_path << endl;
        close(client.transfer_socket);
        return;
    }

    int total_received = 0;
    while(true) {
        int bytes_recv = recv(client.transfer_socket, buffer, BUFFER_SIZE, 0);
        if(bytes_recv <= 0) break;
        outfile.write(buffer, bytes_recv);
        total_received += bytes_recv;
    }
    outfile.close();

    if(total_received > 0) {
        cout << "文件下载成功: " << save_path << " (" << total_received << " bytes)" << endl;
    } else {
        remove(save_path.c_str());
        cout << "文件下载失败" << endl;
    }
    client.resume_heartbeat();//恢复心跳监测
    close(client.transfer_socket);
    
    
}
int main(int argc, char* argv[]){

if (argc != 2) {
    cerr << "Usage:./Client 10.30.0.142\n";
    return 1;
}
const char* SERVER_IP = argv[1]; // 从命令行参数获取服务器IP
//const int PORT = 8080;
TCP client(SERVER_IP);
LOGIN login;
FRI friends;
int command;
char buffer[BUFFER_SIZE];
//cout<<"请选择命令 1.注册 2.登陆 3.注销"<<endl;

//不知道为啥程序不正常进行的话就会一直死循环，先不写循环了
//好了
while(1)
{
sign_up_page();

cin>>command;
cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
cin.clear();
if (command == 1) {
const char *message = "register";  
send(client.getClientSocket(), message, strlen(message), 0);  
//login.register_user(client);
client.new_socket();
login.register_user(client);

} else if (command== 2) {
const char *message = "login";  
send(client.getClientSocket(), message, strlen(message), 0);
client.new_socket();
if(login.login_user(client))
{
  thread heartbeat_thread(&TCP::heartbeat, &client);
    heartbeat_thread.detach();
    
    friends.make_choice(client,login);
}
}else if(command == 3)
{
const char *message = "deregister"; 
send(client.getClientSocket(), message, strlen(message), 0);
client.new_socket();
login.deregister_user(client);
}else if(command == 4)
{
    const char* message = "quit";
    send(client.getClientSocket(), message, strlen(message), 0);
    
    break;
}
if(heartbeat_received == false)
    {
        break;
    }

}

return 0;

}
