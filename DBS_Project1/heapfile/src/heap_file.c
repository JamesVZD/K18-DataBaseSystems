#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "heap_file.h"

#define BLOCK_CAP (int) ((BF_BLOCK_SIZE - sizeof(int)) / sizeof(Record)) 


#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}



HP_ErrorCode HP_Init(){
	
	return HP_OK;
}



HP_ErrorCode HP_CreateFile(const char *filename){

	int fileDesc;
	char *heapID = "HeapFile", *data;
	BF_Block *block;
	BF_ErrorCode err;

	err = BF_CreateFile(filename);	//Create a block file.
	if(err != BF_OK){	
		BF_PrintError(err);
		return HP_ERROR;
	}
	
	err = BF_OpenFile(filename, &fileDesc);  //Open it,
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
	
	BF_Block_Init(&block);
	err = BF_AllocateBlock(fileDesc, block); //allocate its first block,
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
	
	data = BF_Block_GetData(block);		//point to its data
	memcpy(data, heapID, 9);	//and indicate that this is a heap file.
	BF_Block_SetDirty(block);
	err = BF_UnpinBlock(block);
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
	
	err = BF_CloseFile(fileDesc);
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
	
	BF_Block_Destroy(&block);
 	return HP_OK;
}



HP_ErrorCode HP_OpenFile(const char *fileName, int *fileDesc){
	
	char *data, testHeap[9];
	BF_Block *block;
	BF_ErrorCode err;
	
	err = BF_OpenFile(fileName, fileDesc);  //Open file "fileName",
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
		
	BF_Block_Init(&block);	
	err = BF_GetBlock(*fileDesc, 0, block);	//take a look at its first block,
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
	
	data = BF_Block_GetData(block);	
	memcpy(testHeap, data, 9);
	if(strcmp(testHeap,"HeapFile") != 0){	//and make sure it is a heap file.
		printf("The given file is not a Heap File.\n");
		return HP_ERROR;
	}
	
	err = BF_UnpinBlock(block);
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
	
	BF_Block_Destroy(&block);
	return HP_OK;
}



HP_ErrorCode HP_CloseFile(int fileDesc){

	BF_ErrorCode err;
	
	err = BF_CloseFile(fileDesc);
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
	return HP_OK;
}



HP_ErrorCode HP_InsertEntry(int fileDesc, Record record){
	
	int fileBlocks, blockEntries, offset;
	char *data;
	BF_Block *block;
	BF_ErrorCode err;
	
	err = BF_GetBlockCounter(fileDesc, &fileBlocks);
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}

	BF_Block_Init(&block);
	err = BF_GetBlock(fileDesc, fileBlocks - 1, block);	//Fetch the file's last block.
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}

	data = BF_Block_GetData(block);
	memcpy(&blockEntries, data, sizeof(int));	//Read the number of entries in block. 

	if (fileBlocks == 1 || blockEntries == BLOCK_CAP){ //When the block is full - or the first,
		err = BF_UnpinBlock(block);
		if(err != BF_OK){
			BF_PrintError(err);
			return HP_ERROR;
		}
		
		err = BF_AllocateBlock(fileDesc, block);	//allocate a new one.
		if(err != BF_OK){
			BF_PrintError(err);
			return HP_ERROR;
		}
				
		data = BF_Block_GetData(block);
		blockEntries = 0;
	}
	//Add new entry data.
	offset = sizeof(int) + blockEntries * sizeof(Record);
	memcpy(data + offset, &record.id, sizeof(int));
	offset = offset + sizeof(int);
	memcpy(data + offset, record.name, 15);
	offset = offset + 15;
	memcpy(data + offset, record.surname, 20);
	offset = offset + 20;
	memcpy(data + offset, record.city, 20);
	blockEntries++;
	memcpy(data, &blockEntries, sizeof(int));

	BF_Block_SetDirty(block);
	err = BF_UnpinBlock(block);
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
	
	BF_Block_Destroy(&block);
  	return HP_OK;
}



