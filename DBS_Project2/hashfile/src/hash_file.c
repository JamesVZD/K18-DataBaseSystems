#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hash_file.h"

#define BLOCK_CAP (int) ((BF_BLOCK_SIZE - 3*sizeof(int)) / sizeof(Record))	//max records per block
#define HASH_CAP (int) (BF_BLOCK_SIZE / (2*sizeof(int)))	//max buckets per block


#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HT_ERROR;        \
  }                         \
}






HT_ErrorCode HT_Init(){

	return HT_OK;
}






HT_ErrorCode HT_CreateIndex(const char *filename, int buckets){

	int fileDesc, hashBlocks, lastBuckets, i;
	char *hashID = "HashFile", *data;
	BF_Block *block;
	
	//Create a block file.
	CALL_BF(BF_CreateFile(filename));
	CALL_BF(BF_OpenFile(filename, &fileDesc));
	
	//Allocate its metadata block.
	BF_Block_Init(&block);
	CALL_BF(BF_AllocateBlock(fileDesc, block));
	
	//Mark it as a hash file and save the number of buckets for potential use.
	data = BF_Block_GetData(block);
	memcpy(data, hashID, 9*sizeof(char));
	memcpy(data + 9*sizeof(char), &buckets, sizeof(int));
	
	//Dirt and unpin the first block.
	BF_Block_SetDirty(block);
	CALL_BF(BF_UnpinBlock(block));
	
	//Calculate the amount of hashtable blocks.
	hashBlocks = buckets / HASH_CAP + 1;
	lastBuckets = buckets % HASH_CAP;
	
	//Fix a corner Case - Last block is full.
	if(lastBuckets == 0)
		hashBlocks--;
	
	//Allocate and initialise all hashtable blocks on 0. Then dirt and unpin them.
	for(i=1 ; i<=hashBlocks ; i++){
	
		CALL_BF(BF_AllocateBlock(fileDesc, block));
		
		data = BF_Block_GetData(block);
		memset(data, 0, BF_BLOCK_SIZE);
		
		BF_Block_SetDirty(block);
		CALL_BF(BF_UnpinBlock(block));
	}
	
	//Free BF_block pointer.
	BF_Block_Destroy(&block);
	
	return HT_OK;
}






HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc){
	
	int buckets, tempDesc, notSet=1, i=0;
	char *data, testHash[9];
	BF_Block *block;
	
	//Open a block file.
	CALL_BF(BF_OpenFile(fileName, &tempDesc));
	
	//Access its first block.
	BF_Block_Init(&block);
	CALL_BF(BF_GetBlock(tempDesc, 0, block));
	
	//Ensure it's a hash file and fetch its number of buckets.
	data = BF_Block_GetData(block);
	memcpy(testHash, data, 9*sizeof(char));
	if(strcmp(testHash, "HashFile") != 0){
		printf("\nThe given file is not a Hash File.\n");
		return HT_ERROR;
	}
	memcpy(&buckets, data + 9*sizeof(char), sizeof(int));
	
	//Unpin its first block.
	CALL_BF(BF_UnpinBlock(block));	

	//Attach it to openFiles array.
	while(notSet && i<MAX_OPEN_FILES){
	
		if(openFiles[i].filedesc == -1){
			printf("\nAttaching hash file with %d buckets id %d to openFiles[%d].\n", buckets, tempDesc, i); 
			openFiles[i].filedesc = tempDesc;	//Its file descriptor.
			openFiles[i].buckets = buckets;		//Its number of buckets.
			openFiles[i].inserted = 0;			//Default number of insertions.
			openFiles[i].lookups = 0;			//Default number of lookups.
			openFiles[i].deleted = 0;			//Default number of deletions.
			*indexDesc = i;						//The position in openFiles array.
			notSet = 0;							//Indicates that a slot in openFiles is found.
		}
		i++;
	}
	
	//Free BF_block pointer.
	BF_Block_Destroy(&block);
	
	//Corner Case - There are already MAX_OPEN_FILES files open.
	if(notSet){
		printf("\nNo more hash files can be opened. The maximum allowed number is already open.\n");
		return HT_ERROR;
	}
	
	return HT_OK;
}






