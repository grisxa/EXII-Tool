CFLAGS = -g -Wall -O2

all: exii-tool exii-calibrator exii-drawer ttyecho

exii-calibrator: exii-calibrator.c
	$(CC) $(CFLAGS) $(LDFLAGS) -lX11 -o $@ $^

exii-drawer: exii-drawer.c
	$(CC) $(CFLAGS) $(LDFLAGS) -lX11 -o $@ $^

clean:
	rm -f exii-tool exii-calibrator exii-drawer ttyecho *.o
