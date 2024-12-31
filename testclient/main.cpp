#include <iostream>
#include <system_error>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>

#include "network.h"
#include "session.h"
#include "commands.h"
#include <option.hpp>

static std::string strip(std::string_view data)
{
    std::string_view::const_iterator first = data.cbegin();
    auto last = data.crbegin();

    while (first != data.cend() && *first == ' ') ++first;
    while (last != data.crend() && *last == ' ') ++last;

    if (first >= last.base()) return {};
    return { first, last.base() };
}

static std::vector<std::string> split(std::string_view data)
{
    std::vector<std::string> dataList;

    long long begin = -1;
    long long i = 0;

    for (; static_cast<size_t>(i) < data.size(); i++) {
        if (data[i] == ' ') {
            if ((i - begin - 1) > 0)
                dataList.emplace_back(data.begin() + (begin + 1), data.begin() + i);
            begin = i;
        }
    }
    dataList.emplace_back(data.begin() + (begin + 1), data.begin() + i);

    return dataList;
}

qls::CommandManager command_manager;

int main() {
    std::vector<std::thread> thread_vector;
    for (int i = 0; i < 10; ++i) {
        thread_vector.emplace_back([]() {
            qls::Network network;
            qls::Session session(network);
            std::atomic<bool> can_be_used = false;

            network.add_connected_error_callback("connected_error_callback", [](std::error_code ec) {
                std::cerr << "Connected error: " << ec.message() << '\n';
            });
            network.add_connected_callback("connected_callback", [&](){
                std::cout << "Connected to server successfully!\n";
                can_be_used = true;
            });
            network.add_received_stdstring_callback("received_stdstring_callback", [](std::string message) {
                auto pack = qls::DataPackage::stringToPackage(message);
                
                std::cout << "Message received:\n"
                        << "\tType: " << pack->type << '\n'
                        << "\tBody: " << pack->getData() << '\n';
            });

            network.connect();

            std::cout << "Connecting to server...\n";
            size_t i = 0;
            auto start = std::chrono::high_resolution_clock::now();
            for (; i < 10000ull; ++i) {
                if (!can_be_used) {
                    using namespace std::chrono;
                    std::this_thread::sleep_for(0.1s);
                    continue;
                }
                try {
                    auto command = command_manager.getCommand("registerUser");
                    opt::Option opt = command->getOption();
                    opt.parse(std::vector<std::string>{"--email=1@qq.com", "--password=123456"});
                    command->execute(session, opt);
                } catch(const std::exception& e) {
                    std::cerr << e.what() << '\n';
                }
            }
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << i << ' ' << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << '\n';
        });
    }
    for (auto& i: thread_vector) {
        if (i.joinable())
            i.join();
    }
}
