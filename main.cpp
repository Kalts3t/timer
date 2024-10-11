#include <iostream>
#include <chrono>
#include <functional>
#include <memory>
#include<sys/epoll.h>
#include <set>

using namespace std;

struct TimerNodeBase {
    time_t expire;
    int64_t id;
};

struct TimerNode:TimerNodeBase {
    using Callback=function<void(const TimerNode &node)>;
    Callback func;
};
//实现set红黑树排序：
bool operator <(const TimerNodeBase&lds, const TimerNodeBase&rds) {
    if(lds.expire < rds.expire) {
        return true;
    }
    if(lds.expire > rds.expire) {
        return false;
    }
    return lds.id < rds.id;
}

class Timer {
public:
    static int64_t GenID() {
        return id++;
    }

    [[nodiscard]] static time_t Get_tick() {
        const auto tc=chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now());
        const auto temp=chrono::duration_cast<chrono::milliseconds>(tc.time_since_epoch());
        return temp.count();
    }


    //添加定时器
    TimerNodeBase addTimer(time_t msec, const TimerNode::Callback &func) {
        TimerNode node;
        node.id = GenID();
        node.expire = Get_tick()+msec;
        node.func=func;
        timer_map.insert(node);
        return static_cast<TimerNodeBase>(node);
    }

    //删除定时器
    void delTimer(const TimerNode &node) {
        auto iter=timer_map.find(node);
        if(iter!=timer_map.end()) {
            timer_map.erase(iter);
        }
    }

    //检查定时器
    bool checkTimer() {
        auto iter=timer_map.begin();
        if(iter!=timer_map.end()&&iter->expire<Get_tick()) {
            iter->func(*iter);
            delTimer(*iter);
            return true;
        }
        return false;
    }

    time_t TimetoSleep() {
        auto iter=timer_map.begin();
        auto distance=iter->expire-Get_tick();
        return distance?distance:0;
    }

private:
    static int64_t id;
    set<TimerNode> timer_map;
};
int64_t Timer::id = 0;
int main()
{
    int fd=epoll_create1(1);
    int i=0;
    Timer t;
    t.addTimer(1200,[&](const TimerNodeBase &node) {
        cout<<node.expire<<" "<<node.id<<endl;
    });
    epoll_event ev[64]{0};

    while(true) {
        int nfds=epoll_wait(fd,ev,64,t.TimetoSleep());
        for(int i=0;i<nfds;i++) {
        }
        while(t.checkTimer());
    }
    return 0;
}
