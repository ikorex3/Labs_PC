#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <winsock2.h>
#include "protocol.h"

bool RecvAll(SOCKET s, char* buf, int len) {
    int total = 0;
    while (total < len) {
        int n = recv(s, buf + total, len - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

void Summa(const std::vector<int>& matrix, int starts, int end, int cols, int& result)
{
    int local_sum = 0;
    for (int i = starts; i < end; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            local_sum += matrix[i * cols + j];//бо передали масив
        }
    }
    result = local_sum;
}

int compute_sum_parallel(const std::vector<int>& matrix, int n, int m, int num_threads)
{
    auto t_start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    std::vector results(num_threads, 0);
    int step = n / num_threads;//скільки рядків на потік
    for (int i = 0; i < num_threads; ++i)
    {
        int starts = i * step;
        int end = starts + step;
        if (i == num_threads - 1)//якщо не націло
        {
            end = n;
        }
        threads.emplace_back(Summa, std::ref(matrix), starts, end, m, std::ref(results[i]));
    }

    for (auto& t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    int total_sum = 0;
    for (int res : results)
    {
        total_sum += res;
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> pure_time = t_end - t_start;
    std::cout << "Time calculating: " << pure_time.count() << " ms\n";
    return total_sum;
}

void HandleClient(SOCKET clientSocket)
{
    std::cout << "New client\n";
    std::vector<int> array_data;
    int rows = 0, cols = 0, threads_count = 1;
    bool has_data = false;
    std::thread computation_thread;//запускаємо на потоці обрахунок потім
    std::atomic<bool> is_done{false};//атомік бо один міняє один перевіряє
    int final_result = 0;

    try
    {
        while (true)
        {
            unsigned char cmd;//дізнаємось інструкцію
            u_long payload_size;
            //якщо ресів фолз то обрив звязку
            if (!RecvAll(clientSocket, (char*)&cmd, sizeof(cmd))) break;
            if (!RecvAll(clientSocket, (char*)&payload_size, sizeof(payload_size))) break;
            //перевертаєм розмір
            payload_size = ntohl(payload_size);

            if (cmd == CMD_SEND_DATA)
            {
                //читаєм потоки рядки стовпці
                u_long config[3];
                if (!RecvAll(clientSocket, (char*)config, sizeof(config))) break;

                threads_count = static_cast<int>(ntohl(config[0]));
                rows = static_cast<int>(ntohl(config[1]));
                cols = static_cast<int>(ntohl(config[2]));
                //скільки комірок і підлаштовуєм розмір масиву
                size_t total_elements = static_cast<size_t>(rows) * cols;
                array_data.resize(total_elements);
                //берем матрицю і перевертаєм
                if (!RecvAll(clientSocket, (char*)array_data.data(), total_elements * sizeof(int))) break;
                for (size_t i = 0; i < total_elements; ++i) {
                    array_data[i] = ntohl(array_data[i]);
                }

                std::cout << "\nGot matrix " << rows << "x" << cols << ", threads: " << threads_count << std::endl;
                has_data = true;
                unsigned char ack = ACK;
                //відправляєм ак клієнту
                send(clientSocket, (char*)&ack, sizeof(ack), 0);
            }
            else if (cmd == CMD_START)
            {
                if (!has_data) throw std::runtime_error("without data");
                std::cout << "Calculating started\n";
                is_done = false;
                //берем оригінали всіх зміних в функції
                computation_thread = std::thread([&]()
                {
                    final_result = compute_sum_parallel(array_data, rows, cols, threads_count);
                    is_done = true;
                });
                //коли створили потік ідем віддавати ак
                unsigned char ack = ACK_STARTED;
                send(clientSocket, (char*)&ack, sizeof(ack), 0);
            }
            else if (cmd == CMD_GET_STATUS)
            {
                if (is_done)
                {
                    //чекаєм поки закриється
                    if (computation_thread.joinable())
                    {
                        computation_thread.join();
                    }
                    //відправляєм статус
                    unsigned char status = STATUS_DONE;
                    send(clientSocket, (char*)&status, sizeof(status), 0);
                    //результат переводим і відправляєм
                    int net_result = htonl(final_result);
                    send(clientSocket, (char*)&net_result, sizeof(net_result), 0);
                }
                else
                {
                    //якщо не готово то відправляєм це
                    unsigned char status = STATUS_PROCESSING;
                    send(clientSocket, (char*)&status, sizeof(status), 0);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error " << e.what() << "\n";
    }
    //якшо клієнт вийшов то завершуєм поток
    if (computation_thread.joinable())
    {
        computation_thread.join();
    }
    std::cout << "Disconnected.\n";
    closesocket(clientSocket);
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup error\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;//будь яка мережева карта
    serverAddr.sin_port = htons(8080);
    // берем сокет до порту і слухає + робить чергу якщо що
    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    std::cout << "Server on port 8080. Waiting client\n";

    while (true)
    {
        //чекаєм підключення і створюєм сокет клієнта
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        //потік з функцією і детач робим
        if (clientSocket != INVALID_SOCKET)
        {
            std::thread(HandleClient, clientSocket).detach();
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}