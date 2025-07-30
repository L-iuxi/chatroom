#include "client.hpp"
//创建心跳套接字
void TCP:: heartbeat()
{
    while(1)
    {
    std::this_thread::sleep_for(std::chrono::seconds(5)); // 每5秒休眠一次

    string type = "heart";
    string message = "ping";
    string to_id = "0"
    string from_id = "0";
    send_m(type,from_id,to_id,message);
    }
}