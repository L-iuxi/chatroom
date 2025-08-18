#include "server.hpp"
#include "redis.hpp" 
std::mutex redis_mutex;
std::mutex g_chat_pairs_mutex;
unordered_map<string,string> chat_pairs;
unordered_map<string,string> group_pairs;
unordered_map<string, int> notice_user;
void MSG::send_m(int data_socket,string type,string message)
{
    if(message.empty()||data_socket < 0||type.empty())
    {
        return;
    }
    nlohmann::json j;
    j["type"] = type;
    j["message"] = message;

    string msg = j.dump();
    cout<<"准备发送"<<msg<<endl;
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
            }else if( len == -1)
            {
                if(errno == EPIPE||errno == ECONNRESET)
                {
                    close(data_socket);
                    return;
                }
            }
            else if (len == 0)
                return;
            count -= len;
            buf += len;
        }
}
int MSG::readn(int data_socket,int size,char *buf)
{
    int count = size;
    char *a = buf;
    while(count > 0)
    {
        int len = recv(data_socket,a, count, 0);
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
bool MSG::rec_m(string &type, string &from_id, string &to_id, string &message, int data_socket)
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
    }
    string json_str;
    json_str.assign(buf.begin(), buf.end());
    try {
    nlohmann::json j = nlohmann::json::parse(json_str); 
    type = j["type"].get<string>();
    message = j["message"].get<string>();
    from_id = j["from_id"].get<string>();
    to_id = j["to_id"].get<string>();
    cout<<"toid == "<<to_id;
    cout<<"from_id == "<<from_id;
    cout<<"type=="<<type;
    if(type.empty()||from_id.empty()||to_id.empty())
    {
        type.clear();
        return true;
    }
     }catch (const nlohmann::json::exception& e) {
        cerr << "JSON解析错误: " << e.what() << endl;
        cerr << "原始数据: " << json_str << endl;
        return false;
    }
    
    
    return true;
}

int TCP::new_heartbeat_socket(int data_socket) 
{
 
    int heartbeat_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (heartbeat_socket == -1) {
        cerr << "[Heartbeat] 心跳套接字创建失败" << endl;
        return -1;
    }

    int heartbeat_port = generate_port();

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(heartbeat_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int reuse = 1;
   if (setsockopt(heartbeat_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
    cerr << "[Heartbeat] 设置 SO_REUSEADDR 失败" << endl;
    close(heartbeat_socket);
    return -1;
   }

    if (bind(heartbeat_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "[Heartbeat] 心跳套接字绑定端口 " << heartbeat_port << " 失败" << endl;
        close(heartbeat_socket);
        return -1;
    }

    if (listen(heartbeat_socket, 1) == -1) {
        cerr << "[Heartbeat] 心跳套接字监听失败" << endl;
        close(heartbeat_socket);
        return -1;
    }

    if (send(data_socket, &heartbeat_port, sizeof(heartbeat_port), 0) <= 0) {
        cerr << "[Heartbeat] 心跳端口发送失败" << endl;
        close(heartbeat_socket);
        return -1;
    }

    int heartbeat_conn = accept(heartbeat_socket, nullptr, nullptr);
    if (heartbeat_conn == -1) {
        cerr << "[Heartbeat] 心跳连接接受失败" << endl;
        close(heartbeat_socket);
        return -1;
    }

    cout << "[Heartbeat] 心跳套接字已建立，端口：" << heartbeat_port << endl;
    return heartbeat_conn;
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

    int reuse = 1;
if (setsockopt(notice_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
    cerr << "[Heartbeat] 设置 SO_REUSEADDR 失败" << endl;
    close(notice_socket);
    return -1;
}
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
    int random_number = std::rand() % 90 + 10;

    return random_number;
 }

//创建服务器套接字
TCP::TCP():pool(10) {
    startHeartbeatMonitor(); 
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
            // string command = msg.rec_m(client_socket);
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
            //redis_data.c = cn;
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
            MSG msg;
            int heart_socket = new_heartbeat_socket(data_socket);
            //addSocketPair(data_socket, heart_socket); 
            //std::thread(&TCP::handleHeartbeat, this, heart_socket, data_socket).detach();
            this->make_choice(data_socket,msg);

           // stopHeartbeatMonitor();
            remove_user_socket(find_user_id(data_socket));
            remove_user(data_socket);
             close(data_socket);//进入登陆后的选项  
             close(heart_socket);
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
int TCP::generate_port() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distr(PASV_PORT_MIN, PASV_PORT_MAX);

    while (true) {
        int port = distr(gen);  // 生成随机端口
        
        // 检查端口是否可用
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            cerr << "创建套接字失败: " << strerror(errno) << endl;
            return -1;  // 返回错误
        }

        // 设置 SO_REUSEADDR 以避免 TIME_WAIT 状态的影响
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // 尝试绑定端口
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            // 绑定成功，端口可用
            close(sock);
            return port;
        } else {
            // 绑定失败（端口可能被占用）
            close(sock);
            if (errno != EADDRINUSE) {
                cerr << "绑定端口失败: " << strerror(errno) << endl;
                return -1;  // 非"端口占用"错误，直接返回失败
            }
         
        }
    }
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
  // 服务器开始时启动心跳监测线程
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

    // 服务器关闭时停止心跳监测
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
                remove_user(it->first);
                close(it->first);
                // 清理用户数据
                it = client_last_heartbeat_.erase(it);
            } else {
                ++it;
            }
        }
    }

