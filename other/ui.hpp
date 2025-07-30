#include <iostream>
using namespace std;

void sign_up_page(){
cout<<"-——-——-welcome to my chatroom-——-——- "<<endl;
cout<<"           请选择操作OVO           "<<endl;
cout<<"              1.注册               "<<endl;
cout<<"              2.登陆               "<<endl;
cout<<"              3.注销               "<<endl;
cout<<"              4.退出               "<<endl;
cout<<"-——-——-——-——请输入相应编码——-——-——-—— "<<endl;
}
void main_page(string user_id,string username)
{

cout<<"-——-——-——-——-——-——-——-——-——-——-——-——- "<<endl;
cout<<"     "<< username <<"                "<<endl;
cout<<"      "<<user_id <<"                 "<<endl;
cout<<"             请选择操作OVO           "<<endl;
cout<<"     1.私聊好友           4.查看群聊  "<<endl;
cout<<"     2.管理好友           5.群聊管理   "<<endl;
cout<<"     3.查看好友           6.进入群聊    "<<endl;
cout<<"             -1.退出                 "<<endl;
cout<<"——-——-——-——-————-——-——-——-——-——-——-—— "<<endl;
}
void print_block()
{
    cout<<"-——-——-——-——-——-——-——-——-——-——-——-——- "<<endl;
    cout<<"             1.发送消息              "<<endl;
    cout<<"             2.发送文件              "<<endl;
    cout<<"             3.接收文件              "<<endl;
    cout<<"             -1.退出                 "<<endl;
    cout<<"-——-——-——-——-——-——-——-——-——-——-——-——- "<<endl;
}
void main_page2(string user_id,string username)
{

cout<<"-——-——-——-——-——-——-——-——-——-——-——-——- "<<endl;
cout<<"     "<< username<<"                "<<endl;
cout<<"      "<<user_id<<"                 "<<endl;
cout<<"            请选择操作OVO            "<<endl;
cout<<"             1.添加好友              "<<endl;
cout<<"             2.删除好友              "<<endl;
cout<<"             3.屏蔽好友              "<<endl;
cout<<"             4.取消屏蔽              "<<endl;
cout<<"             5.查看申请              "<<endl;
cout<<"             -1.退出                "<<endl;
cout<<"——-——-——-——-————-——-——-——-——-——-——-—— "<<endl;
}
void main_page3(string user_id,string username)
{

cout<<"-——-——-——-——-——-——-——-——-——-——-——-——- "<<endl;
cout<<"     "<< username<<"                "<<endl;
cout<<"      "<<user_id<<"                 "<<endl;
cout<<"            请选择操作OVO            "<<endl;
cout<<"             1.创建群聊              "<<endl;
cout<<"             2.解散群聊              "<<endl;
cout<<"             3.加入群聊              "<<endl;
cout<<"             -1.退出                "<<endl;
cout<<"——-——-——-——-————-——-——-——-——-——-——-—— "<<endl;
}
void main_page_owner(string user_id,string username)
{

cout<<"-——-——-——-——-——-——-——-——-——-——-——-——- "<<endl;
cout<<"            请选择操作OVO            "<<endl;
cout<<"                                    "<<endl;
cout<<"             1.在线聊天              "<<endl;
cout<<"             2.发送文件              "<<endl;
cout<<"             3.接收文件              "<<endl;
cout<<"             4.退出群聊              "<<endl;
cout<<"             5.查看成员列表           "<<endl;
cout<<"             6.设置管理员            "<<endl;
cout<<"             7.管理成员              "<<endl;
cout<<"             8.查看申请              "<<endl;
cout<<"             -1.退出                "<<endl;
cout<<"——-——-——-——-————-——-——-——-——-——-——-—— "<<endl;
}
void main_page_admin(string user_id,string username)
{

cout<<"-——-——-——-——-——-——-——-——-——-——-——-——- "<<endl;
cout<<"            请选择操作OVO            "<<endl;
cout<<"                                    "<<endl;
cout<<"             1.在线聊天              "<<endl;
cout<<"             2.发送文件              "<<endl;
cout<<"             3.接收文件              "<<endl;
cout<<"             4.退出群聊              "<<endl;
cout<<"             5.查看成员列表              "<<endl;
cout<<"             6.管理成员              "<<endl;
cout<<"             7.查看申请              "<<endl;
cout<<"             -1.退出                "<<endl;
cout<<"——-——-——-——-————-——-——-——-——-——-——-—— "<<endl;
}
void main_page_member(string user_id,string username)
{

cout<<"-——-——-——-——-——-——-——-——-——-——-——-——- "<<endl;
cout<<"            请选择操作OVO            "<<endl;
cout<<"                                    "<<endl;
cout<<"             1.在线聊天              "<<endl;
cout<<"             2.发送文件              "<<endl;
cout<<"             3.接收文件              "<<endl;
cout<<"             4.退出群聊              "<<endl;
cout<<"             5.查看成员列表              "<<endl;
cout<<"             6.邀请好友进入              "<<endl;
cout<<"             -1.退出                "<<endl;
cout<<"——-——-——-——-————-——-——-——-——-——-——-—— "<<endl;
}
// int main()
// {
//     PRINT a;
//     a.sign_up_page();
//     return 0;
// }