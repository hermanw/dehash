#pragma once

#include <mutex>
#include <condition_variable>

class ThreadQueue
{
public:
    ThreadQueue(int _size): size(_size), current(0){};
    void add()
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (current < size)
        {
            if (current == 0)
            {
                cv_remove.notify_one();
            }
            current++;
        }
        else
        {
            cv_add.wait(lock);
        }
    }
    void remove(Device *device, void (*process)(Device *))
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (current > 0)
        {
            process(device);
            if(current==size)
            {
                cv_add.notify_one();
            }
            current--;
        }
        else
        {
            cv_remove.wait(lock);
        }
    }

private:
    const int size;
    int current;
    std::mutex mtx;
    std::condition_variable cv_add;
    std::condition_variable cv_remove;
};