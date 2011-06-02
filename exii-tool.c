/* Microtouch EXII tool: set the touchscreen's parameters */

/*
   Copyright (C) 2011 Grigory Batalov <gbatalov@crystals.ru>
   Copyright (C) 2011 Crystal Service (http://www.crystals.ru/)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.
*/

/* Special exception is made for the Crystal Service company
   that may use, modify and distribute the program under the terms
   of the BSD license:

Copyright (c) 2011, Crystal Service (http://www.crystals.ru/)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Crystal Service nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Crystal Service BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

char *bin(unsigned char n) {
	static char buf[256];
	static int i = 0;
	unsigned char c;

	if (i + 8 >= sizeof(buf))
		i = 8;
	else
		i += 8;
	buf[i--] = '\0';
	for (c = 0; c < 8; c++, i--)
		buf[i] = '0' + ((n >> c) & 1);
	i += 10;
	return buf + i - 9;
}

void flush(int);
char *response(int, char *, int, int);
char *check(char *, int);

#define CMD_Z		('Z'<<8)
#define CMD_NM		('N'<<8|'M')
#define CMD_OI		('O'<<8|'I')
#define CMD_R		('R'<<8)
#define CMD_RD		('R'<<8|'D')
#define CMD_CX		('C'<<8|'X')
#define CMD_CR		('C'<<8|'R')
#define CMD_FR		('F'<<8|'R')
#define CMD_MS		('M'<<8|'S')
#define CMD_P		('P'<<8|'N'|'O'|'E')
#define CMD_XY		('X'<<8|'Y')
#define MAX_CMD_LEN	7
int main(int argc, char *argv[]) {
	int fd, i = 0, len, cmd, ret = -1;
	struct termios newtio;
	char buf[256], zero[] = "\1Z\xD";
	signed short x = 0, y = 0;
	struct baud {
		int code;
		int num;
	} bauds[] = {
		{ B9600,   9600},
		{ B2400,   2400},
		{ B300,     300},
		{ B600,     600},
		{ B1200,   1200},
		{ B1800,   1800}, 
		{ B4800,   4800}, 
		{ B19200, 19200}, 
		{ B38400, 38400} };

	if (argc < 3) {
		printf("Usage: %s <device> <command>\n", argv[0]);
		puts("Commands are:\n"
		     " Z\t- null command\n"
		     " NM\t- get controller's name\n"
		     " OI\t- get controller's identifier\n"
		     " UV\t- get controller's features\n"
		     " R\t- reset\n"
		     " RD\t- restore defaults\n"
		     " CX\t- extended (interactive) calibration\n"
		     " CR\t- collect raw coordinates\n"
		     " FR\t- raw format\n"
		     " FT\t- tablet format (preferred)\n"
		     " MS\t- stream mode\n"
		     " Ppdsb\t- set communication parameters (see manual)\n"
		     " XY\t- (emulated) wait for screen touch and report x,y coordinates");
		return 0;
	}

	if ((fd = open(argv[1], O_RDWR | O_NOCTTY)) == -1) {
		perror(argv[1]);
		return -1;
	}

	cmd = argv[2][0] << 8 | argv[2][1];

	for (i = 0; i < sizeof(bauds) / sizeof(bauds[0]); i++) {
		bzero(&newtio, sizeof(newtio));
		newtio.c_cflag = bauds[i].code | CS8 | CLOCAL | CREAD;
		newtio.c_iflag = IGNPAR;
		newtio.c_cc[VTIME] = 1;

		if (tcflush(fd, TCIFLUSH) == -1) {
			perror("tcflush");
			goto end;
		}
		if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
			perror("tcsetattr");
			goto end;
		}
		flush(fd);
		if (write(fd, zero, strnlen(zero, MAX_CMD_LEN)) < 1) {
			perror("write error");
			goto end;
		}

		if (response(fd, buf, 0, sizeof(buf)) != NULL && buf[1] == '0')
			break;
	}
	if (i == sizeof(bauds) / sizeof(bauds[0])) {
		fputs("Can't detect connection speed\n", stderr);
		goto end;
	}
	fprintf(stderr, "%d baud connection detected\n", bauds[i].num);

	if (!(cmd & ~CMD_XY)) {
		flush(fd);
		response(fd, buf, 5, 15);

		if (!(buf[0] & 0x80)) {
			fputs("stream error", stderr);
			goto end;
		}
		x = buf[1] & ~0x80;
		x |= (signed short)(buf[2] & ~0x80) << 7;
		y = (buf[3] & ~0x80);
		y |= (signed short)(buf[4] & ~0x80) << 7;
		printf("FT %hd %hd\n", x, y);
		x <<= 2;
		x >>= 5;
		y <<= 2;
		y >>= 5;
		printf("CR %hd %hd\n", x, y);
	} else {

		len = strnlen(argv[2], MAX_CMD_LEN);
		strncpy(buf + 1, argv[2], len);
		buf[0] = 1;
		buf[1 + len] = 0xD;
		buf[1 + len + 1] = '\0';

		tcgetattr(fd, &newtio);
		if (!(cmd & ~CMD_R)) {
			newtio.c_cc[VTIME] = 8;
			tcsetattr(fd, TCSANOW, &newtio);
		}

		if (!(cmd & ~CMD_RD)) {
			newtio.c_cc[VTIME] = 28;
			tcsetattr(fd, TCSANOW, &newtio);
		}

		flush(fd);
		if (write(fd, buf, len + 2) < 1) {
			perror("write error");
			goto end;
		}
		if (response(fd, buf, 1, sizeof(buf)) != NULL)
			if (check(buf, sizeof(buf)) != NULL) {
				puts(buf + 1);
				fflush(stdout);
			}
		if (!(cmd & ~CMD_CX)) {

			while (response(fd, buf, 3, 6) == NULL) ;
			if (check(buf, sizeof(buf)) != NULL) {
				puts(buf + 1);
				fflush(stdout);
			}
			while (response(fd, buf, 3, 6) == NULL) ;
			if (check(buf, sizeof(buf)) != NULL) {
				puts(buf + 1);
				fflush(stdout);
			}
		}
	}
	ret = 0;
 end:
	if (fd >= 0)
		close(fd);
	return ret;
}

void flush(int fd) {
	int res;
	char buf[32];
	while ((res = read(fd, buf, sizeof(buf))) > 0) {
		if (res < 0)
			perror("flush/read");
	}
}

char *response(int fd, char *buf, int min, int max) {
	char *ptr = buf;
	int res;

	// wait for response and read
	do {
		res = read(fd, ptr, max + ptr - buf);
		ptr += res;
	} while (res >= 0 && ptr < buf + min && ptr < buf + max);

	// read the tail
	while (res > 0 && ptr < buf + max) {
		res = read(fd, ptr, max + ptr - buf);
		if (res < 0)
			break;
		ptr += res;
	}

	if (res < 0) {
		perror("response/read");
		return NULL;
	}
	return buf;
}

char *check(char *buf, int max) {
	char *ptr;
	if (buf[0] == 1 && (ptr = memchr(buf + 1, 0xD, max - 1)) != NULL) {
		*ptr = '\0';
		buf[max - 1] = '\0';
		return buf + 1;
	}
	return NULL;
}
