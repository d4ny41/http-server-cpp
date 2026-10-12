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
#include <thread>


void handle_client(int connection_socket){
    
}
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

    std::string request_body;        
    size_t header_end = request_buffer.find("\r\n\r\n");
    if (header_end != std::string::npos) {
        request_body = request_buffer.substr(header_end + 4);
    }
    // Check if the client sent a "Content-Length" header telling us and calculate how many byts to expext
        if (headers.count("Content-Length")) {
            // std::stoul converts a string into an unsigned integer (size_t)
            size_t content_length = std::stoul(headers["Content-Length"]);

        // If TCP split the request and we haven't received all body bytes yet,
        while (request_body.length() < content_length) {
            char buffer[4096];
            bytesRead = read(connection_socket, buffer, sizeof(buffer) - 1);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                request_body += buffer;
            } else {
                break; // Client disconnected or error
            }
        }
    }
    /*   std::string body = "Hello, Danyal speaking!"; // len = 23
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 23\r\n\r\n" + body; // update to a dynamic response in the future
    */ 
    std::string status_line;
    std::string content_type = "text/html";
    std::string body;
    if (method == "POST") {
        // Print the incoming payload in the server terminal
        std::cout << "POST Payload Received: " << request_body << std::endl;

        status_line = "HTTP/1.1 200 OK\r\n";
        body = "<!DOCTYPE html><html><head><link rel=\"stylesheet\" href=\"/style.css\"></head>"
               "<body><h1>POST Data Received!</h1>"
               "<p>Your server captured: <strong>" + request_body + "</strong></p>"
               "<a href=\"/\" style=\"color: #00ffcc;\">Go back</a></body></html>";

    } else if (method == "GET") {
        if (path == "/") {
            path = "/index.html";
        }

        if (path.find("..") != std::string::npos) {
            status_line = "HTTP/1.1 403 Forbidden\r\n";
            body = "<!DOCTYPE html><html><body><h1>403 Forbidden</h1>"
                   "<p>Access denied.</p></body></html>";
        } else {
            std::string file_path = "public" + path;
            std::ifstream file(file_path, std::ios::binary | std::ios::ate);

            if (file.is_open()) {
                status_line = "HTTP/1.1 200 OK\r\n";

                size_t dot_pos = file_path.rfind('.');
                if (dot_pos != std::string::npos) {
                    std::string ext = file_path.substr(dot_pos);
                    if (ext == ".html") content_type = "text/html";
                    else if (ext == ".css") content_type = "text/css";
                    else if (ext == ".js") content_type = "application/javascript";
                    else if (ext == ".png") content_type = "image/png";
                    else if (ext == ".jpg" || ext == ".jpeg") content_type = "image/jpeg";
                    else if (ext == ".ico") content_type = "image/x-icon";
                    else content_type = "text/plain";
                }

                std::streamsize size = file.tellg();
                file.seekg(0, std::ios::beg);
                body.resize(size);
                file.read(&body[0], size);
                file.close();
            } else {
                status_line = "HTTP/1.1 404 Not Found\r\n";
                body = "<!DOCTYPE html><html><body><h1>404 Not Found</h1>"
                       "<p>The requested file does not exist.</p></body></html>";
            }
        }
    } else {
        // Reject unsupported jttp methods 
        status_line = "HTTP/1.1 405 Method Not Allowed\r\n";
        body = "<!DOCTYPE html><html><body><h1>405 Method Not Allowed</h1></body></html>";
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