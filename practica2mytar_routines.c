#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mytar.h"

extern char *use;

/** Copy nBytes bytes from the origin file to the destination file.
 *
 * origin: pointer to the FILE descriptor associated with the origin file
 * destination:  pointer to the FILE descriptor associated with the destination file
 * nBytes: number of bytes to copy
 *
 * Returns the number of bytes actually copied or -1 if an error occured.
 */
int
copynFile(FILE * origin, FILE * destination, int nBytes)
{
	char* c = (char *) malloc(sizeof(char)*nBytes);
	if (c == NULL) return -1;
	int count = fread(c, sizeof(char), nBytes, origin);
	fwrite(c, sizeof(char), count, destination);
	free(c);
	return count;
	// Complete the function
	//return -1;
}

/** Loads a string from a file.
 *
 * file: pointer to the FILE descriptor 
 * 
 * The loadstr() function must allocate memory from the heap to store 
 * the contents of the string read from the FILE. 
 * Once the string has been properly built in memory, the function returns
 * the starting address of the string (pointer returned by malloc()) 
 * 
 * Returns: !=NULL if success, NULL if error
 */
char*
loadstr(FILE * file)
{
	int tam = 0;
	while(getc(file) != '\0') tam++;
	char* c = (char *) malloc(sizeof(char)*tam);
	fseek(file, -tam-1, SEEK_CUR);
	fread(c, sizeof(char), tam +1, file);
	return c;
}

/** Read tarball header and store it in memory.
 *
 * tarFile: pointer to the tarball's FILE descriptor 
 * nFiles: output parameter. Used to return the number 
 * of files stored in the tarball archive (first 4 bytes of the header).
 *
 * On success it returns the starting memory address of an array that stores 
 * the (name,size) pairs read from the tar file. Upon failure, the function returns NULL.
 */
stHeaderEntry*
readHeader(FILE * tarFile, int *nFiles)
{
	fread(nFiles, sizeof(int), 1, tarFile);
	stHeaderEntry* listaPares = (stHeaderEntry*) malloc(sizeof(stHeaderEntry)*(*nFiles));
	for (int i = 0; i < (*nFiles); ++i){
		listaPares[i].name = loadstr(tarFile);
		int tam;
		fread(&tam, sizeof(int), 1, tarFile);
		listaPares[i].size = tam;
	}
	return listaPares;
}

/** Creates a tarball archive 
 *
 * nfiles: number of files to be stored in the tarball
 * filenames: array with the path names of the files to be included in the tarball
 * tarname: name of the tarball archive
 * 
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First reserve room in the file to store the tarball header.
 * Move the file's position indicator to the data section (skip the header)
 * and dump the contents of the source files (one by one) in the tarball archive. 
 * At the same time, build the representation of the tarball header in memory.
 * Finally, rewind the file's position indicator, write the number of files as well as 
 * the (file name,file size) pairs in the tar archive.
 *
 * Important reminder: to calculate the room needed for the header, a simple sizeof 
 * of stHeaderEntry will not work. Bear in mind that, on disk, file names found in (name,size) 
 * pairs occupy strlen(name)+1 bytes.
 *
 */
int
createTar(int nFiles, char *fileNames[], char tarName[])
{
	FILE* file = fopen (tarName, "w");
	if(file == NULL) return EXIT_FAILURE;

	int tamHeader = sizeof(int);
	for (int i = 0; i < nFiles; ++i){
	tamHeader += (strlen(fileNames[i]) + 1 + sizeof(unsigned int));
	}
	fseek(file, tamHeader, SEEK_SET);
	int* tamanos = malloc(sizeof(int)*nFiles);
	for (int i = 0; i < nFiles; ++i){
		FILE* file1 = fopen(fileNames[i], "r");
		if(file1 == NULL) return EXIT_FAILURE;
		int tam = 0;
		int tam1 = copynFile(file1, file, 100);
			while(tam1 != 0){
				tam += tam1;
				tam1 = copynFile(file1, file, 100);
			}
		tamanos[i] = tam;
		fclose(file1);
	}
	fseek(file, 0, SEEK_SET);
	fwrite(&nFiles, sizeof(int), 1, file);
	for(int i = 0; i < nFiles; ++i){
		fwrite(fileNames[i], sizeof(char), strlen(fileNames[i]) + 1, file);
		fwrite(&tamanos[i], sizeof(int), 1, file);
	}
	free(tamanos);
	fclose(file);
	return EXIT_SUCCESS;
}

/** Extract files stored in a tarball archive
 *
 * tarName: tarball's pathname
 *
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First load the tarball's header into memory.
 * After reading the header, the file position indicator will be located at the 
 * tarball's data section. By using information from the 
 * header --number of files and (file name, file size) pairs--, extract files 
 * stored in the data section of the tarball.
 *
 */
int
extractTar(char tarName[])
{
	FILE* file = fopen(tarName, "r");
	if(file == NULL) return EXIT_FAILURE;
	int nFiles;
	stHeaderEntry* header = readHeader(file, &nFiles);
	for (int i = 0; i < nFiles; ++i){
		FILE* file1 = fopen(header[i].name, "w");
		if(file1 == NULL) return EXIT_FAILURE;
		copynFile(file, file1, header[i].size);
		fclose(file1);
	}
	free(header);
	fclose(file);
	return EXIT_SUCCESS;


}

