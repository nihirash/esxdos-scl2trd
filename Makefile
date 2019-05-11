libs = lib/esxdos.c lib/fileDialog.c lib/textUtils.c

all: scl2trd.tap

scl2trd.tap: scl2trd.c
	zcc +zx -lndos -lmzx $(libs) scl2trd.c -o scl2trd -create-app

clean:
	rm *.bin *.tap *.def