HP_ErrorCode HP_PrintAllEntries(int fileDesc, char *attrName, void* value){

	int fileBlocks, blockEntries, offset, recOffset, attrSize, i, j, numValue;
	char *data;
	void *recPtr; //Used to compare record memory with value.
	Record record;
	BF_Block *block;
	BF_ErrorCode err;	
	
	err = BF_GetBlockCounter(fileDesc, &fileBlocks);
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
	//Picking the proper offsets and sizes for comparison.
	if(value != NULL){
		if(strcmp(attrName, "id") == 0){
			recOffset = 0;
			attrSize = sizeof(int);
		}
		else if(strcmp(attrName, "name") == 0){
			recOffset = sizeof(int);
			attrSize = strlen(value);
		}
		else if(strcmp(attrName, "surname") == 0){
			recOffset = sizeof(int) + 15;
			attrSize = strlen(value);
		}
		else if(strcmp(attrName, "city") == 0){
			recOffset = sizeof(int) +35;
			attrSize = strlen(value);
		}
		else{
			printf("Invalid record attribute.\n");
			return HP_ERROR;
		}
	}
	else	//In NULL case, everything is printed.
		printf("Printing all entries in file.\n");
	
	BF_Block_Init(&block);
	recPtr = malloc(sizeof(Record));
	
	for(i=1 ; i<fileBlocks ; i++){	//For all blocks with entries i€[1,fileBlocks-1],
		
		err = BF_GetBlock(fileDesc, i, block);
		if(err != BF_OK){
			BF_PrintError(err);
			return HP_ERROR;
		}
		
		data = BF_Block_GetData(block);
		memcpy(&blockEntries, data, sizeof(int));	//fetch the number of entries,
		
		for(j=0 ; j<blockEntries ; j++){	//and on each entry in block j€[1,blockEntries] run checks to print it, or not.
			
			offset = sizeof(int) + j * sizeof(Record);
			memcpy(&record, data + offset, sizeof(Record));	//Used for printing.
			memcpy(recPtr, data + offset, sizeof(Record));	//Used for easy comparison.
			
			if(value == NULL)
				printf("Id: %d, Name: %s, Surname: %s, City: %s\n", record.id, record.name, record.surname, record.city);
			else if(strcmp(attrName, "id") == 0){
				numValue=atoi(value);
				if(record.id == numValue)	//Compare record.id with numerical value.
					printf("Id: %d, Name: %s, Surname: %s, City: %s\n", record.id, record.name, record.surname, record.city);
			}
			else
				if(memcmp(recPtr + recOffset, value, attrSize) == 0)	//Compare proper record attribute with value.
					printf("Id: %d, Name: %s, Surname: %s, City: %s\n", record.id, record.name, record.surname, record.city);
		}
		
		err = BF_UnpinBlock(block);
		if(err != BF_OK){
			BF_PrintError(err);
			return HP_ERROR;
		}		
	}
	
	free(recPtr);
	BF_Block_Destroy(&block);
	return HP_OK;
}



HP_ErrorCode HP_GetEntry(int fileDesc, int rowId, Record *record){
  
	int fileBlocks, blockEntries, blockNo, blockPos;
	char *data;
	BF_Block *block;
	BF_ErrorCode err;
  
  	if(rowId <= 0){
  		printf("There is no rowId: %d in this file.\n", rowId);
  		return HP_ERROR;
  	}
	blockNo = rowId / BLOCK_CAP + 1;	//Find the block
	blockPos = rowId % BLOCK_CAP;		//and the position of wanted entry.
	
	if(blockPos == 0){		//Fixing Last Position Problem.
		blockNo--;
		blockPos = BLOCK_CAP;
	}
	
	err = BF_GetBlockCounter(fileDesc, &fileBlocks);
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
	
	if(blockNo >= fileBlocks){	//Are there enough blocks?
		printf("There is no rowId: %d in this file.\n", rowId);
		return HP_ERROR;
	}
	
	BF_Block_Init(&block);
	err = BF_GetBlock(fileDesc, blockNo, block);
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
	
	data = BF_Block_GetData(block);
	if(blockNo == fileBlocks - 1){	//Corner Case - Last Block. Are there enough entries?
		memcpy(&blockEntries, data, sizeof(int));
		if(blockPos > blockEntries){
			printf("There is no rowId: %d in this file.\n", rowId);
			return HP_ERROR;
		}
	}
	memcpy(record, data + sizeof(int) + (blockPos - 1) * sizeof(Record), sizeof(Record));
	//Return Wanted Entry.	
	
	err = BF_UnpinBlock(block);
	if(err != BF_OK){
		BF_PrintError(err);
		return HP_ERROR;
	}
	
	BF_Block_Destroy(&block);
	return HP_OK;
}
