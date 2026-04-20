#ifndef PROTOCOL_H
#define PROTOCOL_H

enum Command : uint8_t {
    CMD_SEND_DATA = 1,//скидується матриця
    CMD_START = 2,//початок розрахунку
    CMD_GET_STATUS = 3//чек статусу
};

enum Status : uint8_t {
    ACK = 10,
    ACK_STARTED = 11,//запуск потоків
    STATUS_PROCESSING = 12,//ще рахується
    STATUS_DONE = 13
};

#endif