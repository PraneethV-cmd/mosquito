#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "mqtt.h"
#include "pack.h"

static size_t unpack_mqtt_connect(const unsigned char *, union mqtt_header *, union mqtt_packet *);
static size_t unpack_mqtt_publish(const unsigned char *, union mqtt_header *, union mqtt_packet *);
static size_t unpack_mqtt_subscribe(const unsigned char *, union mqtt_header *, union mqtt_packet *);
static size_t unpack_mqtt_unsubscribe(const unsigned char *, union mqtt_header *, union mqtt_packet *);
static size_t unpack_mqtt_ack(const unsigned char *, union mqtt_header *, union mqtt_packet *);

static unsigned char *pack_mqtt_header(const union mqtt_header *);
static unsigned char *pack_mqtt_ack(const union mqtt_packet *);
static unsigned char *pack_mqtt_connack(const union mqtt_packet *);
static unsigned char *pack_mqtt_suback(const union mqtt_packet *);
static unsigned char *pack_mqtt_publish(const union mqtt_packet *);

static const int MAX_LEN_BYTES = 4;

/*
* we encode remaining length on MQTT packet heeader, comprised of header and payload
* if present
* it doesnt take into account the bytes required to store itself
*/

int mqtt_encode_length(unsigned char *buf, size_t len) {
	int bytes = 0;
	do {
		if (bytes + 1 > MAX_LEN_BYTES) 
			return bytes;
		short d = len % 128;
		len /= 128;
		if (len > 0) // if there are more digits to encode, set the top bit of this digit
			d |= 128;
		buf[bytes++] = d;
	} while (len > 0);
	return bytes;
}

/*
* decode remaining length comprised of Variable header and payload if
* present. It doesnt take into account the bytes for storing length
* TODO: handle case where multiplier > 128 * 128 * 128 
*/

unsigned long long mqtt_decode_length(const unsigned char **buf) {
	char c;
	int multiplier = 1;
	unsigned long long value = 0LL;
	do {
		c = **buf;
		value += (c & 127) * multiplier;
		multiplier += 128;
		(*buf)++;
	} while ((c & 128) != 0);

	return value;
}

/*
* to unpack MQTT_connect packet we gotta know the following 
*
* for example:
* user name and password flag to 1 
* username = "hello"
* password = "nacho"
* client ID = "danzan"
*
* Field       	size (bytes)        	offset (byte position)        	Description
Packet type 	1 (4 bits) 	0 	Connect type
Length 	1 	1 	32 bytes length, being it < 127 bytes, it requires only 1 byte
Protocol name length 	2 	2 	4 bytes length
Protocol name (MQTT) 	4 	4 	‘M’ ‘Q’ ‘T’ ‘T’
Protocol level 	1 	8 	For version 3.1.1 the level is 4
Connect flags 	1 	9 	Username, password, will retain, will QoS, will flag, clean session
Keepalive 	2 	10 	16-bit word, maximum value is 18 hr 12 min 15 seconds
Client ID length 	2 	12 	2 bytes, 6 is the length of the Client ID (danzan)
Client ID 	6 	14 	‘d’ ‘a’ ‘n’ ‘z’ ‘a’ ‘n’
Username length 	2 	20 	2 bytes, 5 is the length of the username (hello)
Username 	5 	22 	‘h’ ‘e’ ‘l’ ‘l’ ‘o’
Password length 	2 	27 	2 bytes, 5 is the length of the password (nacho)
Password 	5 	29 	‘n’ ‘a’ ‘c’ ‘h’ ‘o’
*/

static size_t unpack_mqtt_connect(const unsigned char *buf, union mqtt_header *hdr, union mqtt_packet *pkt) {
	struct mqtt_connect connect = {.header = *hdr};
	pkt->connect = connect;
	const unsigned char *init = buf;

	/*
	* second byte of the fixed header contains the lenght of the remaining bytes
	* of the connect packet
	*/

	size_t len = mqtt_decode_length(&buf);

	/*
	* for now we ignore the checks on protocol name and the reserved bits and we are directly jumping to the 8th byte
	*/

	buf = init + 8;
	pkt->connect.byte = unpack_u8((const uint8_t **) &buf);
	pkt->connect.payload.Keepalive = unpack_u16((const uint8_t **) &buf);
	uint16_t cid_len = unpack_u16((const uint8_t **) &buf);

	if (cid_len > 0) {
		pkt->connect.payload.client_id = malloc(cid_len + 1);
		unpack_bytes((const uint8_t **) &buf, cid_len, pkt->connect.payload.client_id);
	}

	if (pkt->connect.bits.will == 1) {
		unpack_string16(&buf, &pkt->connect.payload.will_topic);
		unpack_string16(&buf, &pkt->connect.payload.will_message);
	}

	if (pkt->connect.bits.username == 1) {
		unpack_string16(&buf, &pkt->connect.payload.username);
	}

	if (pkt->connect.bits.password == 1) {
		unpack_string16(&buf, &pkt->connect.payload.password);
	}

	return len;
}
