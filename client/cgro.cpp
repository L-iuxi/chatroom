#include "cgro.hpp"
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
  // cout<<"确定要删除该群聊吗"<<endl;
     cout<<"确定要删除该群聊吗"<<endl;
    PRINT_GREEN("【1】确定");
    PRINT_RED("【2】点错了");
   //cout<<"1.确定"<<endl<<"2.点错了"<<endl;
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

void GRO::open_group_owner(TCP &client,LOGIN &login,int &m,string group_id)
{

    int command = 0; 
    string b;
    main_page_owner(login.getuser_id(),login.getusername());
    while (!(cin >> command)) {
    clear_screen() ;
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
           
             cin.clear();
              cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
           main_page_owner(login.getuser_id(),login.getusername());
        }
        
   cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
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
        //client.recv_server(client.data_socket);
        break;
        case 2:
        cout<<"当前操作：发送文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        send_file_group(client,login,group_id);
       // client.recv_server(client.data_socket);
        break;
         case 3:
        cout<<"当前操作：发送文件"<<endl;
        accept_file_group(client,login,group_id);
        //client.recv_server(client.data_socket);
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        }  
        break;
        case 4:
        cout<<"当前操作：退出群聊"<<endl;
        quit_group(client,login,group_id,m);

        break;
        case 5:
        cout<<"当前操作：查看群成员"<<endl;
        see_all_members(client,login,group_id);
        //client.recv_server(client.data_socket);
        break;
        case 6:
        cout<<"当前操作：设置管理员"<<endl;
        manage_admin(client,login,group_id);
         ///client.recv_server(client.data_socket);
        break;
        case 7:
        cout<<"当前操作：管理成员"<<endl;
        manage_member(client,login,group_id);
         //client.recv_server(client.data_socket);
        break;
        case 8:
        cout<<"当前操作：查看申请"<<endl;
        see_add_group(client,login,group_id);
         //client.recv_server(client.data_socket);
        break;
        case -1:
        clear_screen() ;
        cout<<"当前操作：退出"<<endl;
        m =1;
        return;
        break;
        default:  
        clear_screen() ;
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
            
        break;
        if(command == -1)
    {
        m = 1;
        
    }
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
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
   
    // 等待发送消息的信号
    while (group_active.load()) {
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
       typ = "quit_chat_group";
        client.send_m(typ,from_id,to_id,message);
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
    // client.send_m(type,from_id,to_id,message);
    
    string buffer;
    client.rec_m(type,buffer);
    //cout<<"服务器无数据返回"<<endl;
    if(buffer == "quit")
    {
        group_active = false;
        break;
    }
    // else if(buffer == "你不在群聊内")
    // {
    //     cout<<buffer<<endl;
    //     chat_active = false;
    //     PRINT_RED("正在退出群聊......");
    //     break;
    // }
        cout<<endl;
       cout << "\033[1;36m[" << type << "]\033[0m" << buffer << endl;
    }
 
