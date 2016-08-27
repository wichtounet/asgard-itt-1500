//=======================================================================
// Copyright (c) 2015-2016 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "asgard/driver.hpp"

namespace {

// Configuration
std::vector<asgard::KeyValue> config;

// The driver connection
asgard::driver_connector driver;

// The remote IDs
int source_id = -1;
int on_action_id = -1;
int off_action_id = -1;

void stop(){
    std::cout << "asgard:itt-1500: stop the driver" << std::endl;

    asgard::unregister_action(driver, source_id, on_action_id);
    asgard::unregister_action(driver, source_id, off_action_id);
    asgard::unregister_source(driver, source_id);

    // Close the socket
    close(driver.socket_fd);
}

void terminate(int){
    stop();

    std::exit(0);
}

std::string command_result(const std::string& command) {
    std::stringstream output;

    char buffer[1024];

    FILE* stream = popen(command.c_str(), "r");

    if (!stream) {
        return {};
    }

    while (fgets(buffer, 1024, stream) != nullptr) {
        output << buffer;
    }

    if (pclose(stream)) {
        return {};
    }

    std::string out(output.str());

    if (out[out.size() - 1] == '\n') {
        return {out.begin(), out.end() - 1};
    }

    return out;
}

} //End of anonymous namespace

int main(){
    // Load the configuration file
    asgard::load_config(config);

    auto bin = asgard::get_string_value(config, "itt_1500_send_path");

    if(bin.empty()){
        std::cout << "asgard:itt-1500: The path to the itt_1500_send utiltiy is not set" << std::endl;
        return 1;
    }

    auto id = asgard::get_string_value(config, "itt_1500_id");

    if(id.empty()){
        std::cout << "asgard:itt-1500: The ITT-1500 ID is not set" << std::endl;
        return 1;
    }

    // Open the connection
    if(!asgard::open_driver_connection(driver, asgard::get_string_value(config, "server_socket_addr").c_str(), asgard::get_int_value(config, "server_socket_port"))){
        return 1;
    }

    // Register signals for "proper" shutdown
    signal(SIGTERM, terminate);
    signal(SIGINT, terminate);

     // Register the source and actions
    source_id = asgard::register_source(driver, "itt-1500");
    on_action_id = asgard::register_action(driver, source_id, "STRING", "on");
    off_action_id = asgard::register_action(driver, source_id, "STRING", "off");

    // Listen for messages from the server
    while(true){
        if(asgard::receive_message(driver.socket_fd, driver.receive_buffer, asgard::buffer_size)){
            std::string message(driver.receive_buffer);
            std::stringstream message_ss(message);

            std::string command;
            message_ss >> command;

            if(command == "ACTION"){
                std::string action;
                message_ss >> action;
                if(action == "on"){
                    std::string value;
                    message_ss >> value;
                    std::cout << "asgard:itt-1500: On Unit " << value << std::endl;

                    auto result = command_result(bin + " -i " + id + " -u " + value + " -t");
                    std::cout << "asgard:itt-1500: result: " << result << std::endl;
                } else if(action == "off"){
                    std::string value;
                    message_ss >> value;
                    std::cout << "asgard:itt-1500: Off Unit " << value << std::endl;

                    auto result = command_result(bin + " -i " + id + " -u " + value + " -f");
                    std::cout << "asgard:itt-1500: result: " << result << std::endl;
                } else {
                    std::cout << "asgard:itt-1500: Unknown action " << action << std::endl;
                }
            } else {
                std::cout << "asgard:itt-1500: Unknown command " << command << std::endl;
            }
        }
    }

    stop();

    return 0;
}
