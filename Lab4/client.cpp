#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <winsock2.h>
#include "protocol.h"

bool SendAll(SOCKET s, const char* buf, int len) {
    int total = 0;
    while (total < len) {
        //вказівник вперед, скільки залишилось
        //зберігає кількість байтів за іт
        int n = send(s, buf + total, len - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    //buf це початок масиву, тотал успішно відправлені
    return true;
}

bool RecvAll(SOCKET s, char* buf, int len) {
    int total = 0;
    while (total < len) {
        int n = recv(s, buf + total, len - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);//ipv4, tcp
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(8080);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed connetion to the server\n";
        return 1;
    }
    std::cout << "Connected to the server\n";
    try {
        int rows = 10000;
        int cols = 10000;

        std::cout << "Matrix " << rows << "x" << cols << std::endl;
        //зразу заповняєм одиничками в байтах
        std::vector<int> net_data(rows * cols, htonl(1));
        //скільки байт відправити після заголовка
        u_long payload_size = 3 * sizeof(u_long) + net_data.size() * sizeof(int);
        std::vector<int> threads = {1, 2, 4, 8, 16};

        for (int i: threads)
        {
            std::cout << "\nTESTING " << i << " THREADS\n" << std::endl;
            unsigned char cmdSend = CMD_SEND_DATA;
            u_long net_payload = htonl(payload_size);

            auto transfer_start = std::chrono::high_resolution_clock::now();
            //передаєм адресу в памяті і розмір
            //1 байт команди
            SendAll(clientSocket, (char*)&cmdSend, sizeof(cmdSend));
            //4 байти розмір
            SendAll(clientSocket, (char*)&net_payload, sizeof(net_payload));

            u_long config[3] = {htonl(i), htonl(rows), htonl(cols)};
            //12 байтів масиву
            SendAll(clientSocket, (char*)config, sizeof(config));
            //матриця (дата повертаєм перший вказівник)
            SendAll(clientSocket, (char*)net_data.data(), net_data.size() * sizeof(int));

            unsigned char ack;
            RecvAll(clientSocket, (char*)&ack, sizeof(ack));

            auto transfer_end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> transfer_ms = transfer_end - transfer_start;

            if (ack == ACK) {
                std::cout << "Server got data.\n";
                std::cout << "Time to transfer matrix: " << transfer_ms.count() << " ms\n";
            }

            auto calc_start = std::chrono::high_resolution_clock::now();

            unsigned char cmdStart = CMD_START;
            u_long zeroPayload = 0;
            //те саме що і зверху
            SendAll(clientSocket, (char*)&cmdStart, sizeof(cmdStart));
            SendAll(clientSocket, (char*)&zeroPayload, sizeof(zeroPayload));
            //чекаєм відповіді
            RecvAll(clientSocket, (char*)&ack, sizeof(ack));
            if (ack == ACK_STARTED) std::cout << "Server started to calculate\n";

            while (true)
            {
                unsigned char cmdStatus = CMD_GET_STATUS;
                //те саме що і зверху
                SendAll(clientSocket, (char*)&cmdStatus, sizeof(cmdStatus));
                SendAll(clientSocket, (char*)&zeroPayload, sizeof(zeroPayload));
                //чекаєм статус
                unsigned char status;
                RecvAll(clientSocket, (char*)&status, sizeof(status));
                //спим якщо нема результату
                if (status == STATUS_PROCESSING)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                //отримуєм суму
                else if (status == STATUS_DONE)
                {
                    int result_net;
                    RecvAll(clientSocket, (char*)&result_net, sizeof(result_net));
                    int final_result = ntohl(result_net);

                    auto calc_end = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double, std::milli> calc_ms = calc_end - calc_start;
                    std::cout << "Finished calculating\n";
                    std::cout << "Summ of matrix: " << final_result << "\n";
                    std::cout << "Time with waiting and calculating: " << calc_ms.count() << " ms\n";
                    break;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error" << e.what() << "\n";
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}