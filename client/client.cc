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
TCP::TCP(const char* IP){
        SERVER_IP= IP;
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
    
    vector<char> send_buf(sizeof(uint32_t) + serialized_message.size());
    
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
    vector<char> buffer;       // 静态缓冲区保存未处理数据
    static size_t expected_len = 0;   // 当前期望接收的消息长度
    static bool reading_header = true; // 当前是否正在读取消息头

    // 先尝试读取消息头（4字节长度）
    if (reading_header && buffer.size() < sizeof(uint32_t)) {
        char header_buf[sizeof(uint32_t)];
        ssize_t bytes = recv(data_socket, header_buf + buffer.size(), 
                           sizeof(uint32_t) - buffer.size(), 0);
        
        if (bytes <= 0) {
            if (bytes == 0) {
                heartbeat_received = false;
                cout << "与服务器连接已断开" << endl;
                close(data_socket);
            }
            return false;
        }
        
        buffer.insert(buffer.end(), header_buf, header_buf + bytes);
      
        if (buffer.size() < sizeof(uint32_t)) {
            return false;
        }
        
        uint32_t len;
        memcpy(&len, buffer.data(), sizeof(len));
        expected_len = ntohl(len);
        buffer.clear(); 
        reading_header = false;
    }

    // 读取消息体
    if (!reading_header && buffer.size() < expected_len) {
        size_t remaining = expected_len - buffer.size();
        vector<char> temp_buf(min(remaining, (size_t)4096)); // 每次最多读4K
        
        ssize_t bytes = recv(data_socket, temp_buf.data(), temp_buf.size(), 0);
        if (bytes <= 0) {
            if (bytes == 0) {
                heartbeat_received = false;
                cout << "与服务器连接已断开" << endl;
                close(data_socket);
            }
            return false;
        }
        
        buffer.insert(buffer.end(), temp_buf.begin(), temp_buf.begin() + bytes);
        
        // 如果还没收完完整消息，返回继续接收
        if (buffer.size() < expected_len) {
            return false;
        }

    }

    // 完整消息已接收，开始解析
    try {
         if(buffer.empty()) {
            throw runtime_error("接收到的数据为空");
        }

        auto parsed = json::parse(string(buffer.begin(), buffer.end()));
        type = parsed["type"];
        message = parsed["message"];
       // cout << "接收到数据: " << message << endl;
        if (buffer.size() > expected_len) {
            vector<char> remaining(buffer.begin() + expected_len, buffer.end());
            buffer = std::move(remaining);
        } else {
            buffer.clear();
        }
        expected_len = 0;
        reading_header = true;
        
        return true;
    } catch (const json::parse_error& e) {
        buffer.clear();
        expected_len = 0;
        reading_header = true;
        throw runtime_error("JSON解析失败: " + string(e.what()));
    }
}
void TCP:: heartbeat()
{
    while(heartbeat_received == true)
    {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, []{ return !is_paused; });

    string type = "heart";
    string message = "ping";
    string to_id = "0";
    string from_id = "0";
    send_m(type,from_id,to_id,message);

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
  //  cout<<"接收到服务器发来的端口"<<data_port<<endl;

    this->data_socket = socket(AF_INET,SOCK_STREAM,0);
    if(this->data_socket == -1)
    {
        cerr<<"failed  to create data socket" <<endl;
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data_port);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);;

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
   // cout << "Connected to data port: " << data_port << endl;
     
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
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP); // 根据实际情况修改

    if(connect(transfer_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "连接传输套接字失败" << endl;
        close(transfer_socket);
        return;
    }

    cout << "已连接到传输端口：" << transfer_port << endl;
}
void TCP::connect_notice_socket() {
    int notice_port;
    
    // 接收服务器发来的传输端口号
    if(recv(data_socket, &notice_port, sizeof(notice_port), 0) <= 0) {
        cerr << "接收通知端口失败" << endl;
        return;
    }

    // 创建通知套接字
    notice_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(notice_socket == -1) {
        cerr << "传输套接字创建失败" << endl;
        return;
    }

    // 连接服务器
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(notice_port);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP); // 根据实际情况修改

    if(connect(notice_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "连接传输套接字失败" << endl;
        close(notice_socket);
        return;
    }
    
    notice_thread_running_ = true;
    notice_thread_ = std::thread(&TCP::notice_receiver_thread, this);
    notice_thread_.detach(); 
    //cout << "已连接到传输端口：" << transfer_port << endl;
}
//login类关于登陆注册注销的函数