HT_ErrorCode HT_CloseFile(int indexDesc){
	
	//Close the proper block file.
	printf("\nClosing hash file %d with file descriptor %d.\n", indexDesc, openFiles[indexDesc].filedesc);
	CALL_BF(BF_CloseFile(openFiles[indexDesc].filedesc));
	
	//Print file stats.
	printf("Printing file's statistics.\n");
	printf("Total insertions: %d.\n", openFiles[indexDesc].inserted);
	printf("Total deletions: %d.\n", openFiles[indexDesc].deleted);	
	printf("Total lookups: %d.\n", openFiles[indexDesc].lookups);	
	
	//Reset its entry on openFiles array.
	openFiles[indexDesc].filedesc =	-1;
	openFiles[indexDesc].buckets = -1;
	openFiles[indexDesc].inserted = -1;
	openFiles[indexDesc].lookups = -1;
	openFiles[indexDesc].deleted = -1;
	
	return HT_OK;
}






HT_ErrorCode HT_InsertEntry(int indexDesc, Record record){
	
	int b, hashBlockId, hashBlockPos, last, temp, fileBlocks, newBlockId, entries, prev, next;
	char *hashData, *dataData, *dataData2;
	BF_Block *hashBlock, *dataBlock, *dataBlock2;
	
	//Find the appropriate bucket to insert record.
	b = record.id % openFiles[indexDesc].buckets;
	
	//Trace it in the hashtable blocks.
	hashBlockId = b / HASH_CAP + 1;
	hashBlockPos = b % HASH_CAP;
	
	//Get the hashtable block.
	BF_Block_Init(&hashBlock);
	CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, hashBlockId, hashBlock));
	
	//Fetch the bucket's last block.
	hashData = BF_Block_GetData(hashBlock);
	memcpy(&last, hashData + 2*sizeof(int) * hashBlockPos + sizeof(int), sizeof(int));
	//NOTE: hashBlockPos buckets of 2 int must be skipped and we need the second int of the next bucket.
	
	BF_Block_Init(&dataBlock);
	
	//When bucket is empty.
	if(last == 0){
		
		//Allocate bucket's first data block and insert record.
		CALL_BF(BF_AllocateBlock(openFiles[indexDesc].filedesc, dataBlock));
		dataData = BF_Block_GetData(dataBlock);
		
		//Initialise its data.
		temp = 1;
		memcpy(dataData, &temp, sizeof(int));	//Number of entries in block.
		temp = 0;
		memcpy(dataData + sizeof(int), &temp, sizeof(int));	//Previous block id. Zero, indicates first block.
		memcpy(dataData + 2*sizeof(int), &temp, sizeof(int)); //Next block id. Zero, indicates last block.
		
		//Insert entry.	
		memcpy(dataData + 3*sizeof(int), &record, sizeof(Record));
		BF_Block_SetDirty(dataBlock);
		CALL_BF(BF_UnpinBlock(dataBlock));
		
		//Set bucket's first and last block's value to match the allocated block.
		CALL_BF(BF_GetBlockCounter(openFiles[indexDesc].filedesc, &fileBlocks));
		newBlockId = fileBlocks - 1;
		memcpy(hashData + 2*sizeof(int) * hashBlockPos, &newBlockId, sizeof(int));
		memcpy(hashData + 2*sizeof(int) * hashBlockPos + sizeof(int), &newBlockId, sizeof(int));
		BF_Block_SetDirty(hashBlock);
		CALL_BF(BF_UnpinBlock(hashBlock));		
		
	}
	//When bucket is not empty.
	else{
		
		//Fetch bucket's last block, and check whether it's full, empty or neither.
		CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, last, dataBlock));
		dataData = BF_Block_GetData(dataBlock);
		memcpy(&entries, dataData, sizeof(int));
		
		//If the last block is full. 
		if(entries == BLOCK_CAP){
		
			//Get the number of blocks, which is equal to the id of the next allocated block.
			CALL_BF(BF_GetBlockCounter(openFiles[indexDesc].filedesc, &newBlockId));
			
			//Set this value as next block id on data block.
			memcpy(dataData + 2*sizeof(int), &newBlockId, sizeof(int));
			BF_Block_SetDirty(dataBlock);
			CALL_BF(BF_UnpinBlock(dataBlock));
			
			//And also set it as last block in bucket.
			memcpy(hashData + 2*sizeof(int) * hashBlockPos + sizeof(int), &newBlockId, sizeof(int));
			BF_Block_SetDirty(hashBlock);
			CALL_BF(BF_UnpinBlock(hashBlock));
			
			//Allocate new last block, with id newBlockId .
			BF_Block_Init(&dataBlock2);
			CALL_BF(BF_AllocateBlock(openFiles[indexDesc].filedesc, dataBlock2));
			dataData2 = BF_Block_GetData(dataBlock2);
			
			//Initialise its data.
			temp = 1;
			memcpy(dataData2, &temp, sizeof(int));	//Number of entries in block.
			memcpy(dataData2 + sizeof(int), &last, sizeof(int));	//Previous block id.
			temp = 0;
			memcpy(dataData2 + 2*sizeof(int), &temp, sizeof(int)); //Next block id. Zero, for last block.
			
			//Insert entry.
			memcpy(dataData2 + 3*sizeof(int), &record, sizeof(Record));
			BF_Block_SetDirty(dataBlock2);
			CALL_BF(BF_UnpinBlock(dataBlock2));
			BF_Block_Destroy(&dataBlock2);		
		}
		//If the last block is empty, due to deletions.
		else if(entries == 0){
			
			//Get the previous block id.
			memcpy(&prev, dataData + sizeof(int), sizeof(int));
			
			//As long as the current block is empty, and there are previous blocks.
			while(prev != 0 && entries == 0){
				
				//Unpin the current block and get its previous one.
				CALL_BF(BF_UnpinBlock(dataBlock));
				CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, prev, dataBlock));
				
				//Fetch its number of entries.
				dataData = BF_Block_GetData(dataBlock);
				memcpy(&entries, dataData, sizeof(int));
				
				//If that block is empty, fetch its previous block id.
				if(entries == 0){
					
					memcpy(&prev, dataData + sizeof(int), sizeof(int));	
				}
				//If that block is full its next should be filled.
				else if(entries == BLOCK_CAP){
					
					//Fetch next block.
					memcpy(&next, dataData + 2*sizeof(int), sizeof(int));
					CALL_BF(BF_UnpinBlock(dataBlock));
					CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, next, dataBlock));
					dataData = BF_Block_GetData(dataBlock);
					
					//Insert record.
					memcpy(dataData + 3*sizeof(int), &record, sizeof(Record));
					entries = 1;
					memcpy(dataData, &entries, sizeof(int));
					BF_Block_SetDirty(dataBlock);
					CALL_BF(BF_UnpinBlock(dataBlock));
				}
				//If there is space in the current block, insert entry.
				else{
				
					memcpy(dataData + 3*sizeof(int) + entries * sizeof(Record), &record, sizeof(Record));
					entries++;
					memcpy(dataData, &entries, sizeof(int));
					BF_Block_SetDirty(dataBlock);
					CALL_BF(BF_UnpinBlock(dataBlock));
				}
			}
			//Corner Case: If the first block is empty, fill it with the entry.
			if(prev == 0){
			
				memcpy(dataData + 3*sizeof(int), &record, sizeof(Record));
				entries = 1;
				memcpy(dataData, &entries, sizeof(int));
				BF_Block_SetDirty(dataBlock);
				CALL_BF(BF_UnpinBlock(dataBlock));
			}
			
			//Unpin the hashtable block, without any changes.
			CALL_BF(BF_UnpinBlock(hashBlock));
		}
		//If the last block has empty space.
		else{
		
			//Simply insert entry.
			memcpy(dataData + 3*sizeof(int) + entries * sizeof(Record), &record, sizeof(Record));
			entries++;
			memcpy(dataData, &entries, sizeof(int));
			BF_Block_SetDirty(dataBlock);
			CALL_BF(BF_UnpinBlock(dataBlock));
			
			//Unpin the hashtable block, without any changes.
			CALL_BF(BF_UnpinBlock(hashBlock));
		}
	}	
	
	//Free BF_block pointers.
	BF_Block_Destroy(&hashBlock);
	BF_Block_Destroy(&dataBlock);	
	
	//Update openFiles entry.
	openFiles[indexDesc].inserted++;
	
	return HT_OK;
}






HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id){
	
	int found, idBlock, idPos, hashBlockId, inHashBlock, i, dataBlockId, entries, j, total=0, blockNo;
	char *hashData, *dataData;
	BF_Block *hashBlock, *dataBlock;
	Record record;
	
	//If there is a specific lookup.
	if(id != NULL){
	
		//Update openFiles entry and call SeekEntry.
		openFiles[indexDesc].lookups++;
		found = HT_SeekEntry(indexDesc, *id, &idBlock, &idPos, &record);
		
		//If there is an entry with the desired id, print it.
		if(found)
			printf("\nId: %d, Name: %s, Surname: %s, City: %s\n", record.id, record.name, record.surname, record.city);
			
		//Else, print proper message.
		else
			printf("\nThere is no entry with id: %d in file %d.\n\n", *id, indexDesc);
	}
	
	//If all entries must be printed
	else{

		//Initialise BF_Block pointers.
		BF_Block_Init(&hashBlock);
		BF_Block_Init(&dataBlock);
	
		//Fetch the first hashBlock.
		hashBlockId = 1;
		inHashBlock = 0; //Buckets visited in current hashBlock.
		CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, hashBlockId, hashBlock));
		hashData = BF_Block_GetData(hashBlock);
		
		for(i=0 ; i<openFiles[indexDesc].buckets ; i++){
			
			//If there are no more buckets in the current hashBlock, fetch the next one.
			if(inHashBlock == HASH_CAP){
			
				hashBlockId++;
				inHashBlock = 0;
				CALL_BF(BF_UnpinBlock(hashBlock));
				CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, hashBlockId, hashBlock));
				hashData = BF_Block_GetData(hashBlock);
			}
			
			//Fetch the bucket's first block.
			memcpy(&dataBlockId, hashData + 2*sizeof(int) * inHashBlock, sizeof(int));
			
			//If the bucket is empty.
			if(dataBlockId == 0) 
				printf("\nBucket %d is empty.\n", i);
			
			//if the bucket contains entries.
			else{
				
				blockNo = 0;
				printf("\nPrinting contents of bucket %d.\n", i);
				
				//As long as there are blocks in the bucket.
				while(dataBlockId !=0){
					
					//Fetch next block, and get its number of entries.
					CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, dataBlockId, dataBlock));
					dataData = BF_Block_GetData(dataBlock);
					memcpy(&entries, dataData, sizeof(int));
					blockNo++;
					
					//Print all entries in block.
					for(j=0 ; j<entries ; j++){
						
						memcpy(&record, dataData + 3*sizeof(int) + j * sizeof(Record), sizeof(Record));
						printf("Id: %d, Name: %s, Surname: %s, City: %s\n", record.id, record.name, record.surname, record.city);
						total++;
					}
					
					//Fetch the id of the next block.
					memcpy(&dataBlockId, dataData + 2*sizeof(int), sizeof(int));
					
					//Unpin current dataBlock.
					CALL_BF(BF_UnpinBlock(dataBlock));
				}
			}
			
			//Print stats and move to the next bucket.
			printf("\nBucket no %d consists of %d blocks.\n", i, blockNo);
			inHashBlock++;
		}

		//Unpin the last hashBlock.
		CALL_BF(BF_UnpinBlock(hashBlock));
			
		//Free BF_Block pointers.
		BF_Block_Destroy(&hashBlock);
		BF_Block_Destroy(&dataBlock);
		
		printf("\nTotal entries printed: %d.\n\n", total);
	}
		
	return HT_OK;
}






