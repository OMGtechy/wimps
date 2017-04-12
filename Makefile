all: wimps libpreload.so

wimps: wimps.c
	gcc -g -fPIC wimps.c -o wimps

libpreload.so: preload.c
	gcc -g -shared -fPIC preload.c -o libpreload.so -lrt

clean:
	rm -f libpreload.so wimps
