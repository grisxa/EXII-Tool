/* Simple serial port communication program */

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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

char * bin(unsigned char n) {
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

int	main(int argc, char *argv[]) {
	unsigned char buf;
	int i, n, fd, baud, flags;
	fd_set rd, wr;
        struct termios tio;
	struct baud {
		int code;
		int num;
		} bauds[] = {
			{ B300,   300 },
			{ B600,   600 },
			{ B1200,  1200 },
			{ B1800,  1800 },
			{ B2400,  2400},
			{ B4800,  4800},
			{ B9600,  9600},
			{ B19200, 19200},
			{ B38400, 38400} };
	struct frame {
		char *str;
		int flags;
		} frames[] = {
			{ "7N1", CS7 },
			{ "7N2", CS7 | CSTOPB },
			{ "7O1", CS7 | PARENB | PARODD },
			{ "7O2", CS7 | PARENB | PARODD | CSTOPB },
			{ "7E1", CS7 | PARENB },
			{ "7E2", CS7 | PARENB | CSTOPB },
			{ "8N1", CS8 },
			{ "8O1", CS8 | PARENB | PARODD },
			{ "8E1", CS8 | PARENB } };

	if (argc < 4) {
		printf( "Usage:   %s <device> <baud> <frame>\n"
			"Example: %s /dev/ttyS0 9600 8N1\n", argv[0], argv[0]);
		return 0;
	}
	baud = atoi(argv[2]);

	n = sizeof(bauds)/sizeof(bauds[0]);
	for (i = 0; i < n; i++) {
		if (baud == bauds[i].num)
			break;
	}
	if (i < n)
		baud = bauds[i].code;
	else {
		printf("Unknown baudrate %d\n", baud);
		return -1;
	}

	n = sizeof(frames)/sizeof(frames[0]);
	for (i = 0; i < n; i++) {
		if (!strncmp(argv[3], frames[i].str, 3))
			break;
	}
	if (i < n)
		flags = frames[i].flags;
	else {
		printf("Unknown frame format %s\n", argv[2]);
		return -1;
	}

	if ((fd = open(argv[1], O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1) {
		perror("open");
		return -1;
	}

	bzero (&tio, sizeof(tio));
	tio.c_cflag = baud | flags | CLOCAL | CREAD;
	if (tcsetattr(fd, TCSANOW, &tio) == -1) {
		perror("tcsetattr");
		close(fd);
		return -1;
	}
	printf("using %s with baud 0x%X and flags 0%o\n", argv[1], baud, flags);

	do {
		FD_ZERO(&rd);
		FD_ZERO(&wr);
		FD_SET(0, &rd);
		FD_SET(fd, &rd);
		FD_SET(fd, &wr);

		n = select (fd + 1, &rd, &wr, NULL, NULL);

		if (n == 0)
			continue;
		if (n == -1)
			break;
		if (FD_ISSET(fd, &rd)) {
			if ((n = read(fd, &buf, sizeof(buf))) == -1) {
				perror("fd read");
				continue;
			}
			printf("0x%X\tb%s\t%c\n", buf, bin(buf), (buf >= 32 && buf < 128) ? buf : '.');
			if (buf == 'q')
				break;
		}
		if (FD_ISSET(0, &rd)) {
			if ((n = read(0, &buf, sizeof(buf))) == -1) {
				perror("stdin read");
				continue;
			}
			switch (buf) {
				case '.': buf =   1; break;
				case ';': buf = 0xD; break;
			}
			if ((n = write(fd, &buf, sizeof(buf))) == -1) {
				perror("fd write");
				continue;
			}
		}
	} while (buf != 'q');
	printf("error: %d\n", errno);
	if (n == -1)
		perror("read");

	close(fd);
	return 0;
}
