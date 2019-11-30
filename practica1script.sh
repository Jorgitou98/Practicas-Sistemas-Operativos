#!/bin/bash

if ! test -f mytar -a -x mytar; then
	echo "El fichero no se encuentra o no es ejecutable"
	exit 1
fi

if test -d tmp; then
	rm -r tmp
fi

mkdir tmp
cd tmp

echo "Hello world!" > file1.txt
head /etc/passwd > file2.txt
head -c 1024 /dev/urandom > file3.dat

../mytar -cf filetar.mtar file1.txt file2.txt file3.dat
mkdir out
cp ./filetar.mtar ./out/filetar.mtar
cd out
../../mytar -xf filetar.mtar

if diff file1.txt ../file1.txt && diff file2.txt ../file2.txt && diff file3.dat ../file3.dat; then
	echo "Correct"
	cd ../..
	exit 0
else
	cd ../..
	echo "Los ficheros no son iguales"
	exit 1
fi
