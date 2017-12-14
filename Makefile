all: wimps-trace wimps-read libpreload.so

wimps-trace: wimps_trace.c
	gcc -g -std=gnu99 -fPIC wimps_trace.c -o wimps-trace -Wall -Werror

libpreload.so: preload.c
	gcc -g -std=gnu99 -shared -fPIC preload.c -o libpreload.so -Wall -Werror -lrt

wimps-read: wimps_read.c wimps_read.h
	gcc -g -std=gnu99 -fPIC wimps_read.c -o wimps-read -Wall -Werror

clean:
	rm -f libpreload.so wimps-read wimps-trace