void LOGIN::register_user(TCP& client) {
    string username;
    string password;
    
    
    while (true) {
        //cout << "请输入用户名（不能包含空格）: ";
        PRINT_GREEN("请输入用户名（不能包含空格）: ");
        getline(cin, username); 
        
     
        username.erase(0, username.find_first_not_of(" \t\n\r"));
        username.erase(username.find_last_not_of(" \t\n\r") + 1);
        
       
        if (username.empty()) {
            PRINT_RED("用户名不能为空！" );
        } else if (username.find(' ') != string::npos) {
            PRINT_RED( "用户名不能包含空格！" );
        } else {
            break;  
        }
    }
    send(client.data_socket, username.c_str(), username.length(), 0);
    
 
    while (true) {
        PRINT_GREEN("请输入密码（至少6位）: ");
        getline(cin, password);
        
        password.erase(0, password.find_first_not_of(" \t\n\r"));
        password.erase(password.find_last_not_of(" \t\n\r") + 1);
        
        if (password.size() < 6) {
            PRINT_RED( "密码必须大于六位！" );
        } else if (password.find(' ') != string::npos) {
            PRINT_RED( "密码不能包含空格！" );
        } else {
            break;
        }
    }
    send(client.data_socket, password.c_str(), password.length(), 0);

 
    char buffer[1024] = {0};
    int bytes_received = recv(client.data_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        cout << buffer << endl;
    } else {
        cout << "接收数据失败!" << endl;
    }
}
bool LOGIN::login_user(TCP& client){
    string user_id;
    string password;
    char buffer[1024] = {0};
   
    PRINT_GREEN("请输入账号");
    cin>>user_id;
    send(client.data_socket, user_id.c_str(), user_id.length(), 0);  
    
    PRINT_GREEN("请输入密码");
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
        string message,type;
        rec_m(type,message);

    //     char b[1024];
    //     int by = recv(data_socket,b,sizeof(b),0);
    // if(by > 0 )
    // {
    //     b[by] = '\0';
         cout<<"*************************************"<<endl;
         cout<<message<<endl;
         cout<<"*************************************"<<endl;
    // }else{
    //     cout<<"无数据"<<endl;
    // }
    }
   
