all: scl2trd.tap

scl2trd.tap: scl2trd.c
	zcc +zxn -v -clib=new scl2trd.c -o scl2trd -Cz"--clean" -create-app

clean:
	rm *.bin *.tap *.def