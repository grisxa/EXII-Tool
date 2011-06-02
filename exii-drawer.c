/* Microtouch EXII drawer: test the touchscreen */

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
	Window w;

	int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
	int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));
	char message[] = "Draw a line or press Esc";
	XTextItem text = { message, sizeof(message) - 1, 5, None };
	int x, y, lastx, lasty;

		w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
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
		lastx = x = chg.width/2;
		lasty = y = chg.height/2;

		XConfigureWindow(dpy, w, CWX | CWY | CWWidth | CWHeight | CWStackMode, &chg);

		GC gc = XCreateGC(dpy, w, 0, NULL);
		XSetForeground(dpy, gc, whiteColor);

		XSelectInput(dpy, w, StructureNotifyMask | VisibilityChangeMask | KeyPressMask | ButtonPressMask|ButtonReleaseMask|ButtonMotionMask | PointerMotionMask);
		while (1) {

			memset(&e, 0, sizeof(XEvent));
			if (XPending(dpy)) {

				XNextEvent(dpy, &e);
				if (e.type == KeyPress && e.xkey.keycode == 9)
					break;

				if (e.type == ButtonPress || e.type == ButtonRelease) {
					x = e.xbutton.x;
					y = e.xbutton.y;
				}
				if (e.type == MotionNotify) {
					x = e.xmotion.x;
					y = e.xmotion.y;
				}
				XSetForeground(dpy, gc, whiteColor);
				XDrawText(dpy, w, gc,
					  chg.width / 2 - 50, chg.height - 20, &text, 1);
				XDrawLine(dpy, w, gc, lastx, lasty, x, y);				
				XFlush(dpy);
				lastx = x;
				lasty = y;
			}
			usleep(1000);
		}
	return 0;
}
