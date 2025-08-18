#include "client.hpp"
std::atomic<bool> chat_active{true};
std::atomic<bool> group_active{true};
std::atomic<bool> heartbeat_received{true};
std::condition_variable cv;
bool is_paused = false;  // 确保与声明一致
std::mutex mtx;
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
void TCP::send_m(const string& type,const string& from_sb,const string& to_sb, const string& message)
{
    if(data_socket < 0||type.empty()||from_sb.empty() || to_sb.empty())
    {
        return;
    }
    nlohmann::json j;
    j["type"] = type;
    j["message"] = message;
    j["to_id"] = to_sb;
    j["from_id"] = from_sb;
    string msg = j.dump();
    uint32_t msg_len = htonl(msg.size());
    string ext_len(4 + msg.size(), '\0');
    memcpy(ext_len.data(), &msg_len, 4);
    memcpy(ext_len.data() + 4, msg.data(), msg.size());

    int count = ext_len.size();
    const char *buf = ext_len.c_str();
    while (count > 0)
    {
            int len = send(data_socket, buf, count, 0);
            if (len < 0)
            {
                if (errno == EAGAIN)
                continue;
            }
            else if (len == 0)
                return;
            count -= len;
            buf += len;
        }
}
int TCP::readn(int data_socket,int size,char *buf)
{
    int count = size;
    char *a = buf;
    while(count > 0)
    {
        int len = recv(data_socket,a, count, 0);
        //cout<<"我在接受"<<endl;
        if (len == -1)
            {
                if (errno == EAGAIN)
                    cout << " 继续尝试读取" << endl;
                return -1;
            }else if (len == 0)
            return size - count; 
            a += len;
            count -= len;
    }
    return size;
}
bool TCP::rec_m(string &type, string &message)
{
    uint32_t len = 0;
    if(readn(data_socket,4,(char*)&len) != 4)
    {
        return false;
    }
    len = ntohl(len);
    vector<char> buf(len);
    int ret = readn(data_socket,len,buf.data());
    if(ret != len)
    {
        return false;
    }
    if(ret == 0)
    {
        cout <<"连接断开"<<endl;
        close(this->data_socket);
        close(this->notice_socket);
        close(this->heart_socket);
    }
    
    string json_str;
    json_str.assign(buf.begin(), buf.end());
   // cout<<"收到"<<json_str<<endl;
    try {
    nlohmann::json j = nlohmann::json::parse(json_str);
        
        
    type = j["type"].get<string>();
    message = j["message"].get<string>();
    //cout<<"解析了type"<<type<<"message"<<message;
     }catch (const nlohmann::json::exception& e) {
        cerr << "JSON解析错误: " << e.what() << endl;
        cerr << "原始数据: " << json_str << endl;
        return false;
    }
        
    return true;
}

// 辅助函数：二进制数据转十六进制输出
string hexdump(const char* data, size_t len) {
    static const char* hex = "0123456789ABCDEF";
    string output;
    for (size_t i = 0; i < len; ++i) {
        output += hex[(data[i] >> 4) & 0xF];
        output += hex[data[i] & 0xF];
        output += ' ';
        if ((i + 1) % 16 == 0) output += '\n';
    }
    return output;
}
void TCP:: heartbeat()
{
    while(heartbeat_received == true)
    {
    std::this_thread::sleep_for(std::chrono::seconds(30));
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, []{ return !is_paused; });

    // string type = "heart";
    string message = "ping";
     send(heart_socket, message.c_str(), message.size(), 0); 
    // string to_id = "0";
    // string from_id = "0";
    // send_m(type,from_id,to_id,message);

    }
}

bool TCP::new_heartbeat_socket() 
{
    int heart_port;
    if (recv(data_socket, &heart_port, sizeof(heart_port), 0) <= 0) {
        cerr << "[Heartbeat] Failed to receive heartbeat port" << endl;
        return false;
    }

    heart_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (heart_socket == -1) {
        cerr << "[Heartbeat] Failed to create socket" << endl;
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(heart_port);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(heart_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "[Heartbeat] Failed to connect to port " << heart_port << endl;
        close(heart_socket);
        return false;
    }

    //cout << "[Heartbeat] Connected to port: " << heart_port << endl;
    heartbeat_received = true;
    thread(&TCP::heartbeat, this).detach();

    return true;
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
    // this->heart_socket = socket(AF_INET,SOCK_STREAM,0);
    // if(connect(this->heart_socket,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1)
    // {
    //     cerr << "Failed to connect to data port." << endl;
    //         close(this->heart_socket);
    //         return;
    // }
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
bool LOGIN::login_user(TCP& client) {
    string user_id;
    string password;
    char buffer[1024] = {0};

    // 1. 输入账号
    PRINT_GREEN("请输入账号");
    cin >> user_id;
    if(send(client.data_socket, user_id.c_str(), user_id.length(), 0) <= 0) {
        cerr << "账号发送失败" << endl;
        return false;
    }

    // 2. 输入密码
    PRINT_GREEN("请输入密码");
    cin >> password;
    if(send(client.data_socket, password.c_str(), password.length(), 0) <= 0) {
        cerr << "密码发送失败" << endl;
        return false;
    }

    // 3. 接收登录响应
    int bytes_received = recv(client.data_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        cerr << "接收登录响应失败" << endl;
        return false;
    }
    buffer[bytes_received] = '\0';
    string response(buffer, bytes_received);

    // 4. 处理服务器响应
    if (response == "用户不存在!") {
        cerr << "错误: " << response << endl;
        return false;
    }
    else if (response == "密码错误TAT") {
        cerr << "错误: " << response << endl;
        return false;
    }
    else if (response == "请勿重复登陆") {
        cerr << "错误: " << response << endl;
        return false;
    }
    else if (response == "success") {
        // 发送确认
        const char* ack = "ok";
        if(send(client.data_socket, ack, strlen(ack), 0) <= 0) {
            cerr << "确认发送失败" << endl;
            return false;
        }
        char buffer2[1024];
        // 接收用户名
        int bytes_received2 = recv(client.data_socket, buffer2, sizeof(buffer2) - 1, 0);
        if (bytes_received2 <= 0) {
            cerr << "接收用户名失败" << endl;
            return false;
        }
        buffer2[bytes_received2] = '\0';
       // cout<<buffer2<<endl;
        this->username = string(buffer2, bytes_received2);
        this->user_id = user_id;
        this->passward = password;

        //cout << "登录成功！欢迎, " << this->username << endl;
        return true;
    }
    else {
        cerr << "未知响应: " << response << endl;
        return false;
    }
}
void TCP::recv_server_(int data_socket)
    {
    string message,type;
    rec_m(type,message);
    
    
    cout<<"***************离线消息***************"<<endl;
    cout<<message<<endl;
    cout<<"*************************************"<<endl;
   
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
                    cerr << "通知服务器连接已关闭" << endl;
                    close(data_socket);
                    close(notice_socket);
                    close(heart_socket);
                    break;
                } else {
                    // std::cerr << "通知接收错误: " << strerror(errno) << std::endl;
                    close(data_socket);
                    close(notice_socket);
                    close(heart_socket);
                }
                continue;
            }else{
                cout<<buffer<<endl;
                memset(buffer, 0, sizeof(buffer));  
            }

            // 处理接收到的通知
            //std::string notice(buffer, bytes);
           // handle_notice(notice);
        }
    }
