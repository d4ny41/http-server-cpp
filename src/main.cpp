#include <cstddef>
#include <cstdio>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <map>
#include <fstream>


int main(){
    int listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(listeningSocket == -1){
        perror("socket failed \n");
        return 1;
    }
    int option =1;
    if(setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))< 0){
        perror("setting socket option failed! \n");
        return 1;
    }
    sockaddr_in serverAddress;
    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    // this tells the structure that it holds an ipv4 address
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    // tells os to accept connections on any network interface instead of just my laptop's ip address
    serverAddress.sin_port = htons(8080);
    // setting socket internet port to the standard for html
    if(bind(listeningSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress))< 0){
        perror("binding error \n");
        return 1;
    }
    if(listen(listeningSocket, 128) < 0){
        // the backlog is 128
        perror("Listening Error \n");
        return 1;
    } else{
        std::cout << "Listening on port 8080" << "\n" << std::endl;
    }
    // server shouold be active now
    bool running = true;
    while(running){
        int connection_socket;
        sockaddr_in client_address;
        socklen_t clientLen = sizeof(client_address);
        // the client address hold the ip address of the browser each time the loop iterates
        // when accepts answers a connection, client_address tells the OS who called hence empty struct
        connection_socket = accept(listeningSocket, (struct sockaddr *)&client_address, &clientLen);
        if(connection_socket < 0){
            perror("Connection to client failed! \n");
            continue;
        }
        std::string request_buffer;        
        int bytesRead;

        while(true){
            char buffer[4096]; // dumb piece of memory holding down 5096 slots in the memory 
            bytesRead = read(connection_socket, buffer, sizeof(buffer)-1);
            if(bytesRead > 0){
                buffer[bytesRead] = '\0'; // manually marking the end of the memory we just read as there could be more which will be fetched in the future iterations
                request_buffer += buffer;
                if(request_buffer.find("\r\n\r\n") != std::string::npos || request_buffer.length() > 8192){
                    break;
                }
            }else{
                break;
            }
        }
        if(request_buffer.empty()){
            close(connection_socket);
            continue;
        }
        size_t line_end = request_buffer.find("\r\n");
        if(line_end == std::string::npos){
            close(connection_socket);
            continue;
        }
        std::string request_line = request_buffer.substr(0, line_end);
        size_t first_space = request_line.find(' ');
        if (first_space == std::string::npos) {
            close(connection_socket);
            continue;
        }
        size_t second_space = request_line.find(' ', first_space + 1);
        if (second_space == std::string::npos) {
            close(connection_socket);
            continue;
        }
        //  Slicing out the three substrings
        std::string method = request_line.substr(0, first_space);
        std::string path = request_line.substr(first_space + 1, second_space - (first_space + 1));
        std::string version = request_line.substr(second_space + 1);

        // Printing the parsed values to verify
        std::cout << "Method: " << method << "\n"
                  << "Path: " << path << "\n"
                  << "Version: " << version << std::endl;
        // headers are parsed one line at a time into a key value map
    std::map<std::string, std::string> headers;
    size_t pos = line_end + 2; // Start right after the first "\r\n"

    while (pos < request_buffer.length()) {
      size_t next_line = request_buffer.find("\r\n", pos);
      // If next_line doesn't havea position, we hit the empty line
      if (next_line == std::string::npos || next_line == pos) {
        break;
      }
      std::string header_line = request_buffer.substr(pos, next_line - pos);
      size_t colon = header_line.find(':');
     
      if (colon != std::string::npos) {
        std::string key = header_line.substr(0, colon);
        std::string value = header_line.substr(colon + 2); // Skip ": "
        headers[key] = value;
      }

      pos = next_line + 2; // Jump past "\r\n" to the next header line
    }

    // only for verification, remove bevore git committing 
    if (headers.count("Host")) {
      std::cout << "Host Header: " << headers["Host"] << std::endl;
    }
    /*   std::string body = "Hello, Danyal speaking!"; // len = 23
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 23\r\n\r\n" + body; // update to a dynamic response in the future
    */ 
    if (path == "/") {
            path = "/index.html";
        }

        std::string file_path = "public" + path;
        std::ifstream file(file_path);

        std::string status_line;
        std::string content_type = "text/html";
        std::string body;

        if (file.is_open()) {
            status_line = "HTTP/1.1 200 OK\r\n";
            std::string line;
            while (std::getline(file, line)) {
                body += line + "\n";
            }
            file.close();
        } else {
            status_line = "HTTP/1.1 404 Not Found\r\n";
            body = "<!DOCTYPE html><html><body><h1>404 Not Found</h1>"
                   "<p>The requested file does not exist.</p></body></html>";
        }

        std::string response = status_line +
                               "Content-Type: " + content_type + "\r\n" +
                               "Content-Length: " + std::to_string(body.length()) + "\r\n" +
                               "\r\n" +
                               body;
        write(connection_socket, response.c_str(), response.length());
        close(connection_socket);
    }
    return 0;
}