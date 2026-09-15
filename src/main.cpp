#include <cstdio>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

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
        int bytesRead;
        char buffer[4096]; // dumb piece of memory holding down 5096 slots in the memory 
        bytesRead = read(connection_socket, buffer, sizeof(buffer)-1);
        if(bytesRead > 0){
            buffer[bytesRead] = '\0'; // manually marking the end of the memory we just read as there could be more which will be fetched in the future iterations
        std::cout << buffer << std::endl;
        }
        std::string body = "Hello, Danyal speaking!"; // len = 23
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 23\r\n\r\n" + body; // update to a dynamic response in the future
        write(connection_socket, response.c_str(), response.length());
        close(connection_socket);
    }
    return 0;
}