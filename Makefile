all: wimps-trace libpreload.so

wimps-trace: wimps_trace.c
	gcc -g -std=gnu99 -fPIC wimps_trace.c -o wimps-trace -Wall -Werror

libpreload.so: preload.c
	gcc -g -std=gnu99 -shared -fPIC preload.c -o libpreload.so -Wall -Werror -lrt

clean:
	rm -f libpreload.so wimps-trace
