#include "client.hpp"
int main(int argc, char* argv[]){

if (argc != 2) {
    cerr << "./Client 10.30.0.142\n";
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

if (!(cin >> command)) {
            cout<<"无效的命令，重新输入吧"<<endl;
            cin.clear(); 
            cin.ignore(numeric_limits<streamsize>::max(), '\n');  
            continue;  
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
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
  //  cout<<"登陆成功!"<<endl;
  clear_screen() ;
    thread heartbeat_thread(&TCP::heartbeat, &client);
    heartbeat_thread.detach();
    
    friends.make_choice(client,login);
   heartbeat_received = false;
//    close(heart_socket);
//    close(notice_socket);
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