HT_ErrorCode HT_DeleteEntry(int indexDesc, int id){

	int found, idBlock, idPos, b, hashBlockId, hashBlockPos, last, entries;
	char *data;
	BF_Block *block;
	Record record;
	
	//Seek entry with the desired id.
	found = HT_SeekEntry(indexDesc, id, &idBlock, &idPos, NULL);
	
	//If the id was found.
	if(found){
		
		//Initialise a BF_Block pointer.
		//BF_Block_Init(&block);
		
		//Find the appropriate bucket.
		b = id % openFiles[indexDesc].buckets;
	
		//Trace it in the hashtable blocks.
		hashBlockId = b / HASH_CAP + 1;
		hashBlockPos = b % HASH_CAP;
	
		//Get the hashtable block.
		BF_Block_Init(&block);
		CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, hashBlockId, block));
	
		//Fetch the bucket's last block.
		data = BF_Block_GetData(block);
		memcpy(&last, data + 2*sizeof(int) * hashBlockPos + sizeof(int), sizeof(int));
		
		//Unpin hashBlock
		CALL_BF(BF_UnpinBlock(block));
		
		//Get last block and fetch its number of entries.
		CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, last, block));
		data = BF_Block_GetData(block);
		memcpy(&entries, data, sizeof(int));
		
		//As long as the current last block is empty, get its previous one.
		while(entries == 0){ //NOTE: Since SeekEntry returned 1, there is at least one non-empty block in the bucket. Thus, this condition is enough.
		
			memcpy(&last, data + sizeof(int), sizeof(int));
			CALL_BF(BF_UnpinBlock(block));
			CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, last, block));
			
			//Fetch its number of entries.
			data = BF_Block_GetData(block);
			memcpy(&entries, data, sizeof(int));
		}
		
		//If entry with value id is in the last non empty block.
		if(last == idBlock){
			
			//Overwrite it with the last entry and decrease entries. Works even if the deleted entry is the only one in the block.
			memcpy(data + 3*sizeof(int) + idPos * sizeof(Record), data + 3*sizeof(int) + (entries - 1) * sizeof(Record), sizeof(Record));
			entries--;
			memcpy(data, &entries, sizeof(int));
			BF_Block_SetDirty(block);
			CALL_BF(BF_UnpinBlock(block));
		}
		//If entry with value id is not in the last non empty block.
		else{
		
			//Copy the last entry, and decrease entries.
			memcpy(&record, data + 3*sizeof(int) + (entries - 1) * sizeof(Record), sizeof(Record));
			entries--;
			memcpy(data, &entries, sizeof(int));
			BF_Block_SetDirty(block);
			CALL_BF(BF_UnpinBlock(block));
			
			//Get the block containing the entry for deletion.
			CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, idBlock, block));
			data = BF_Block_GetData(block);
			
			//Overwrite it with the last entry.
			memcpy(data + 3*sizeof(int) + idPos * sizeof(Record), &record, sizeof(Record));
			BF_Block_SetDirty(block);
			CALL_BF(BF_UnpinBlock(block));			
		}
	
		//Free BF_Block pointer.
		BF_Block_Destroy(&block);
	
		//Update openFiles entry.
		openFiles[indexDesc].deleted++;
		printf("\nEntry with id: %d was deleted successfully from file %d.\n", id, indexDesc);
	}
	//There is no such id.
	else
		printf("\nThere is no entry with id: %d to delete in file %d.\n", id, indexDesc);
	
	return HT_OK;
}






