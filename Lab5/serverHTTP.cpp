#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <winsock2.h>
#define PORT 8080
#define BUFFER_SIZE 4096

std::string readFile(const std::string& fileName)
{
    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void handleRequest(SOCKET clientSocket)
{
    char buffer[BUFFER_SIZE] = {0};
    //блокується і чекає поки буде інформація
    int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead > 0)
    {
        std::string request(buffer);
        //виводим перший рядок запиту
        std::cout << "Request Received\n" << request.substr(0, request.find("\r\n")) << "\n";
        //рядок у потік даних
        std::istringstream iss(request);
        std::string method, path, protocol;
        // розбиваєм по частинам, гет - індекс - версія
        iss >> method >> path >> protocol;
        if (path == "/") path = "/index.html";
        std::string filePath = path.substr(1);
        std::string content = readFile(filePath);
        std::string response;
        if (!content.empty())
        {
            response = "HTTP/1.1 200 OK\r\n";
            //каже що відправляєм і розмір у байтах
            response += "Content-Type: text/html\r\n";
            response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
            //відділяє заголовки і тіло відповіді
            response += "\r\n";
            response += content;
        }
        else
        {
            std::string errorMsg = "<h1>404 Not Found</h1>";
            response = "HTTP/1.1 404 Not Found\r\n";
            response += "Content-Length: " + std::to_string(errorMsg.size()) + "\r\n";
            //відділяє заголовки і тіло відповіді
            response += "\r\n";
            response += errorMsg;
        }
        //надсилаєм і потім розриваєм з'єднання
        send(clientSocket, response.c_str(), (int)response.size(), 0);
    }
    closesocket(clientSocket);
}

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);
    std::cout << "Server http://localhost:" << PORT << std::endl;
    while (true)
    {
        sockaddr_in clientAddr;
        //кажем розмір заздалегідь
        int clientAddrSize = sizeof(clientAddr);
        //зупиняємся і чекаєм, потім новий сокет
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket != INVALID_SOCKET)
        {
            //все добре то детач
            std::thread(handleRequest, clientSocket).detach();
        }
    }
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}