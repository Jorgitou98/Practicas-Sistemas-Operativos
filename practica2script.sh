#!/bin/bash

if test -d temp; then
	rm -r temp
fi

mkdir temp
cp ./src/fuseLib.c ./temp/fuseLib.c
cp ./src/fuseLib.c ./mount-point/fuseLib.c

cp ./src/myFS.h ./temp/myFS.h
cp ./src/myFS.h ./mount-point/myFS.h

./my-fsck-static-64 virtual-disk
if diff ./src/fuseLib.c ./mount-point/fuseLib.c && diff ./src/myFS.h ./mount-point/myFS.h; then
	echo "Correct"
else
	echo "Los ficheros no son iguales"
fi

truncate -o -s -1 ./temp/fuseLib.c
truncate -o -s -1 ./mount-point/fuseLib.c

./my-fsck-static-64 virtual-disk
if diff ./temp/fuseLib.c ./mount-point/fuseLib.c; then
	echo "Correct"
else
	echo "Los ficheros no son iguales"
fi

cp ./src/myFS.c ./mount-point/myFS.c
./my-fsck-static-64 virtual-disk

if diff ./src/myFS.c ./mount-point/myFS.c; then
	echo "Correct"
else
	echo "Los ficheros no son iguales"
fi

truncate -o -s +1 ./temp/myFS.h
truncate -o -s +1 ./mount-point/myFS.h

./my-fsck-static-64 virtual-disk
if diff ./temp/myFS.h ./mount-point/myFS.h; then
	echo "Correct"
else
	echo "Los ficheros no son iguales"
fi