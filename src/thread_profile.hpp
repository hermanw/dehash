#pragma once

// #ifdef THREAD_PROFILE
#include <iostream>
#include <map>
#include <vector>
#include <chrono>

using namespace std::chrono;
using std::cin;
using std::cout;
using std::endl;
using std::map;
using std::string;
using std::vector;

map<string, vector<steady_clock::duration> > tp_data;

void tp_begin(string id)
{
    if (tp_data.find(id) == tp_data.end())
    {
        vector<steady_clock::duration> v;
        tp_data[id] = v;
    }
    auto now = steady_clock::now() - steady_clock::time_point::min();
    tp_data[id].push_back(now);
}

void tp_end(string id)
{
    auto start = tp_data[id].back();
    auto now = steady_clock::now() - steady_clock::time_point::min();
    tp_data[id].pop_back();
    tp_data[id].push_back(now - start);
}

void print_tp_data()
{
    for (auto &item : tp_data)
    {
        cout << item.first << endl;
        for (auto &dur : item.second)
        {
            cout << duration_cast<microseconds>(dur).count() << ", ";
        }
        cout << endl;
    }
}

// #endif

#define TP_BEGIN(id) tp_begin(id)
#define TP_END(id) tp_end(id)
#define PRINT_TP_DATA() print_tp_data()