//更新在线时间
void TCP::updateHeartbeat(int data_socket) {
    std::unique_lock<std::mutex> lock(heartbeat_mutex_);
    client_last_heartbeat_[data_socket] = std::chrono::steady_clock::now();
    cout << "更新客户端 " << data_socket << " 心跳时间" << endl;
}

//关闭所有在线的客户端数据套接字
TCP:: ~TCP(){
     std::cerr << "TCP对象被析构，socket=" << server_socket << std::endl;
        for(auto& user : logged_users){
            int close_socket = user.first;
            close(close_socket);
            cout<<"已关闭套接字"<<close_socket<<endl;
        }
        stopHeartbeatMonitor();
        close(server_socket);
    }

//登陆成功创建完noticesocket之后加入表
bool TCP::add_notice_socket(string userid, int sockfd) {
        auto [it, inserted] = notice_user.emplace(userid, sockfd);
        return inserted;
    }

//下线之后从表里面移除通知套接字
void TCP:: remove_user_socket(string userid) {
        notice_user.erase(userid);
    }

// 通过用户ID获取套接字
int TCP:: get_noticesocket_by_userid(string userid) {
        auto it = notice_user.find(userid);
        return (it != notice_user.end()) ? it->second : -1;
    }


// 通过套接字获取用户ID（需要反向映射）
string TCP::get_userid_by_noticesocket(int sockfd) {
        for (const auto& [uid, fd] : notice_user) {
            if (fd == sockfd) {
                return uid;
            }
        }
        return "";
    }