//cout<<"我也结束了"<<endl;
}
void GRO::open_group_admin(TCP &client,LOGIN &login,int &m,string group_id)
{
    int command = 0; 
    string b;
    main_page_admin(login.getuser_id(),login.getusername());
    while (!(cin >> command)) {
    clear_screen() ;
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
           
             cin.clear();
              cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
           main_page_admin(login.getuser_id(),login.getusername());
        }
        
   cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
    string type = "";
    string to_id = "";
    string from_id = "";
    string message= "";
    switch(command){
        case 1:
        cout<<"当前操作：在线聊天"<<endl;
        open_group_block(client,login,group_id);
        //client.recv_server(client.data_socket);
        break;
        case 2:
        cout<<"当前操作：发送文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        send_file_group(client,login,group_id);
       // client.recv_server(client.data_socket);
        break;
        case 3:
        cout<<"当前操作：接收文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        accept_file_group(client,login,group_id);
        
        //client.recv_server(client.data_socket);

        break;
        case 4:
        cout<<"当前操作：退出群聊"<<endl;
        quit_group(client,login,group_id,m);
         //client.recv_server(client.data_socket);
        break;
        case 5:
        cout<<"当前操作：查看群成员"<<endl;
        see_all_members(client,login,group_id);
        // client.recv_server(client.data_socket);
        break;
        case 6:
        cout<<"当前操作：管理成员"<<endl;
        manage_member(client,login,group_id);
        // client.recv_server(client.data_socket);
        break;
        case -1:
        cout<<"当前操作：退出"<<endl;
        clear_screen() ;
        m = 1;
        return;
        break;
        default:  
        clear_screen() ;
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
            break;

        if(command == -1)
    {
        m =1 ;
        break;
    }
    }   
   
}
void GRO::open_group_member(TCP &client,LOGIN &login,int &m,string group_id)
{

    int command = 0; 
    string b;
    main_page_member(login.getuser_id(),login.getusername());
     while (!(cin >> command)) {
    clear_screen() ;
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
           
             cin.clear();
              cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
           main_page_member(login.getuser_id(),login.getusername());
        }
        
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
       // client.recv_server(client.data_socket);
        break;
        case 2:
        cout<<"当前操作：发送文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        send_file_group(client,login,group_id);
        //client.recv_server(client.data_socket);
        break;
        case 3:
        cout<<"当前操作：接收文件"<<endl;
        if(this->is_transfer)
        {
            cout<<"正在发送/接收文件...请等待当前文件处理完"<<endl;
            break;
        } 
        accept_file_group(client,login,group_id);
        //client.recv_server(client.data_socket);
        break;
        case 4:
        cout<<"当前操作：退出群聊"<<endl;
        quit_group(client,login,group_id,m);
        // client.recv_server(client.data_socket);
        break;
        case 5:
        cout<<"当前操作：查看群成员"<<endl;
        see_all_members(client,login,group_id);
        // client.recv_server(client.data_socket);
        break;
        case -1:
        cout<<"当前操作：退出"<<endl;
        clear_screen() ;
        m =1;
        return;
        break;
        default:  
        clear_screen() ;
            cout << "\033[31m错误：无效选项，请重新输入！\033[0m" << endl;
    
        break;
         if(command == -1)
    {
        m =1 ;
        break;
    }
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
    string to_id = "0";
    //char buffer[BUFFER_SIZE];
    cout<<"确定要退出群聊吗TAT"<<endl;
    cout<<"1.确定"<<"2.点错啦> <"<<endl;
  int command;
    while (1)
    {
        // Check if input is valid integer
        while (!(cin >> command))
        {
            cin.clear(); // Clear error flags
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Discard bad input
            cout << "输入无效，请输入1或者2" << endl;
            cout << "1.确定" << "2.点错啦> <" << endl;
        }
        
        if (command == 1)
        {
            break;
        }
        else if (command == 2)
        {
            return;
        }
        cout << "请输入1或者2" << endl;
    }
    client.send_m(type,from_id,to_id,message);
    string buffer;
    client.rec_m(type,buffer);
   
    cout  << buffer << endl;
     m =1 ;
    
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
  
    cout  << buffer << endl;
    if(buffer == "无申请消息")
    {
        return;
    }

    int command = -1;
    cout<<"请选择操作"<<endl<<"1.同意某申请"<<endl<<"2.拒绝某申请"<<endl<<"-1.退出"<<endl;
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
            cout<<"请输入想要通过的申请id"<<endl;
            string to_id;
            cin>>to_id;
            string message = group_id;
            type = "approve_group_requests";
            client.send_m(type,from_id,to_id,message);
            string buffer2;
            client.rec_m(type,buffer2);
    
            cout<<buffer2<<endl;
           

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
           
            cout<<buffer2<<endl;
          
        }else if(command == -1)
        {
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
        cout<<"文件不存在"<<endl;
        return;
    }
    test_file.close();
  string to_id = group_id;
  string buffer,type;
  client.send_m("send_file_group", from_id, to_id, filepath);
  client.rec_m(type,buffer);
  if(buffer!= "group")
  {
    cout<<buffer<<endl;
    return;
  }
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
    if (response == "NO_FILES") {
        cout << "没有文件" << endl;
        return;
    }

    // 显示文件列表并让用户选择
    cout << "可下载文件列表:\n" << buffer1;
    cout << "请输入要下载的文件序号: ";
    
    int choice;
    while (true) {
        cin >> choice;
        if (cin.fail()) {  
            cout << "输入无效，请输入数字: ";
            cin.clear(); 
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
            continue;
        }
        cin.ignore();  
        break;
    }
string choice_str = to_string(choice);
    send(client.data_socket, choice_str.c_str(), choice_str.size(), 0);

    // 接收服务器对 choice 的响应
    char response_buf[BUFFER_SIZE];
    bytes = recv(client.data_socket, response_buf, BUFFER_SIZE, 0);
    if (bytes <= 0) {
        cout << "服务器无响应" << endl;
        return;
    }
    response_buf[bytes] = '\0';
    string choice_response(response_buf);

    if (choice_response == "INVALID_CHOICE") {
        cout << "无效的选择，请重新操作！" << endl;
        return;
    } else if (choice_response != "VALID_CHOICE") {
        cout << "未知的服务器响应" << endl;
        return;
    }


    client.connect_transfer_socket();
    this->is_transfer = true;
     this->is_transfer = true;
    thread([this,&client,download_dir](){
    char filename_buf[256];
    char buffer[BUFFER_SIZE];
    int name_len = recv(client.transfer_socket, filename_buf, sizeof(filename_buf)-1, 0);
    if(name_len <= 0) {
        cerr << "\033[31m接收文件名失败\033[0m" << endl;
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
    if(bytes_recv <= 0) {
        if(bytes_recv == 0) {
            cout << "文件传输完成" << endl;
        } else {
            cerr << "接收错误" << endl;
        }
        break;
    }
    outfile.write(buffer, bytes_recv);
    total_received += bytes_recv;
}
    outfile.close();

    // if(total_received > 0) {
        cout << "\033[32m文件下载成功: " << save_path << " (" << total_received << " bytes)\033[0m" << endl;
    // } else {
    //     remove(save_path.c_str());
    //     cout << "文件下载失败" << endl;
    // }
   // client.resume_heartbeat();//恢复心跳监测
    close(client.transfer_socket);
   // cout<<"111"<<endl;
   this->is_transfer = false;
    }).detach(); 
    
}
