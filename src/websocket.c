#include "vpn-ws.h"

int vpn_ws_websocket_pong(vpn_ws_peer *peer) {
	return -1;
}

int vpn_ws_websocket_parse(vpn_ws_peer *peer) {
	if (peer->pos < 2) return 0;

	uint8_t byte1 = peer->buf[0];
        uint8_t byte2 = peer->buf[1];	

	uint8_t opcode = byte1 & 0xf;
	uint8_t has_mask = byte2 >> 7;
        uint64_t pktsize = byte2 & 0x7f;

	uint64_t needed = 2;

	// 16bit len
	if (pktsize == 126) {
		needed += 2;
		if (peer->pos < needed) return 0;
		pktsize = vpn_ws_be16(peer->buf + 2);
	}
	// 64bit
	else if (pktsize == 127) {
		needed += 8;
		if (peer->pos < needed) return 0;
		pktsize = vpn_ws_be64(peer->buf + 2);
	}

	if (has_mask) {
		needed += 4;
		if (peer->pos < needed) return 0;
	}

	switch(opcode) {
		// 0/1/2 -> forward
		case 0:
		case 1:
		case 2:
			return 1;
		// 8 -> close connection
		case 8:
			return -1;
		// 9 -> send back a pong
		case 9:
			return vpn_ws_websocket_pong(peer);
		// 10 -> ignore	
		default:
			return -1;
	}

	// never here
	return -1;
}