#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <boost/program_options.hpp>

#include "decoder.h"
#include "device_pool.h"

bool read_from_file(const std::string &filename, std::string &str)
{
    std::ifstream file(filename);
    if (file.is_open())
    {
        std::ostringstream stream;
        stream << file.rdbuf();
        file.close();
        str = stream.str();
        return true;
    }
    else
    {
        return false;
    }
}

void write_to_file(const std::string &filename, std::string &str)
{
    std::string outfile(filename);
    outfile += ".out";
    std::ofstream file(outfile);
    file << str;
    file.close();
    std::cout << "please find results in file: " << outfile << std::endl;
}

void print_version()
{
    std::cout << "hash decoder 0.1, by herman\n";
}

void print_info()
{
    std::cout << "compute devices:\n";
    auto dp = new DevicePool();
    dp->print_info();
    delete dp;
}

void benchmark()
{
    std::cout << "compute devices:\n";
    auto thecfg = new Cfg("config.json", "benchmark");
    Decoder decoder(*thecfg);
    decoder.set_hash_string("f3cf9924949207114472783ed41aa9ee,757295d360508357f8ac682c432416f3,ec7adbba8ee2f1481821b879541fdb7e,eeb1f4145fe2adb38356cd665cf0a179,3f3a4159a9340300c8db318831152f14,cc32ce34137a758ee009bd521e00177f,e2577739aee99b47211b4e23b9ce802b,26b3e34f0b1c139693c725a2dd79b3d4,ef5e10e0d2d0134ac37d89fbf98539c0,6e39bf085901f95c7123ff9205e3f9c2");
    decoder.benchmark();
    delete thecfg;

    // write dehash.ini
    std::cout << "--------\nyou can choose to save devices list in dehash.ini\n";
    std::cout << "input like 0,1,2 (enter to skip):";
    std::string list;
    std::getline(std::cin, list);
    if (list.size())
    {
        std::ofstream file("dehash.ini");
        file << "devices = " << list << "\n";
        file.close();
    }
}

void decode(const std::string &str, std::string &result, const std::string &devices, Cfg &cfg)
{
    Decoder decoder(cfg);
    decoder.set_hash_string(str.c_str());
    // const int hash_len = decoder.get_hash_len();
    // const int dedup_len = decoder.get_dedup_len();
    // std::cout << "find " << hash_len << " hashes (" << hash_len - dedup_len << " duplicated, " << dedup_len << " unique)\n";
    // std::cout << "using decode pattern \"" << cfg. << "\"" << std::endl;

    decoder.decode(devices);
    // decoder.get_result(result);
}

int main(int argc, char *argv[])
{
    // check dehash.ini
    std::ifstream ini("dehash.ini");
    if (!ini.good())
    {
        // run for the first time
        print_info();
        return 0;
    }

    // handle program options
    std::string devices;
    std::string cfg;
    namespace po = boost::program_options;
    po::options_description desc("usage: dehash [option] filename\noptions");
    desc.add_options()
        ("help,h", "print help message")
        ("version,v", "print version")
        ("info,i", "print compute devices info")
        ("benchmark,b", "benchmark compute devices")
        ("devices,d", po::value<std::string>(&devices), "set compute devices: -d 0,1,2")
        ("cfg,c", po::value<std::string>(&cfg), "set decode pattern [required]")
        ;
    po::positional_options_description p;
    p.add("input-file", -1);
    po::options_description cmdline_options;
    cmdline_options.add(desc).add_options()
        ("input-file", po::value< std::string >(), "input file")
        ;
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    po::store(po::parse_config_file("dehash.ini", desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc ;
        return 0;
    }
    if (vm.count("version"))
    {
        print_version();
        return 0;
    }
    if (vm.count("info"))
    {
        print_info();
        return 0;
    }
    if (vm.count("benchmark"))
    {
        benchmark();
        return 0;
    }
    if (!vm.count("input-file") || !vm.count("cfg"))
    {
        std::cout << desc ;
        return 0;
    }

    print_version();

    // process hash file
    auto filename = vm["input-file"].as<std::string>();
    std::string str;
    if (!(read_from_file(filename, str)))
    {
        std::cout << "file " << filename << " not found\n";
        return 1;
    }

    // decode
    auto thecfg = new Cfg("config.json", cfg.c_str());
    std::string result;
    decode(str, result, devices, *thecfg);
    delete thecfg;

    // write reults
    write_to_file(filename, result);
}
