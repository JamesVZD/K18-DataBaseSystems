#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hash_file.h"

#define RECORDS_NUM 1700
#define BUCKETS_NUM 200
#define FILE_NAME "templateData.db"


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

int main(void){

	int i, indexDesc, r, id;
	
	//Initialise BF level and global matrix.
	BF_Init(LRU);
	for(i=0 ; i<MAX_OPEN_FILES ; i++){
		openFiles[i].filedesc = -1;
		openFiles[i].buckets = -1;
		openFiles[i].inserted = -1;
		openFiles[i].lookups = -1;
		openFiles[i].deleted = -1;
	}

	//Create a HashFile.
	CALL_OR_DIE(HT_Init());
	CALL_OR_DIE(HT_CreateIndex(FILE_NAME, BUCKETS_NUM));
	CALL_OR_DIE(HT_OpenIndex(FILE_NAME, &indexDesc));

	//Insert Entries.
	Record record;
	srand(12569874);
	printf("Insert Entries\n");
	for (id = 0 ; id<RECORDS_NUM ; id++){
	
		record.id = id;
		r = rand() % 12;
		memcpy(record.name, names[r], strlen(names[r]) + 1);
		r = rand() % 12;
		memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
		r = rand() % 10;
		memcpy(record.city, cities[r], strlen(cities[r]) + 1);
		CALL_OR_DIE(HT_InsertEntry(indexDesc, record));
	}

	//Print them all.
	printf("RUN PrintAllEntries\n");
	id = rand() % RECORDS_NUM;
	CALL_OR_DIE(HT_PrintAllEntries(indexDesc, NULL));
	
	//Print a specific entry, then delete it, and make sure everything worked.
	printf("Print Entry with id = %d\n", id); 
	CALL_OR_DIE(HT_PrintAllEntries(indexDesc, &id));
	printf("Delete Entry with id = %d\n" ,id);
	CALL_OR_DIE(HT_DeleteEntry(indexDesc, id));
	printf("Print Entry with id = %d\n", id); 
	CALL_OR_DIE(HT_PrintAllEntries(indexDesc, &id));
	
	//Close file and print stats.
	CALL_OR_DIE(HT_CloseFile(indexDesc));
	BF_Close();
	
	return 0;
}
