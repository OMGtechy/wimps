all: wimps libpreload.so

wimps: wimps.c
	gcc -g -std=gnu99 -fPIC wimps.c -o wimps -Wall -Werror

libpreload.so: preload.c
	gcc -g -std=gnu99 -shared -fPIC preload.c -o libpreload.so -Wall -Werror -lrt

clean:
	rm -f libpreload.so wimps
