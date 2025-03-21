/**
 * Project 2
 * LC-2K Linker
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXSIZE 500
#define MAXLINELENGTH 1000
#define MAXFILES 6

static inline void printHexToFile(FILE *, int);

typedef struct FileData FileData;
typedef struct SymbolTableEntry SymbolTableEntry;
typedef struct RelocationTableEntry RelocationTableEntry;
typedef struct CombinedFiles CombinedFiles;

struct SymbolTableEntry {
	char label[7];
	char location;
	unsigned int offset;
};

struct RelocationTableEntry {
    unsigned int file;
	unsigned int offset;
	char inst[6];
	char label[7];
};

struct FileData {
	unsigned int textSize;
	unsigned int dataSize;
	unsigned int symbolTableSize;
	unsigned int relocationTableSize;
	unsigned int textStartingLine; // in final executable
	unsigned int dataStartingLine; // in final executable
	int text[MAXSIZE];
	int data[MAXSIZE];
	SymbolTableEntry symbolTable[MAXSIZE];
	RelocationTableEntry relocTable[MAXSIZE];
};

struct CombinedFiles {
	unsigned int textSize;
	unsigned int dataSize;
	unsigned int symbolTableSize;
	unsigned int relocationTableSize;
	int text[MAXSIZE * MAXFILES];
	int data[MAXSIZE * MAXFILES];
	SymbolTableEntry symbolTable[MAXSIZE * MAXFILES];
	RelocationTableEntry relocTable[MAXSIZE * MAXFILES];
};

int main(int argc, char *argv[]) {
	char *inFileStr, *outFileStr;
	FILE *inFilePtr, *outFilePtr; 
	unsigned int i, j, currentTextOffset = 0, currentDataOffset = 0;

    if (argc <= 2 || argc > 8 ) {
        printf("error: usage: %s <MAIN-object-file> ... <object-file> ... <output-exe-file>, with at most 5 object files\n",
				argv[0]);
		exit(1);
	}

	outFileStr = argv[argc - 1];

	outFilePtr = fopen(outFileStr, "w");
	if (outFilePtr == NULL) {
		printf("error in opening %s\n", outFileStr);
		exit(1);
	}

	FileData files[MAXFILES];
	CombinedFiles finalOutput = {0};

  // read in all files and combine into a "master" file
	for (i = 0; i < argc - 2; ++i) {
		inFileStr = argv[i+1];

		inFilePtr = fopen(inFileStr, "r");
		printf("opening %s\n", inFileStr);

		if (inFilePtr == NULL) {
			printf("error in opening %s\n", inFileStr);
			exit(1);
		}

		char line[MAXLINELENGTH];
		unsigned int textSize, dataSize, symbolTableSize, relocationTableSize;

		// parse first line of file
		fgets(line, MAXSIZE, inFilePtr);
		sscanf(line, "%d %d %d %d",
				&textSize, &dataSize, &symbolTableSize, &relocationTableSize);

		files[i].textSize = textSize;
		finalOutput.textSize += textSize;
		files[i].dataSize = dataSize;
		finalOutput.dataSize += dataSize;
		files[i].symbolTableSize = symbolTableSize;
		files[i].relocationTableSize = relocationTableSize;
		files[i].textStartingLine = currentTextOffset;
		

		// read in text section
		int instr;
		for (j = 0; j < textSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			instr = strtol(line, NULL, 0);
			files[i].text[j] = instr;
			finalOutput.text[currentTextOffset] = files[i].text[j];
			currentTextOffset++;
		}

		files[i].dataStartingLine = currentTextOffset + currentDataOffset;

		// read in data section
		int data;
		for (j = 0; j < dataSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			data = strtol(line, NULL, 0);
			files[i].data[j] = data;
			finalOutput.data[currentDataOffset] = files[i].data[j];
			currentDataOffset++;
		}

		// read in the symbol tables
		char label[7];
		char type;
		unsigned int addr;
		for (j = 0; j < symbolTableSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%s %c %d",
					label, &type, &addr);
			files[i].symbolTable[j].offset = addr;
			strcpy(files[i].symbolTable[j].label, label);
			files[i].symbolTable[j].location = type;

			if (!strcmp(label, "Stack")) {
				printf("Error: Stack label defined\n");
				exit(1);
			}
			
			// if location is T or D, can resolve
			if (files[i].symbolTable[j].location == 'T')
				files[i].symbolTable[j].offset += files[i].textStartingLine;
			else if (files[i].symbolTable[j].location == 'D')
				files[i].symbolTable[j].offset += files[i].dataStartingLine;
			
			// Error checking: Duplicate defined global labels
			// might have to check for Stack label as well
			for (unsigned int k = 0; k < finalOutput.symbolTableSize; k++) {
				if (!strcmp(finalOutput.symbolTable[k].label, files[i].symbolTable[j].label) && finalOutput.symbolTable[k].location != 'U') {
					printf("Error: duplicate label\n");
					exit(1);
				}
			}
			finalOutput.symbolTable[finalOutput.symbolTableSize] = files[i].symbolTable[j];
			finalOutput.symbolTableSize++;
		}

		// read in relocation tables
		char opcode[7];
		for (j = 0; j < relocationTableSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%d %s %s",
					&addr, opcode, label);
			files[i].relocTable[j].offset = addr;
			strcpy(files[i].relocTable[j].inst, opcode);
			strcpy(files[i].relocTable[j].label, label);
			files[i].relocTable[j].file	= i;
			finalOutput.relocTable[finalOutput.relocationTableSize] = files[i].relocTable[j];
			finalOutput.relocationTableSize++;
		}
		fclose(inFilePtr);
	} // end reading files

	// *** INSERT YOUR CODE BELOW ***
	//    Begin the linking process
	//    Happy coding!!!

	// Second pass over relocation table to fix the addresses
	for (i = 0; i < finalOutput.relocationTableSize; ++i) {
		unsigned int instrOffset = finalOutput.relocTable[i].offset;
		unsigned int file = finalOutput.relocTable[i].file;
		int newAddr = 0;
		int found = 0;

		// check if label is in symbol table (global label)
		for (unsigned int k = 0; k < finalOutput.symbolTableSize; ++k) {
			if (!strcmp(finalOutput.symbolTable[k].label, finalOutput.relocTable[i].label) && finalOutput.symbolTable[k].location != 'U') {
				char loc = finalOutput.symbolTable[k].location;
				if (loc == 'T')
					newAddr = files[file].textStartingLine + finalOutput.symbolTable[k].offset;
				else
					newAddr = finalOutput.symbolTable[k].offset;
				found = 1;
				break;
			}
		}

		// if not found in symbol table, check if local label
		if (!found) {
			int originalOffset;
			// if label is used in data section
			if (!strcmp(finalOutput.relocTable[i].inst, ".fill")) {
				originalOffset = files[file].data[instrOffset] & 0xFFFF;

				// if line number + original offset < textSize, it's in the text section
				if (originalOffset < files[file].textSize) {
					newAddr = files[file].textStartingLine + originalOffset;
				}
				else {
					newAddr = (originalOffset - files[file].textSize) + files[file].dataStartingLine;
				}
			}
			// if label is used in text section
			else {
				originalOffset = files[file].text[instrOffset] & 0xFFFF;

				if (instrOffset + originalOffset >= files[file].textSize) {
					newAddr = originalOffset + (finalOutput.textSize - files[file].textSize);
				}
			}
		}

		if (!strcmp(finalOutput.relocTable[i].inst, "lw") || !strcmp(finalOutput.relocTable[i].inst, "sw"))
			finalOutput.text[instrOffset + files[file].textStartingLine] = (finalOutput.text[instrOffset + files[file].textStartingLine] & 0xFFFF0000) | (newAddr & 0xFFFF);
		else
			finalOutput.data[instrOffset + (finalOutput.dataSize - files[file].dataSize)] = newAddr;
	}

	unsigned int stackAddr = currentTextOffset + currentDataOffset;
	SymbolTableEntry stackSymbol;
	strcpy(stackSymbol.label, "Stack");
	stackSymbol.location = 'D';
	stackSymbol.offset = stackAddr;
	finalOutput.symbolTable[finalOutput.symbolTableSize] = stackSymbol;
	finalOutput.symbolTableSize++;

	// Write to output file
	for (i = 0; i < currentTextOffset; ++i) {
		printHexToFile(outFilePtr, finalOutput.text[i]);
	}
	for (i = 0; i < currentDataOffset; ++i) {
		printHexToFile(outFilePtr, finalOutput.data[i]);
	}

} // main

// Prints a machine code word in the proper hex format to the file
static inline void 
printHexToFile(FILE *outFilePtr, int word) {
    fprintf(outFilePtr, "0x%08X\n", word);
}
