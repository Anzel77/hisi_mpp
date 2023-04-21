
#ifndef LOTO_CONTROLLER_H
#define LOTO_CONTROLLER_H

#include <netinet/in.h>
#include <stdint.h>

// cover_state
#define COVER_OFF   0x00
#define COVER_ON    0x01
#define COVER_NULL  0xFF

// message
#define MESSAGE_COVER_ON            0x00A0
#define MESSAGE_COVER_OFF           0x00A1
#define MESSAGE_USER_OFFLINE        0x00B0
#define MESSAGE_USER_ONLINE         0x00B1

// commond
#define CONTROL_SET_PUSH_URL        0x01A0
#define CONTROL_ADD_COVER           0x01A1
#define CONTROL_RMV_COVER           0x01A2
#define CONTROL_GET_COVER_STATE     0x01A3
#define CONTROL_REQUEST_RESEND      0x01C0

#define PACK_TYPE_INFO              0x0000
#define PACK_TYPE_CTRL              0x0101

typedef struct PacketHeader {
    uint16_t type;      // 0x0000: string info; 0x0101: control info
    uint16_t length;
} PacketHeader;

typedef struct ControlPacket {
    PacketHeader header;
    uint16_t command;
    uint16_t checksum;
} ControlPacket;

void *server_thread(void *arg);

void *client_thread(void *arg);

#endif