//发送实时消息
void TCP::send_notice(string from_id,string to_id,string message,DATA &redis_data)
{
    int socket =get_noticesocket_by_userid(to_id);
    if(socket == -1)
    {
        redis_data.add_unread_message(to_id,message);
        cout<<"该用户不在线,已经存为离线消息"<<endl;
        return;
    }
    if(send(socket,message.c_str(),message.length(),0) < 0)
    {
         if (errno == EPIPE) {
            close(socket);
            cerr << "Client disconnected, cannot send notice." << endl;
            return; 
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
    std::string message = "success";

     size_t byte_len = message.size();
     send(data_socket, message.c_str(), message.size(), 0);
    
     char buffer3[1024];
     int bytes_rec3= recv(data_socket, buffer3, 1024, 0);
    if(bytes_rec3 < 0)
    {
        cout<<"接收失败"<<endl;
         return false;
    }
    else{
        buffer3[bytes_rec3] = '\0'; 
        if(strcmp(buffer3, "ok") == 0)
        {
            cout<<"ok"<<endl;
     
 const char* msg = username.c_str();  // 获取C风格字符串
 send(data_socket, msg, strlen(msg), 0);  // 发送
        }
    }
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

//操作
void TCP::make_choice(int data_socket,MSG &msg){
    DATA redis_data;
    redis_data.data_create();
    FRI friends(this);
    GRO group(this);
    //建立通知套接字
    int notice_socket = new_notice_socket(data_socket);
    add_notice_socket(find_user_id(data_socket),notice_socket);

    string type,to_id,from_id,message;
    recived_messages(redis_data,find_user_id(data_socket),data_socket,msg);
    cout<<"已发送离线消息"<<endl;
    while(1)
    {
        if(!msg.rec_m(type,from_id,to_id,message,data_socket))
        {
            cout<<"客户端已断开"<<endl;
            break;
        }
        if(type == "send_add_friends_request")
        {
            cout<<"接收到命令：添加好友"<<endl;
            friends.send_add_request(*this,data_socket,to_id,from_id,message,redis_data,msg);
        }else if(type  == "delete_friend")
        {
            cout<<"接收到命令:删除好友"<<endl;
            friends.delete_friend(*this,data_socket,to_id,from_id,redis_data,msg);
        }else if(type  == "quit")
        {
            cout<<"接收到命令：退出登陆"<<endl;
           // running_ = false;
            // remove_user_socket(find_user_id(data_socket));
            // remove_user(data_socket);
           // close(data_socket);
            break;
        }else if(type == "check_add_friends_request")
        {
            cout<<"接收到命令：查看该用户收到的好友申请"<<endl;
            int m = 0;
            friends.check_add_friends_request(*this,data_socket,from_id,redis_data,m,msg);
        }else if(type == "approve_friends_requests")
        {
            cout<<"接收到命令：通过好友申请"<<endl;
            friends.add_friend(*this,data_socket,to_id,from_id,redis_data,msg);
             //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "see_all_friends")
        {
            cout<<"接收到命令:查看所有好友"<<endl;
            friends.see_all_friends(*this,data_socket,from_id,redis_data,msg);
           // recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "refuse_friends_requests")
        {
            cout<<"接收到命令:拒绝好友申请"<<endl;
            friends.refuse_friend_request(*this,data_socket,to_id,from_id,redis_data,msg);
           // recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "send_message")
        {
           cout<<"接收到命令：发送消息"<<endl;
           friends.send_message(*this,data_socket,from_id,to_id,message,redis_data,msg);
           // recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "nothing")
        {
            //recived_message(redis_data,find_user_id(data_socket),data_socket);
            continue;
        }else if(type == "friend_open_block")
        {
            cout<<"接收到命令，打开聊天框"<<endl;
            friends.open_block(*this,data_socket,from_id,to_id,redis_data,msg);
           // recived_message(redis_data,find_user_id(data_socket),data_socket);
        }
        else if(type == "quit_chat")
        {
            cout<<"当前命令：退出聊天"<<endl;
            friends.quit_chat(*this,data_socket,from_id,to_id,redis_data,msg);

        }else if(type == "shield_friend")
        {
            cout<<"接收到命令：屏蔽好友";
            friends.shield_friend(*this,data_socket,from_id,to_id,redis_data,msg);
        }else if(type == "cancel_shield_friend")
        {
            cout<<"接收到命令：取消屏蔽好友";
            friends.cancel_shield_friend(*this,data_socket,from_id,to_id,redis_data,msg);
        }else if(type == "send_message_no")
        {
            cout<<"接收到命令：发送消息2"<<endl;
           friends.send_message(*this,data_socket,from_id,to_id,message,redis_data,msg);
            
        }else if(type == "new_message")
        {
             cout<<"接收到命令：查看新消息"<<endl;
             friends.new_message(*this,data_socket,from_id,to_id,redis_data,msg);
        }else if(type == "send_file")
        {
            cout<<"接收到命令：发送文件"<<endl;
            friends.send_file(*this,data_socket,from_id,to_id,message,redis_data,msg);
        // recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "accept_file")
        {
            cout<<"接收到命令：接收文件"<<endl;
            friends.accept_file(*this,data_socket,from_id,to_id,message,redis_data,msg);
           //  recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "is_friends")
        {
            cout<<"收到命令：私聊好友"<<endl;
            friends.is_friends(*this,data_socket,from_id,to_id,message,redis_data,msg);
            //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "generate_group")
        {
            cout<<"收到命令：创建群聊"<<endl;
            group.generate_group(*this,data_socket,from_id,to_id,message,redis_data,msg);
          
        }else if(type == "delete_group")
        {
            cout<<"收到命令：删除群聊"<<endl;
            group.delete_group(*this,data_socket,from_id,to_id,message,redis_data,msg);
           
        }else if(type == "get_all_my_group")
        {   cout<<"收到命令：查看所有群聊"<<endl;
            group.see_all_group(*this,data_socket,from_id,to_id,message,redis_data,msg);
            //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "see_all_member")
        {
            cout<<"收到命令：查看群所有成员"<<endl;
            group.see_all_member(*this,data_socket,from_id,to_id,message,redis_data,msg);
            // recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "add_admin")
        {
            group.add_admin(*this,data_socket,from_id,to_id,message,redis_data,msg);
            // recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "open_group")
        {
            group.open_group(*this,data_socket,from_id,to_id,message,redis_data,msg);
            //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "quit_group")
        {
            group.quit_group(*this,data_socket,from_id,to_id,message,redis_data,msg);
           // recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "delete_admin")
        {
              group.delete_admin(*this,data_socket,from_id,to_id,message,redis_data,msg);
              // recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "add_member")
        {
             group.add_member(*this,data_socket,from_id,to_id,message,redis_data,msg);
              //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "delete_member")
        {
            group.delete_member(*this,data_socket,from_id,to_id,message,redis_data,msg);
            // recived_message(redis_data,find_user_id(data_socket),data_socket);
            //group.quit_group(data_socket,from_id,to_id,message,redis_data);
        }else if(type == "send_add_group")
        {
            group.send_add_group(*this,data_socket,from_id,to_id,message,redis_data,msg);
        }else if(type == "see_add_group")
        {
            group.see_add_group(*this,data_socket,from_id,to_id,message,redis_data,msg);
            //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type ==  "approve_group_requests")
        {
            group.add_group_member(*this,data_socket,from_id,to_id,message,redis_data,msg);
             //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "refuse_group_requests")
        {
             group.refuse_group_member(*this,data_socket,from_id,to_id,message,redis_data,msg);
              //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type ==  "send_message_group")
        {
            group.add_group_message(*this,data_socket,from_id,to_id,message,redis_data,msg);
        }else if(type == "receive_group_new")
        {
            group.receive_group_message(*this,data_socket,from_id,to_id,message,redis_data,msg);
        }else if(type == "group_open_block")
        {
            group.open_group_block(*this,data_socket,from_id,to_id,message,redis_data,msg);
        }else if(type ==  "send_file_group")
        {
            cout<<"收到命令：上传群文件"<<endl;
            group.send_file_group(*this,data_socket,from_id,to_id,message,redis_data,msg);
             //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }else if(type == "accept_file_group")
        {
            cout<<"收到命令：下载群文件"<<endl;
             group.accept_file_group(*this,data_socket,from_id,to_id,message,redis_data,msg);
              //recived_message(redis_data,find_user_id(data_socket),data_socket);
        }
        else if(type == "heart")
        {
            cout<<"接收到客户端心跳监测"<<endl;
             updateHeartbeat(data_socket);
        }
        else if(type == "quit_chat_group")
        {
            cout<<"收到命令：退出群聊天"<<endl;
             group.quit_chat(*this,data_socket,from_id,to_id,message,redis_data,msg);
             
        }else if(type.empty())
        {
            cout<<"json解析结束后有某个消息为空"<<endl;
        }
        else{
            cout<<"接收到命令：未知命令"<<endl;
            cout<<"未知命令为"<<type<<endl;
            break;
        }
     }
 }


