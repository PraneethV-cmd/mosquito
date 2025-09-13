#ifndef MQTT_H
#define MQTT_H 

#include<iostream>

/*

MQTT header has 
- fixed header (very much needed)
- variable header (optional)
- payload (optional)

# Fixed Header

 | Bit    | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 |--------|---------------|---------------|
 | Byte 1 | MQTT type     |  Flags        |
 |--------|-------------------------------|
 | Byte 2 |                               |
 |  .     |      Remaining Length         |
 |  .     |                               |
 | Byte 5 |                               |


in the flags: 
one bit is for DUP FLAG: used when the message is sent more than once 
QoS level bit: for AT_MOST_ONCE, AT_LEAST_ONCE, EXACTLY_ONCE (0, 1, 2) respectively
Retain flag: if a message gotta be retained, because when a message is published
it is saved and any clients that connect in the future, they can also get it 
and later it can be updated with another retained message

*/

/*
 
NOTE: client to server will be represented as CS 
	Server to client will be represented as SC 
	for the direction of the flow of the messages

*/

#define MQTT_HEADER_LEN 2
#define MQTT_ACK_LEN 4

#define CONNACK_BYTE 0x20 // SC, connect acknowledgement  
#define PUBLISH_BYTE 0x30 // SC or CS, publishing the message 
#define PUBACK_BYTE 0x40 // CS or SC, publish acknowledgement
#define PUBREC_BYTE 0x50 // CS or SC, publish received, assured delivery part 1
#define PUBREL_BYTE 0x60 // CS or SC, publish release, assured delivery part 2
#define PUBCOMP_BYTE 0x70 // CS or SC, publish complete, assured delivery part 3
#define SUBACK_BYTE 0x90 // SC, subscribe acknowledgement
#define UNSUBACK_BYTE 0xB0 // SC, unsubscribe acknowledgement
#define PINGRESP_BYTE 0xD0 // SC, PING response 


enum packet_type {
	CONNECT = 1,
	CONNACK = 2,
	PUBLISH = 3,
	PUBACK = 4,
	PUBREC = 5,
	PUBREL = 6,
	PUBCOMP = 7,
	SUBSCRIBE = 8,
	SUBACK = 9,
	UNSUBSCRIBE = 10,
	UNSUBACK = 11,
	PINGREQ = 12,
	PINGRESP = 13,
	DISCONNECT = 14 
};

enum qos_level {
	AT_MOST_ONCE,
	AT_LEAST_ONCE, 
	EXACTLY_ONCE
};

union mqtt_header {
	unsigned char byte;
	struct {
		unsigned retain: 1;
		unsigned qos: 2;
		unsigned dup: 1;
		unsigned type: 4;
	} bytes; // total 8 bits = 1 byte
};

struct mqtt_connect {
	union mqtt_header header;
	union {
		unsigned char byte;
		struct {
			int reserved: 1;
			unsigned clean_session: 1;
			unsigned will: 1;
			unsigned will_qos: 2;
			unsigned will_retain: 1;
			unsigned username: 1;
		} bits;
	};
	struct {
		unsigned short keepalive;
		unsigned char *client_id;
		unsigned char *username;
		unsigned char *password;
		unsigned char *will_topic;
		unsigned char *will_message;
	} payload;
};

struct mqtt_connack {
	union mqtt_header header;
	union {
		unsigned char byte;
		struct {
			unsigned session_present: 1;
			unsigned reserved: 7;
		} bits;
	};
};

struct mqtt_subscribe {
	union mqtt_header header;
	unsigned short pkt_id;
	unsigned short tuples_len;
	struct {
		unsigned short topic_len;
		unsigned char *topic;
		unsigned qos;
	} *tuples;
};

struct mqtt_unsubscribe {
	union mqtt_header header;
	unsigned short pkt_id;
	unsigned short tuples_len;
	struct {
		unsigned short topic_len;
		unsigned char *topic;
	} *tuples;
};

struct mqtt_suback {
	union mqtt_header header;
	unsigned short pkt_id;
	unsigned short rcslen;
	unsigned char *rcs;
};

struct mqtt_publish {
	union mqtt_header header;
	unsigned short pkt_id;
	unsigned short topiclen;
	unsigned char *topic;
	unsigned short payloadlen;
	unsigned char *payload;
};

struct mqtt_ack {
	union mqtt_header header;
	unsigned short pkt_id;
};

typedef struct mqtt_ack mqtt_puback;
typedef struct mqtt_ack mqtt_pubrec;
typedef struct mqtt_ack mqtt_pubrel;
typedef struct mqtt_ack mqtt_pubcomp;

typedef struct mqtt_ack mqtt_unsuback;
typedef struct mqtt_ack mqtt_pingreq;
typedef struct mqtt_ack mqtt_pingresp;
typedef struct mqtt_ack mqtt_disconnect;


union mqtt_packet {
	struct mqtt_ack ack;
	union mqtt_header header;
	struct mqtt_connect connect;
	struct mqtt_connack connack;
	struct mqtt_suback suback;
	struct mqtt_publish publish;
	struct mqtt_subscribe subscribe;
	struct mqtt_unsubscribe unsubscribe;
};
