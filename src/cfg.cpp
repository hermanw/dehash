#include <iostream>
#include <fstream>
#include <sstream>
#include "nlohmann/json.hpp"
#include "cfg.h"

using json = nlohmann::json;

void set_data_section(DataSection &ds, json &item)
{
    auto type =item["type"];
    if(type == "list")
    {
        ds.type =  ds_type_list;
    }
    else if(type == "digit")
    {
        ds.type =  ds_type_digit;
    }
    else if(type == "idsum")
    {
        ds.type =  ds_type_idsum;
    }
    else if(type == "char")
    {
        ds.type =  ds_type_char;
    }
    ds.index = item["index"];
    ds.length = item["length"];
    if (item.contains("source"))
    {
        ds.source = item["source"];
    }
}

Cfg::Cfg(const char *filename, const char *cfg_name)
{
    this->cfg_name = cfg_name;
    
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cout << "missing " << filename << std::endl;
        exit(1);
    }
    std::ostringstream stream;
    stream << file.rdbuf();
    file.close();

    auto j = json::parse(stream.str())[cfg_name];
    // length
    length = j["length"];
    // sources
    for (auto &item : j["sources"].items())
    {
        std::vector<std::string> list;
        for (auto &l : item.value())
        {
            list.push_back(l);
        }
        sources[item.key()] = list;
    }
    // cpu_sections
    for (auto &item : j["cpu_sections"].items())
    {
        DataSection ds;
        set_data_section(ds, item.value());
        cpu_sections.push_back(ds);
    }
    // gpu_sections
    int kernel_index = 0;
    for (auto &item : j["gpu_sections"].items())
    {
        DataSection ds;
        set_data_section(ds, item.value());
        gpu_sections.push_back(ds);
        if(ds.type == ds_type_digit)
        {
            kernel_work_size[kernel_index++] = pow(10, ds.length);
        }
        else if (ds.type == ds_type_list)
        {
            kernel_work_size[kernel_index++] = sources[ds.source].size();
        }
    }
}
