#include "redis.hpp"
#include "server.hpp"

int main() {
    DATA redis_data;
    TCP server;
    signal(SIGPIPE, SIG_IGN); 
    server.start(redis_data);
    return 0;
}
