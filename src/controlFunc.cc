#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"

// Input: Pointer to memspace, length of memspace
// Output: Prints a map of the memspace, no return
void map(char* mem, int len)
{
	for(int i = 0; i < len; i++)
		printf("%02x ", mem[i]);	
	printf("\n");
	for(int i = 0; i < len; i++)
	{
		if(i > 0 && mem[i - 1] != 0 && mem[i] == 0)
			printf("%c%c ", '\\', '0');
		else
			printf("%2c ", mem[i]);	
	}
	printf("\n");
}
// Input: Pointer to memspace, pointer to symbol table, pointer to freespace heap, prime number being used, var name, length of string, value
// Output: Memspace contains value, symbol table contains symbol and location, space of variable is removed from heap
void myMallocInt(char* mem, struct symbolTableEntry* symTable, struct heapEntry* freeHeap, int prime, char const* varName, unsigned int value)
{
	struct heapEntry topOfHeap = heapExtractMax(freeHeap);
	
	if(topOfHeap.blockSize >= 4)
	{
		//Update the symbol table
		hashTableInsert(symTable, prime, varName, INT, topOfHeap.offset, 4);

		//Place the variable in the memspace
		*((unsigned int*) &mem[topOfHeap.offset]) = value; 

		//Update the free heap
		maxHeapInsert(freeHeap, topOfHeap.blockSize - 4, topOfHeap.offset + 4);
	}	
	else
	{
		printf("ERROR: Not enough space to allocate variable.");
		return;
	}
}
// Input: Pointer to memspace, pointer to symbol table, pointer to freespace heap, prime number being used, var name, length of string, value
// Output: Memspace contains value, symbol table contains symbol and location, space of variable is removed from heap
void myMallocChar(char* mem, struct symbolTableEntry* symTable, struct heapEntry* freeHeap, int prime, char const* varName, int len, char const* value)
{
	struct heapEntry topOfHeap = heapExtractMax(freeHeap);
	int adjustedLen = len;
	if(len % 4 != 0)
		adjustedLen += (4 - (len % 4));
	
	if(topOfHeap.blockSize >= adjustedLen)
	{
		//Update the symbol table
		hashTableInsert(symTable, prime, varName, CHAR, topOfHeap.offset, len);

		//Place the variable in the memspace
		strcpy(((char*) &mem[topOfHeap.offset]), value);
		mem[topOfHeap.offset + len] = STRTERM; //Null terminator

		//Update the free heap
		maxHeapInsert(freeHeap, topOfHeap.blockSize - adjustedLen, topOfHeap.offset + adjustedLen);
	}	
	else
	{
		printf("ERROR: Not enough space to allocate variable.");
		return;
	}
}
// Input: Pointer to memspace, pointer to symbol table, pointer to freespace heap, prime number being used, var name
// Output: Var is removed from symbol table if it was found, else no change. 
void myFree(char* mem, struct symbolTableEntry* symTable, struct heapEntry* freeHeap, int prime, char const* varName)
{
	int indexOfSymbol = 0;
	indexOfSymbol = hashTableSearch(symTable, prime, varName);

	if(indexOfSymbol != -1)
	{
		int indexInMem = symTable[indexOfSymbol].offset;
		int sizeInMem = symTable[indexOfSymbol].noBytes;

		//Return space to free heap
		maxHeapInsert(freeHeap, sizeInMem, indexInMem);	

		//Clear the free space
		for(int i = indexInMem; i < indexInMem + sizeInMem; i++)
			mem[i] = '~';

		hashTableRemove(symTable, prime, varName);
	}
	else
	{
		printf("ERROR: Cannot free variable that has not been allocated");	
	}
}
// Input: Pointer to memspace, pointer to symbol table, var name, var name / scalar
// Output: Value of scalar or second var is added to first
void myAdd(char* mem, struct symbolTableEntry* symTable, int prime, char const* varName, char const* vName)
{
	int varIndex1 = 0;
	varIndex1 = hashTableSearch(symTable, prime, varName);

	if(varIndex1 != -1 && symTable[varIndex1].type == INT) //Var1 exists in symbol table
	{
		if(vName[0] > 47 && vName[0] < 58) //vName is a scalar
		{
			int value = atoi(vName);
			*((unsigned int*) &mem[symTable[varIndex1].offset]) += value; 
		}
		else //vName is a variable name
		{
			int varIndex2 = 0;
			varIndex2 = hashTableSearch(symTable, prime, vName);

			if(varIndex2 != -1 && symTable[varIndex2].type == INT) //Var2 exists in symbol table
			{
				*((unsigned int*) &mem[symTable[varIndex1].offset]) += *((unsigned int*) &mem[symTable[varIndex2].offset]);
			}
			else
				printf("ERROR: RHS does not exist in symbol table, or is not of type INT");	
		}
	}
	else
		printf("ERROR: LHS does not exist in symbol table, or is not of type INT");	

}
// Input: Pointer to free heap
// Output: Adjacent free blocks have been coalesced
void myCompact(struct heapEntry* freeHeap)
{
	int numCompacted = 0;
	int debug = 0;
	int heapSize = getHeapSize();
	if(debug == 1)
		printf("\nHeapSize = %d\n", heapSize);
	struct heapEntry* heapArr = (struct heapEntry*) malloc(heapSize * sizeof(struct heapEntry));	
	
	//Move to new array
	for(int i = 0; i < heapSize; i++)
		heapArr[i] = heapExtractMax(freeHeap);

	//Sort
	struct heapEntry temp;
	for(int i = 0; i < heapSize-1; i++)
		for(int j = i+1; j < heapSize; j++)
			if(heapArr[i].offset > heapArr[j].offset)
			{
				temp = heapArr[j];
				heapArr[j] = heapArr[i];
				heapArr[i] = temp;		
			}	
	
	//Coalesce
	if(debug == 1)
	{
		printf("Offsets: ");
		for(int i = 0; i < heapSize; i++)
			printf("%d ", heapArr[i].offset);
		printf("\n");
		printf("BlockSizes: ");
		for(int i = 0; i < heapSize; i++)
			printf("%d ", heapArr[i].blockSize);
		printf("\n");
	}
	for(int i = 0; i < heapSize; i++)
	{
		if(heapArr[i].offset + heapArr[i].blockSize == heapArr[i+1].offset)
		{
			if(debug == 1)
				printf("Compacting i = %d, i+1 = %d\n", i, i+1);
			heapArr[i].blockSize += heapArr[i+1].blockSize;
			heapArr[i+1].offset = -1;		
			numCompacted++;
			
			if(debug == 1)
			{
				printf("\nAfter Compacting:\n");
				printf("Offsets: ");
				for(int i = 0; i < heapSize; i++)
					printf("%d ", heapArr[i].offset);
				printf("\n");
				printf("BlockSizes: ");
				for(int i = 0; i < heapSize; i++)
					printf("%d ", heapArr[i].blockSize);
				printf("\n");
			}
		}
	}
	if(debug == 1)
		printf("\n");
	
	//Push back into heap
	for(int i = 0; i < heapSize; i++)
		if(heapArr[i].offset != -1)
		{
			if(debug == 1)
				printf("Pushing back into heap: %d\n", i);	
			maxHeapInsert(freeHeap, heapArr[i].blockSize, heapArr[i].offset);	
		}
	if(numCompacted > 0)
		myCompact(freeHeap);	

	free(heapArr);
}
// Input: Relevant data structures, prime, name of base var, literal of var to add
// Output: literal is concatenated to sBase in mem
void myStrCatConst(char* mem, struct symbolTableEntry* symTable, int prime, char const* sBase, char const* sToAdd)
{
	int varIndex = hashTableSearch(symTable, prime, sBase);

	if(varIndex != -1 && symTable[varIndex].type == CHAR)
	{
		int lenBase = strlen(((char*) &mem[symTable[varIndex].offset]));	
		int lenToAdd = strlen(sToAdd);	

		int bytesAllocated = symTable[varIndex].noBytes;
		if(bytesAllocated % 4 != 0) //Word align
			bytesAllocated += (4 - (bytesAllocated % 4));

		if((bytesAllocated - lenToAdd - lenBase) >= 0)
		{
			int indexOfBaseInMem = symTable[varIndex].offset;

			//Concatenate
			strcpy(((char*) &mem[indexOfBaseInMem + lenBase]), sToAdd);
			mem[indexOfBaseInMem + lenBase + lenToAdd] = STRTERM; //Null terminator
		}
		else
			printf("ERROR: LHS insufficient length to perform strcat");
	}
	else
		printf("ERROR: LHS not found in symbol table or not type CHAR");
}
// Input: Relevant data structures, prime, name of base var, name of var to add
// Output: var is concatenated to sBase in mem
void myStrCatVar(char* mem, struct symbolTableEntry* symTable, int prime, char const* sBase, char const* sToAdd)
{
	int baseIndex = hashTableSearch(symTable, prime, sBase);
	int toAddIndex = hashTableSearch(symTable, prime, sToAdd);

	if(baseIndex != -1 && symTable[baseIndex].type == CHAR)
	{
		if(toAddIndex != -1 && symTable[toAddIndex].type == CHAR)
		{
			int lenToAdd = strlen(((char*) &mem[symTable[toAddIndex].offset]));
			int lenBase = strlen(((char*) &mem[symTable[baseIndex].offset]));

			int bytesAllocated = symTable[baseIndex].noBytes;
			if(bytesAllocated % 4 != 0) //Word align
				bytesAllocated += (4 - (bytesAllocated % 4));

			if((bytesAllocated - lenToAdd - lenBase) >= 0)
			{
				int indexOfBaseInMem = symTable[baseIndex].offset;
				int indexOfToAddInMem = symTable[toAddIndex].offset;

				//Concatenate
				strcpy(((char*) &mem[indexOfBaseInMem + lenBase]), ((char*) &mem[indexOfToAddInMem]));
				mem[indexOfBaseInMem + lenBase + lenToAdd] = STRTERM; //Null terminator
			}
			else
				printf("ERROR: LHS insufficient length to perform strcat");
		
		}
		else
			printf("ERROR: RHS not found in symbol table or not type CHAR");
	}
	else
		printf("ERROR: LHS not found in symbol table or not type CHAR");

}
// Input: Relevant data structures, prime, name of base var, literal or name of var to add
// Output: sToAdd is concatenated to sBase in mem
void myStrCat(char* mem, struct symbolTableEntry* symTable, int prime, char const* sBase, char const* sToAdd)
{
	if(sToAdd[0] == '\"')
	{
		int len = strlen(sToAdd);

		char* newToAdd = (char*) malloc(len * sizeof(char));
		strcpy(newToAdd, sToAdd);
		for(int i = 0; i < len - 1; i++)
			newToAdd[i] = newToAdd[i+1];
		newToAdd[len - 2] = STRTERM; //Possibly dubious, more unit tests for this

		myStrCatConst(mem, symTable, prime, sBase, newToAdd);

		free(newToAdd);
	}
	else
		myStrCatVar(mem, symTable, prime, sBase, sToAdd);
}
// Input: Relevant data structures, prime, name of var to print
// Output: prints value of var
void printVar(char* mem, struct symbolTableEntry* symTable, int prime, const char* varName)
{
	int indexOfSymbol = 0;
	indexOfSymbol = hashTableSearch(symTable, prime, varName);

	if(symTable[indexOfSymbol].type == CHAR)
		printf("%s\n", ((char*) &mem[symTable[indexOfSymbol].offset]));
	else if (symTable[indexOfSymbol].type == INT)
		printf("%d\n", mem[symTable[indexOfSymbol].offset]);
	else
		printf("ERROR: Type is not CHAR or INT");
}
