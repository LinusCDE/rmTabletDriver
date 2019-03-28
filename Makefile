rebuild:
	make clean build

clean:
	rm -f tabletDriver

build:
	gcc tabletDriver.c -o tabletDriver
