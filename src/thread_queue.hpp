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
        cv_add.wait(lock);
    }
    void remove(Device *device, void *p_input, int input_size, int hash_size, void (*process)(Device *,void*,int,int))
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (current > 0)
        {
            process(device, p_input, input_size, hash_size);
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