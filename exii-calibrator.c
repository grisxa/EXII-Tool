/* Microtouch EXII calibrator: set the touchscreen's margins */

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

#include <X11/Xlib.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include "mwmhints.h"

void DrawAim(Display *, Window, GC, int, int, char, int color);

int main(int argc, char *argv[]) {

	MwmHints hints;
	XWindowChanges chg;
	Display *dpy = XOpenDisplay(NULL);
	XEvent e;
	assert(dpy);
	char reply[32];

	int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
	int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));
	int color[2] = { whiteColor, whiteColor };
	int fds[2], seq = 0, n, status, update = 0;
	char message[] = "Push the center of aim 1 and 2 for 2 seconds or press Esc";
	fd_set rd;
	pid_t pid;
	struct timeval tv;
	XTextItem text = { message, sizeof(message) - 1, 5, None };

	if (argc < 2) {
		printf("Usage: %s <device>\n", argv[0]);
		return 0;
	}

	if (pipe(fds) < 0) {
		perror("pipe");
		return -1;
	}
	if ((pid = fork()) < 0) {
		perror("fork");
		return -1;
	}
	if (pid) {
		close(fds[1]);
		Window w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
					       400, 400, 0, blackColor, blackColor);

		hints.flags = MWM_HINTS_DECORATIONS;
		hints.decorations = MWM_DECOR_NONE;
		Atom props = XInternAtom(dpy, "_MOTIF_WM_HINTS", True);
		XChangeProperty(dpy, w, props, props, 32, PropModeReplace,
				(unsigned char *)&hints, MWM_HINTS_ELEMENTS);

		XMapWindow(dpy, w);

		memset(&chg, 0, sizeof(chg));
		chg.x = chg.y = 0;
		chg.width = DisplayWidth(dpy, 0);
		chg.height = DisplayHeight(dpy, 0);
		chg.stack_mode = TopIf;

		XConfigureWindow(dpy, w, CWX | CWY | CWWidth | CWHeight | CWStackMode, &chg);

		GC gc = XCreateGC(dpy, w, 0, NULL);
		XSetForeground(dpy, gc, whiteColor);

		XSelectInput(dpy, w, StructureNotifyMask | VisibilityChangeMask | KeyPressMask);

		while (seq < 3) {

			FD_ZERO(&rd);
			FD_SET(fds[0], &rd);
			tv.tv_sec = 0;
			tv.tv_usec = 100;
			waitpid(pid, &status, WNOHANG);
			if (WIFEXITED(status))
				break;
			if (select(fds[1] + 1, &rd, NULL, NULL, &tv) < 0)
				perror("select");
			if (FD_ISSET(fds[0], &rd)) {
				if ((n = read(fds[0], &reply, sizeof(reply))) == -1)
					perror("pipe read");
				if (n > 0)
					switch (seq) {
					case 0:
						if (reply[0] == '0')
							seq++;
						else
							seq = 3;
						break;
					case 1:
						if (reply[0] == '1') {
							seq++;
							color[0] = blackColor;
							update = 1;
						} else
							seq = 3;
						break;
					case 2:
						if (reply[0] == '1') {
							seq++;
							color[1] = blackColor;
							update = 1;
						} else
							seq = 3;
						break;
					}
			}
			memset(&e, 0, sizeof(XEvent));
			if (XPending(dpy)) {

				XNextEvent(dpy, &e);
				if (e.type == KeyPress && e.xkey.keycode == 9)
					break;
			}
			if (update
			    || e.type == MapNotify
			    || e.type == VisibilityNotify
			    || e.type == ConfigureNotify || e.type == KeyPress) {
				XSetForeground(dpy, gc, whiteColor);
				DrawAim(dpy, w, gc, chg.width / 8,
					chg.height * 7 / 8, '1', color[0]);
				DrawAim(dpy, w, gc, chg.width * 7 / 8,
					chg.height / 8, '2', color[1]);
				XSetForeground(dpy, gc, whiteColor);
				XDrawText(dpy, w, gc,
					  chg.width / 2 - 200, chg.height / 2, &text, 1);
				XFlush(dpy);
				update = 0;
			}
			usleep(1000);
		}
	} else {
		close(fds[0]);
		close(1);
		if (dup2(fds[1], 1) < 0) {
			perror("dup2");
			return -1;
		}
		if (execl("exii-tool", "exii-tool", argv[1], "CX", NULL) < 0)
			perror("execl");
	}
	return 0;
}

#define R 40
void DrawAim(Display * dpy, Window w, GC gc, int x, int y, char c, int color) {
	XTextItem text = { &c, 1, 5, None };

	XSetForeground(dpy, gc, color);
	XDrawArc(dpy, w, gc, x - R / 2, y - R / 2, R, R, 0, 23040);
	XDrawLine(dpy, w, gc, x - R / 2 - 10, y, x + R / 2 + 10, y);
	XDrawLine(dpy, w, gc, x, y - R / 2 - 10, x, y + R / 2 + 10);
	XDrawText(dpy, w, gc, x, y - 4, &text, 1);
}
