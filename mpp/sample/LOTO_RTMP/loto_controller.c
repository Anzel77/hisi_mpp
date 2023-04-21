/**
 * @file loto_controller.c
 * @author Karl Meng (karlmfloating@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-03-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "loto_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "common.h"
#include "loto_venc.h"

uint16_t calculate_checksum(uint8_t *data, int len) {
    uint32_t sum = 0;
    uint16_t word = 0;
    int i;

    // 计算所有16位的和
    for (i = 0; i < len; i += 2) {
        word = ((data[i] << 8) & 0xFF00) + (data[i + 1] & 0xFF);
        sum += word;
    }

    // 如果数据长度是奇数，则处理最后一个字节
    if (len % 2) {
        sum += (data[len - 1] << 8);
    }

    // 将高16位和低16位相加，直到高16位为0
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // 对和取反，得到校验位
    uint16_t checksum = (uint16_t)(~sum & 0xFFFF);

    return checksum;
}

void serialize_control_packet(uint8_t *buffer, const ControlPacket *packet) {
    buffer[0] = (packet->header.type >> 8) & 0x00FF;
    buffer[1] = packet->header.type & 0xFF;

    buffer[2] = (packet->header.length >> 8) & 0x00FF;
    buffer[3] = packet->header.length & 0xFF;

    buffer[4] = (packet->command >> 8) & 0x00FF;
    buffer[5] = packet->command & 0xFF;

    buffer[6] = (packet->checksum >> 8) & 0x00FF;
    buffer[7] = packet->checksum & 0xFF;
}

void deserialize_control_packet(const uint8_t *buffer, ControlPacket *packet) {
    packet->header.type = ((buffer[0] << 8) & 0xff00) | (buffer[1] & 0x00ff);

    packet->header.length = ((buffer[2] << 8) & 0xff00) | (buffer[3] & 0x00ff);

    packet->command = ((buffer[4] << 8) & 0xff00) | (buffer[5] & 0x00ff);

    packet->checksum = ((buffer[6] << 8) & 0xff00) | (buffer[7] & 0x00ff);
}

#define PORT 8998
#define MAX_PENDING 5
#define MAX_BUF_SIZE 512

extern void get_cover_state(int *cover_state);
extern void set_cover_switch(int *cover_switch);

int create_server_socket(struct sockaddr_in *server_address) {
    int server_socket;

    struct sockaddr_in address;
    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LOGE("Failed to create socket\n");
        return -1;
    }

    // Set server address
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(server_socket);
        LOGE("Failed to bind socket\n");
        return -1;
    }

    // Listen for connections
    if (listen(server_socket, MAX_PENDING) < 0) {
        close(server_socket);
        LOGE("Failed to listen for connections\n");
        return -1;
    }

    memcpy(server_address, (void *)&address, sizeof(struct sockaddr_in));

    return server_socket;
}

void *server_thread(void *arg) {
    int server_socket, client_socket;
    socklen_t address_length;
    struct sockaddr_in server_address;

    int cover_state;
    const int cover_state_on = COVER_ON;
    const int cover_state_off = COVER_OFF;

    // uint8_t recv_buffer[MAX_BUF_SIZE];
    // memset(recv_buffer, 0, MAX_BUF_SIZE);
    uint8_t send_buffer_c[MAX_BUF_SIZE];
    memset(send_buffer_c, 0, MAX_BUF_SIZE);

    int resend_state = 0;

    do {
        server_socket = create_server_socket(&server_address);
    } while (server_socket < 0);
    LOGI("Waiting for connections...\n");

    address_length = sizeof(server_address);

    while (1) {
        // Accept connection from client
        if ((client_socket = accept(server_socket, (struct sockaddr *)&server_address, &address_length)) < 0) {
            LOGE("Failed to accept connection\n");
            continue;
        }
        LOGD("Client connected: %s:%d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

        while (1) {
            uint8_t recv_buffer[MAX_BUF_SIZE];
            memset(recv_buffer, 0, MAX_BUF_SIZE);
            uint8_t send_buffer[MAX_BUF_SIZE];
            memset(send_buffer, 0, MAX_BUF_SIZE);
            ControlPacket recv_packet;
            memset(&recv_packet, 0, sizeof(recv_packet));
            ControlPacket send_packet;
            memset(&send_packet, 0, sizeof(send_packet));

            if (recv(client_socket, recv_buffer, MAX_BUF_SIZE, 0) <= 0) {
                LOGE("Client disconnected\n");
                break;
            }
            LOGD("recv_buffer: \n");
            for (int i = 0; i < sizeof(ControlPacket); i++) {
                LOGD("%#x\n", recv_buffer[i]);
            }

            // 将数组中的内容保存近本地结构体
            deserialize_control_packet(recv_buffer, &recv_packet);

            LOGD("length = %d\n", recv_packet.header.length);

            uint16_t checksum = calculate_checksum(recv_buffer, sizeof(ControlPacket) - 2);
            if (checksum != recv_packet.checksum) {
                LOGE("the received packet is wrong! Please resend.\n");
                sprintf(send_buffer, "RESEND");

                if (send(client_socket, send_buffer, strlen(send_buffer), 0) < 0) {
                    LOGE("Client disconnected\n");
                    break;
                }

                break;
            } else {
                if (recv_packet.header.type == PACK_TYPE_CTRL) {
                    if (recv_packet.command == CONTROL_ADD_COVER) {
                        set_cover_switch(&cover_state_on);
                        sprintf(send_buffer, "COVER ON");
                    } else if (recv_packet.command == CONTROL_RMV_COVER) {
                        set_cover_switch(&cover_state_off);
                        sprintf(send_buffer, "COVER OFF");
                    } else if (recv_packet.command == CONTROL_GET_COVER_STATE) {
                        get_cover_state(&cover_state);
                        send_packet.header.type = PACK_TYPE_INFO;
                        send_packet.header.length = sizeof(ControlPacket) - sizeof(PacketHeader);
                        if (cover_state == COVER_ON) {
                            send_packet.command = MESSAGE_COVER_ON;
                        } else if (cover_state == COVER_OFF) {
                            send_packet.command = MESSAGE_COVER_OFF;
                        }
                        send_packet.checksum = calculate_checksum((uint8_t*)&send_packet, sizeof(send_packet) - 2);
                        serialize_control_packet(send_buffer, &send_packet);
                    }
                }
            }
            
            if (send(client_socket, send_buffer, strlen(send_buffer), 0) < 0) {
                LOGE("Client disconnected\n");
                break;
            }
            // sleep(MAX_PENDING);
        }

        close(client_socket);
    }

    close(server_socket);
    LOGD("close server socket\n");

    pthread_exit(NULL);
}

int create_client_socket(const char *server_ip_addr) {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip_addr, &server_address.sin_addr);
    // serv_addr.sin_addr.s_addr = inet_addr(server_ip_addr);
    server_address.sin_port = htons(PORT);


    int connection_status = connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    if (connection_status == -1) {
        LOGE("connect failed!\n");
        return -1;
    }

    return client_socket;
}

void *client_thread(void *arg) {
    int ret;
    int client_socket;
    const char *server_ip_addr = (const char *)arg;

    int retry = 0;
    const int MAX_RETRIES = 5;
    int counter = 0;

    uint8_t send_buffer[MAX_BUF_SIZE] = {0};
    uint8_t recv_buffer[MAX_BUF_SIZE] = {0};

    int cover_state = COVER_OFF;

    memset(send_buffer, 0, MAX_BUF_SIZE);

    ControlPacket add_cover_pack;
    ControlPacket rmv_cover_pack;
    
    add_cover_pack.header.type = 0x01;
    add_cover_pack.header.length = sizeof(ControlPacket) - sizeof(PacketHeader);
    add_cover_pack.command = CONTROL_ADD_COVER;
    add_cover_pack.checksum = calculate_checksum((uint8_t*)&add_cover_pack, sizeof(add_cover_pack) - 2);

    rmv_cover_pack.header.type = 0x01;
    rmv_cover_pack.header.length = sizeof(ControlPacket) - sizeof(PacketHeader);
    rmv_cover_pack.command = CONTROL_RMV_COVER;
    rmv_cover_pack.checksum = calculate_checksum((uint8_t*)&rmv_cover_pack, sizeof(rmv_cover_pack) - 2);
    

    while (retry < MAX_RETRIES) {
        client_socket = create_client_socket(server_ip_addr);

        memset(send_buffer, 0, MAX_BUF_SIZE);
        if (cover_state == COVER_OFF) {
            serialize_control_packet(send_buffer, &add_cover_pack);
            printf("checksum = %d\n", add_cover_pack.checksum);
        } else if (cover_state == COVER_ON) {
            serialize_control_packet(send_buffer, &rmv_cover_pack);
            printf("checksum = %d\n", rmv_cover_pack.checksum);
        }

        // printf("size of PacketHeader = %d\n", sizeof(PacketHeader));
        // printf("size of ControlPacket = %d\n", sizeof(ControlPacket));
        // for (int i = 0; i < sizeof(ControlPacket); i++) {
        //     printf("%#x\n", send_buffer[i]);
        // }

        ret = send(client_socket, send_buffer, sizeof(ControlPacket), 0);
        if (ret == -1) {
            printf("Error: Failed to send message.\n");
            close(client_socket);
            retry++;
            continue;
        }

        memset(recv_buffer, 0, MAX_BUF_SIZE);
        ret = recv(client_socket, recv_buffer, sizeof(recv_buffer), 0);
        if (ret == -1) {
            printf("Error: Failed to receive massage.\n");
            close(client_socket);
            retry++;
            continue;
        }

        printf("[CLIENT] Received: %s\n", recv_buffer);
        
        if (strncmp(recv_buffer, "COVER ON", 8) == 0) {
            cover_state = COVER_ON;
        } else if (strncmp(recv_buffer, "COVER OFF", 9) == 0) {
            cover_state = COVER_OFF;
            counter++;
            printf("counter = %d\n", counter);
        } else if (strncmp(recv_buffer, "RESEND", 6) == 0) {
            printf("recv_mess: %s\n", recv_buffer);
            retry++;
            close(client_socket);
            continue;
        }

        close(client_socket);
        sleep(MAX_PENDING);
    }

    pthread_exit(NULL);
}

void *check_server_socket(void *arg) {

}

void *check_client_socket(void *arg) {

}