void LOGIN::deregister_user(TCP& client){
    string user_id;
    string password;
    string username;
    char buffer[1024] = {0};
    //cout<<"请输入用户ID"<<endl;
    PRINT_GREEN("请输入用户ID");
    cin>>user_id;
    send(client.data_socket, user_id.c_str(), user_id.length(), 0);  
    //cout<<"请输入密码"<<endl;
    PRINT_GREEN("请输入密码");
    cin>>password;
    
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
void TCP::notice_receiver_thread() {
        char buffer[1024];
        while (notice_thread_running_) {
            ssize_t bytes = recv(notice_socket, buffer, sizeof(buffer), 0);
            if (bytes <= 0) {
                if (bytes == 0) {
                    std::cerr << "通知服务器连接已关闭" << std::endl;
                } else {
                    std::cerr << "通知接收错误: " << strerror(errno) << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }else{
                cout<<buffer<<endl;
            }

            // 处理接收到的通知
            //std::string notice(buffer, bytes);
           // handle_notice(notice);
        }
    }
  //登陆成功后进入选择
void FRI:: make_choice(TCP &client,LOGIN &login){

    //开始接收线程
    client.connect_notice_socket();
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
      //  client.recv_server(client.data_socket);
        break;
        case 2:
        cout<<"当前操作：管理好友"<<endl;
        manage_friends(client,login);
        // client.recv_server(client.data_socket);
        break;
        case 3:
        cout<<"当前操作:查看好友列表"<<endl;
        see_all_friends(client,login.getuser_id());
       // client.recv_server(client.data_socket);
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
         close(client.notice_socket);
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
        cout<<buffer<<endl;
         type = "nothing";
        message = "0"; 
        client.send_m(type,from_id,to_id,message);
        return;
     }
    
    while(1)
    {
    int a;
    print_block();
    //cin.ignore();
    cin>>a;
    switch(a)
    {
        case 1:
        cout<<"当前操作：发送消息"<<endl;
        open_block(client,login,to_id);
        //client.recv_server(client.data_socket);
        break;
        case 2:
        cout<<"当前操作：发送文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        send_file_to_friends(client, login, to_id);
       // client.recv_server(client.data_socket);
        break;
        case 3:
        cout<<"当前操作：接收文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        accept_file(client,login,to_id);
        //client.recv_server(client.data_socket);
        break;
        case -1:
        break;
        default:  
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
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
    client.resume_heartbeat();
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
    
    // 获取文件总大小
    streampos file_size = test_file.tellg();
    test_file.close();

    this->is_transfer = true;
    client.send_m("send_file", from_id, to_id, filepath);
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
        cout<<"准备接收ack"<<endl;
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
        cout<<"接收ack"<<endl;
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
        //heartbeat_received = false;
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
    string message;
    string type = "send_message_no";
    if (!chat_active.load()) break;  
    getline(cin, message);
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
    string type = "new_message";
    string message = "0";
    
    string buffer; 
    if (!chat_active.load()) break; 
    client.rec_m(type,buffer);
    //cout<<"111"<<endl;
    if (!chat_active.load()) break; 
    //buffer =拉黑以及buffer = 退出聊天
    if(buffer == "quit")
    {
         chat_active = false;
         break;
    }
    cout<<"\033[1;36m["<<type<<"]\033[0m"<<buffer<<endl;
       // cout<<buffer<<endl;
        
 }
//cout<<"我也结束了"<<endl;
}
void FRI:: open_block(TCP &client,LOGIN &login,string to_id)
{
    //暂停心跳监测
    // client.pause_heartbeat();
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
    
    
    chat_active = true; 
    thread receive_thread(std::bind(&FRI::receive_log, this, std::ref(client), from_id, to_id)); 
    thread send_thread(std::bind(&FRI::send_message_no, this, std::ref(client), from_id, to_id)); 

    receive_thread.join();
    send_thread.join();

    type = "nothing";
    message = "0"; 
    client.send_m(type,from_id,to_id,message);
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
        cout<<"还没有好友申请哦"<<endl;
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
    for (const auto& req : requests)
    {
        cout<<req.from_id<<":"<<req.message<<endl;
    }

        int command = -1;
        cout<<"1].同意某好友申请"<<endl<<"2].拒绝某好友申请"<<endl<<"-1].退出"<<endl;
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
   // cout << buffer << endl;
        if(buffer == "该群不存在")
        {
            cout<<"该群不存在"<<endl;
             string from_id = login.getuser_id();
       string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
            return;
        }else if(buffer == "你不在该群聊内")
        {
            cout <<"你不在该群聊内"<<endl;
             string from_id = login.getuser_id();
       string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
            return;
        }else if(buffer =="群主")
        {
             //cout<<"群主"<<endl;
            group.open_group_owner(client,login,m,group_id);
           ;
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
    //cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    //cin.clear();
    string type = "";
    string to_id = "";
    string from_id = "";
    string message= "";
    switch(command){
        
        case 1:
        cout<<"当前操作：在线聊天"<<endl;
        open_group_block(client,login,group_id);
        client.recv_server(client.data_socket);
        break;
        case 2:
        cout<<"当前操作：发送文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        send_file_group(client,login,group_id);
        client.recv_server(client.data_socket);
        break;
         case 3:
        cout<<"当前操作：发送文件"<<endl;
        accept_file_group(client,login,group_id);
        client.recv_server(client.data_socket);
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        }  
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
    if(command == -1)
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
    type = "nothing";
    message = "0"; 
    client.send_m(type,from_id,to_id,message);
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
        client.recv_server(client.data_socket);
        break;
        case 2:
        cout<<"当前操作：发送文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        send_file_group(client,login,group_id);
        client.recv_server(client.data_socket);
        break;
        case 3:
        cout<<"当前操作：接收文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        accept_file_group(client,login,group_id);
        
        client.recv_server(client.data_socket);

        break;
        case 4:
        cout<<"当前操作：退出群聊"<<endl;
        quit_group(client,login,group_id,m);
         client.recv_server(client.data_socket);
        break;
        case 5:
        cout<<"当前操作：查看群成员"<<endl;
        see_all_members(client,login,group_id);
         client.recv_server(client.data_socket);
        break;
        case 6:
        cout<<"当前操作：管理成员"<<endl;
        manage_member(client,login,group_id);
         client.recv_server(client.data_socket);
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
        client.recv_server(client.data_socket);
        break;
        case 2:
        cout<<"当前操作：发送文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        send_file_group(client,login,group_id);
        client.recv_server(client.data_socket);
        break;
        case 3:
        cout<<"当前操作：接收文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        accept_file_group(client,login,group_id);
        client.recv_server(client.data_socket);
        break;
        case 4:
        cout<<"当前操作：退出群聊"<<endl;
        quit_group(client,login,group_id,m);
         client.recv_server(client.data_socket);
        break;
        case 5:
        cout<<"当前操作：查看群成员"<<endl;
        see_all_members(client,login,group_id);
         client.recv_server(client.data_socket);
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
    client.send_m(type,from_id,to_id,message);

    string buffer;
    client.rec_m(type,buffer);
    cout  << buffer << endl;
    
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
            string from_id = login.getuser_id();
       string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
        return;
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
        
    client.send_m(type, from_id, to_id, message);
   
    string buffer;
    client.rec_m(type,buffer);
    cout<<buffer<<endl;
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
    string filepath;
    string from_id = login.getuser_id();

    cout << "请输入文件名: ";
    cin >> filepath;

    // 检查文件是否存在
    ifstream test_file(filepath);
    if (!test_file) {
        cerr << "\033[31m文件 " << filepath << " 不存在\033[0m" << endl;
       string type = "nothing";
        string to_id = "0";
        string message = "0"; 
        client.send_m(type,from_id,to_id,message);
        return;
    }
    test_file.close();
  string to_id = group_id;
  client.send_m("send_file_group", from_id, to_id, filepath);
 client.connect_transfer_socket();

    this->is_transfer = true;
    std::thread([this,&client, from_id, to_id, filepath]() {
         cout<<"进入发送文件线程"<<endl;
          //  client.connect_transfer_socket();

            
            ifstream file(filepath, ios::binary);
            if (!file) {
                cerr << "\033[31m文件打开失败: " << filepath << "\033[0m" << endl;
                return;
            }
            cout<<"开始发送文件"<<endl;
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
            
            int h = recv(client.transfer_socket, ack, sizeof(ack) - 1, 0);
            if (h > 0) {
             ack[h] = '\0';
             string a = string(ack);
                   if(a == "SUCCESS")
                   {
                    cout<<"文件发送成功"<<endl;
                   }else{
                    cout<<ack<<endl;
                   }
            }
            
        close(client.transfer_socket);
        this->is_transfer = false;
      //  client.resume_heartbeat();
    }).detach(); 
}
void GRO::accept_file_group(TCP &client, LOGIN &login,string group_id) {
    
    const string download_dir = "../download";
    string type = "accept_file_group";
    string from_id = login.getuser_id();
    string message = "0";
    string to_id = group_id;
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
        cout<<"没有文件"<<endl;
        return;
    }

    
    // 3. 显示文件列表并让用户选择
    cout << "可下载文件列表:\n" << buffer1;
    cout << "请输入要下载的文件序号: ";
    
    int choice;
    cin >> choice;
    cin.ignore(); // 清除输入缓冲区
    
    string choice_str = to_string(choice);
    send(client.data_socket, choice_str.c_str(), choice_str.size(), 0);

    client.connect_transfer_socket();
     this->is_transfer = true;
    thread([this,&client,download_dir](){
    char filename_buf[256];
    char buffer[BUFFER_SIZE];
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
   // client.resume_heartbeat();//恢复心跳监测
    close(client.transfer_socket);
   // cout<<"111"<<endl;
   this->is_transfer = false;
    }).detach(); 
    
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
   heartbeat_received = false;
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
    heartbeat_received = false;
   
    break;
}else{
    cout<<"无效的命令，重新输入吧"<<endl;
    cin.clear();
}
// if(heartbeat_received == false)
//     {
//         break;
//     }

}

return 0;

}
