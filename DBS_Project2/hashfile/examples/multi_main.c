#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hash_file.h"

#define RECORDS_NUM 5000
#define BUCKETS_NUM 15


#define CALL_OR_DIE(call)     \
  {                           \
    HT_ErrorCode code = call; \
    if (code != HT_OK) {      \
      printf("Error\n");      \
      exit(code);             \
    }                         \
  }

const char* names[] = {
  "Yannis",
  "Christofos",
  "Sofia",
  "Marianna",
  "Vagelis",
  "Maria",
  "Iosif",
  "Dionisis",
  "Konstantina",
  "Theofilos",
  "Giorgos",
  "Dimitris"
};

const char* surnames[] = {
  "Ioannidis",
  "Svingos",
  "Karvounari",
  "Rezkalla",
  "Nikolopoulos",
  "Berreta",
  "Koronis",
  "Gaitanis",
  "Oikonomou",
  "Mailis",
  "Michas",
  "Halatsis"
};

const char* cities[] = {
  "Athens",
  "San Francisco",
  "Los Angeles",
  "Amsterdam",
  "London",
  "New York",
  "Tokyo",
  "Hong Kong",
  "Munich",
  "Miami"
};


const char* fileNames[] = {
  "multiData00.db",
  "multiData01.db",
  "multiData02.db",
  "multiData03.db",
  "multiData04.db",
  "multiData05.db",
  "multiData06.db",
  "multiData07.db",
  "multiData08.db",
  "multiData09.db",
  "multiData10.db",
  "multiData11.db",
  "multiData12.db",
  "multiData13.db",
  "multiData14.db",
  "multiData15.db",
  "multiData16.db",
  "multiData17.db",
  "multiData18.db"  
};


int main(void){

	int i, b, indexDesc, r, id, file, found;
	Record record;
	
	//Initialise BF level and global matrix.
	BF_Init(LRU);
	for(i=0 ; i<MAX_OPEN_FILES ; i++){
		openFiles[i].filedesc = -1;
		openFiles[i].buckets = -1;
		openFiles[i].inserted = -1;
		openFiles[i].lookups = -1;
		openFiles[i].deleted = -1;
	}
	
	//Create MAX_OPEN_FILES-1 HashFiles with a random number of buckets b€[5,19].
	srand(18360285);
	for(i=0 ; i<MAX_OPEN_FILES-1 ; i++){
		b = rand() % BUCKETS_NUM + 5;
		CALL_OR_DIE(HT_CreateIndex(fileNames[i], b));
		CALL_OR_DIE(HT_OpenIndex(fileNames[i], &indexDesc));
	}
	
	//Create a second filedesc to the last open file.
	CALL_OR_DIE(HT_OpenIndex(fileNames[i-1], &indexDesc));
	
	//Insert entries.
	for(id=0 ; id<RECORDS_NUM ; id++){
	
		record.id = id;
		r = rand() % 12;
		memcpy(record.name, names[r], strlen(names[r]) + 1);
		r = rand() % 12;
		memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
		r = rand() % 10;
		memcpy(record.city, cities[r], strlen(cities[r]) + 1);
		
		//Insert each entry in one of the first 19 files.
		file = rand() % (MAX_OPEN_FILES - 1);
		CALL_OR_DIE(HT_InsertEntry(file, record));
	}
	
	//Print and delete all entries.
	for(id=0 ; id<RECORDS_NUM ; id++){
		
		//Begin in the first file.
		i = 0;
		
		do{
			//Seek entry in current file.
			found = HT_SeekEntry(i, id, NULL, NULL, NULL);
			
			//If you find it, print and then delete it.
			if(found){
				CALL_OR_DIE(HT_PrintAllEntries(i, &id));
				CALL_OR_DIE(HT_DeleteEntry(i, id));
			}
			//Move onto the next file.
			i++;	
		
		//Untin you find it, or reach the last "twin" files.
		}while(found == 0 && i<MAX_OPEN_FILES-2);
		
		//In case none of the above files contained the desired entry, it lies within the last "twin" files.
		if(found == 0){
			CALL_OR_DIE(HT_PrintAllEntries(MAX_OPEN_FILES-1, &id));
			CALL_OR_DIE(HT_DeleteEntry(MAX_OPEN_FILES-1, id));
		}
	}
			
	//Closes files and prints stats.
	for(i=0 ; i<MAX_OPEN_FILES ; i++){
		CALL_OR_DIE(HT_CloseFile(i));
	}
	
	printf("\nNormally, all files except for the last two, should have the same amount of insertions, lookups, and deletions.\n");
	printf("Out of the last two, the first should have only insertions, and the other one, should have an equivalent amount of lookups and deletions.\n");
	
	BF_Close();
	
	return 0;
}