int HT_SeekEntry(int indexDesc, int id, int *idBlock, int *idPosition, Record *rec){

	int b, hashBlockId, hashBlockPos, current, entries, i;
	char *data;
	BF_Block *block;
	Record record;
	
	//Find the appropriate bucket to seek entry.
	b = id % openFiles[indexDesc].buckets;
	
	//Trace it in the hashtable blocks.
	hashBlockId = b / HASH_CAP + 1;
	hashBlockPos = b % HASH_CAP;
	
	//Get the hashtable block.
	BF_Block_Init(&block);
	CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, hashBlockId, block));
	
	//Fetch the bucket's first block id.
	data = BF_Block_GetData(block);
	memcpy(&current, data + 2*sizeof(int) * hashBlockPos, sizeof(int));
	CALL_BF(BF_UnpinBlock(block));
		
	//Look for the desired entry, as long as there are remaining blocks in the bucket. 
	while(current != 0){
	
		//Fetch current block, and count its entries.
		CALL_BF(BF_GetBlock(openFiles[indexDesc].filedesc, current, block));
		data = BF_Block_GetData(block);
		memcpy(&entries, data, sizeof(int));
		
		//Check all entries in block.
		for(i=0 ; i<entries ; i++){
				
			memcpy(&record, data + 3*sizeof(int) + i * sizeof(Record), sizeof(Record));

			//If the entry is found, return necessary data, unpin current block, and free block pointer.
			if(record.id == id){
				
				//If each pointer is defined by the calling function, fill its content.
				if(idBlock != NULL)
					*idBlock = current;
		
				if(idPosition != NULL)
					*idPosition = i;
				
				if(rec != NULL)
					memcpy(rec, data + 3*sizeof(int) + i * sizeof(Record), sizeof(Record));	
					
				CALL_BF(BF_UnpinBlock(block));
				BF_Block_Destroy(&block);
				return 1;
			}
		}
		
		//Update current with the next block and unpin the last one.
		memcpy(&current, data + 2*sizeof(int), sizeof(int));
		CALL_BF(BF_UnpinBlock(block));
	}
	
	//Free BF_Block pointer.
	BF_Block_Destroy(&block);
	
	return 0;
}
