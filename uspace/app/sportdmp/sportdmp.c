/*
 * Copyright (c) 2011 Martin Sucha
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <device/char_dev.h>
#include <errno.h>
#include <ipc/serial_ctl.h>
#include <loc.h>
#include <stdio.h>

#define BUF_SIZE 1

static void syntax_print(void)
{
	fprintf(stderr, "Usage: sportdmp <baud> <device_service>\n");
}

int main(int argc, char **argv)
{
	const char* svc_path = "devices/\\hw\\pci0\\00:01.0\\com1\\a";
	sysarg_t baud = 9600;
	
	if (argc > 1) {
		char *endptr;
		baud = strtol(argv[1], &endptr, 10);
		if (*endptr != '\0') {
			fprintf(stderr, "Invalid value for baud\n");
			syntax_print();
			return 1;
		}
	}
	
	if (argc > 2) {
		svc_path = argv[2];
	}
	
	if (argc > 3) {
		syntax_print();
		return 1;
	}
	
	service_id_t svc_id;
	int rc = loc_service_get_id(svc_path, &svc_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		fprintf(stderr, "Cannot find device service %s\n", svc_path);
		return 1;
	}
	
	async_sess_t *sess = loc_service_connect(EXCHANGE_SERIALIZE, svc_id,
	    IPC_FLAG_BLOCKING);
	if (!sess) {
		fprintf(stderr, "Failed connecting to service %s\n", svc_path);
	}
	
	async_exch_t *exch = async_exchange_begin(sess);
	rc = async_req_4_0(exch, SERIAL_SET_COM_PROPS, baud,
	    SERIAL_NO_PARITY, 8, 1);
	async_exchange_end(exch);
	
	if (rc != EOK) {
		fprintf(stderr, "Failed setting serial properties\n");
		return 2;
	}
	
	uint8_t *buf = (uint8_t *) malloc(BUF_SIZE);
	if (buf == NULL) {
		fprintf(stderr, "Failed allocating buffer\n");
		return 3;
	}
	
	while (true) {
		ssize_t read = char_dev_read(sess, buf, BUF_SIZE);
		if (read < 0) {
			fprintf(stderr, "Failed reading from serial device\n");
			break;
		}
		ssize_t i;
		for (i = 0; i < read; i++) {
			if ((buf[i] >= 32) && (buf[i] < 128))
				putchar((wchar_t) buf[i]);
			else
				putchar('.');
			fflush(stdout);
		}
	}
	
	free(buf);
	return 0;
}
