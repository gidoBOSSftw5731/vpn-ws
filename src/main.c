#include "vpn-ws.h"

static struct option vpn_ws_options[] = {
	{"tuntap", required_argument, 0, 1 },
	{NULL, 0, 0, 0}
};

int main(int argc, char *argv[], char **environ) {
	int option_index = 0;
	int event_queue = -1;
	int server_fd = -1;
	int tuntap_fd = -1;
	char *tuntap_name = "vpn-ws\%d";

	for(;;) {
		int c = getopt_long(argc, argv, "", vpn_ws_options, &option_index);
		if (c < 0) break;
		switch(c) {
			// tuntap
			case 1:
				tuntap_name = optarg;
				break;
			case '?':
				break;
			default:
				fprintf(stderr, "error parsing arguments\n");
				vpn_ws_exit(1);
		}
	}

	if (optind < argc) {
		vpn_ws_conf.server_addr = argv[optind];
	}

	if (!vpn_ws_conf.server_addr) {
		fprintf(stderr, "you need to specify a socket address\n");
                vpn_ws_exit(1);
	}

	server_fd = vpn_ws_bind(vpn_ws_conf.server_addr);
	if (server_fd < 0) {
		vpn_ws_exit(1);
	}

	event_queue = vpn_ws_event_queue(256);
	if (event_queue < 0) {
		vpn_ws_exit(1);
	}

	tuntap_fd = vpn_ws_tuntap(tuntap_name);
	if (tuntap_fd < 0) {
		vpn_ws_exit(1);
	}

	if (vpn_ws_event_add_read(event_queue, server_fd)) {
		vpn_ws_exit(1);
	}

	if (vpn_ws_event_add_read(event_queue, tuntap_fd)) {
		vpn_ws_exit(1);
	}

	void *events = vpn_ws_event_events(64);
	if (!events) {
		vpn_ws_exit(1);
	}

	for(;;) {
		int ret = vpn_ws_event_wait(event_queue, events);
		if (ret <= 0) break;

		int i;
		for(i=0;i<ret;i++) {
			int fd = vpn_ws_event_fd(events, i);
			// a new connection ?
			if (fd == server_fd) {
				vpn_ws_peer_accept(event_queue, server_fd);
				continue;
			}

			// on peer modification, exit the cycle
			if (vpn_ws_manage_fd(event_queue, fd)) break;
		}
	}

	return 0;
}