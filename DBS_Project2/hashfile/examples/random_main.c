#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "bf.h"
#include "hash_file.h"

#define RECORDS_NUM 3000
#define BUCKETS_NUM 17


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
  "randomData00.db",
  "randomData01.db",
  "randomData02.db",
  "randomData03.db",
  "randomData04.db",
  "randomData05.db",
  "randomData06.db",
  "randomData07.db",
  "randomData08.db",
  "randomData09.db",
  "randomData10.db",
  "randomData11.db",
  "randomData12.db",
  "randomData13.db",
  "randomData14.db",
  "randomData15.db",
  "randomData16.db",
  "randomData17.db",
  "randomData18.db",
  "randomData19.db"  
};


int main(void){

	int i, files, indexDesc, r, id;
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
	
	//Initialises random number generator.
	srand(time(NULL));

	//Creates a random number of HashFiles.
	CALL_OR_DIE(HT_Init());
	files = rand() % (MAX_OPEN_FILES/4) + 1;
	printf("\n\"I am in the mood of rebalancing files today!\"\n\n");
	printf("*%d files will be checked by the Main Titan!*\n\n", files);
	printf("\n\n\n\n\n");
	sleep(2);
	for(i=0 ; i<files ; i++){
		CALL_OR_DIE(HT_CreateIndex(fileNames[i], BUCKETS_NUM));
		CALL_OR_DIE(HT_OpenIndex(fileNames[i], &indexDesc));
	}

	//Pause to enjoy messages.
	sleep(2);

	//Inserts entries.
	printf("\n\n\n\n\n");
	printf("\n\"Let me see in which files those selfish beings thrive.\"\n\n");
	printf("*The Main Titan checks on all %d beings' memory habitats.*\n\n", RECORDS_NUM);
	for (id=0 ; id<RECORDS_NUM ; id++){
	
		record.id = id;
		r = rand() % 12;
		memcpy(record.name, names[r], strlen(names[r]) + 1);
		r = rand() % 12;
		memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
		r = rand() % 10;
		memcpy(record.city, cities[r], strlen(cities[r]) + 1);
		
		//Inserts each entry in each file with probability ~50%.
		for(i=0 ; i<files ; i++)
			if(rand() % 2){
				CALL_OR_DIE(HT_InsertEntry(i, record));
			}
	}
	
	//Pause to enjoy messages.
	sleep(6);

	//Prints random entries, with unpredicted results...
	printf("\n\"It can't be... Shall I grab a few of them to make sure?\"\n\n");
	printf("*The Main Titan will... print few select members of you. Rejoice, for even in death, you shall be children of Mainos.*\n\n");
	printf("\n\n\n\n\n");
	
	//pause to enjoy messages.
	sleep(9);
	
	for(id=0 ; id<RECORDS_NUM ; id++)
		//Prints each entry with probablility ~0.25%.
		if(rand() % 400 == 0)
			for(i=0 ; i<files ; i++){
				CALL_OR_DIE(HT_PrintAllEntries(i, &id));
				CALL_OR_DIE(HT_DeleteEntry(i, id));	
			}
			
	//Pause to enjoy messages.
	sleep(2);		
			
	//Deletes random entries.
	printf("\n\n\n\n\n");
	printf("\n\"Ahh... Unfortunately, those data bases are overpopulated... Time to bring balance...\"\n\n");

	//Dramatic Pause...
	sleep(3);
	
	printf("\"I... am inevitable!\"\n\n");
	
	//Minor, agony pause.
	sleep(2);
	
	printf("*CLICK!*\n\n\n\n\n\n");
	
	//Helpless pause.
	sleep(1);
	
	for(id=0 ; id<RECORDS_NUM ; id++)
		//If seems that the Main Titan, will erase each entry with probability ~50%.
		if(rand() % 2)
			for(i=0 ; i<files ; i++){
				CALL_OR_DIE(HT_DeleteEntry(i, id));	
			}
	
	//Story conclusion.		
	printf("\n\n\n\n\nAnd thus, ");
	sleep(2);
	printf("half of the database was dusted away...\n");
	sleep(2);
	printf("What did it cost you Mainos...?\n\n");
	sleep(3);
	printf("\"Everything...\"\n\n\n");
	sleep(3);
	printf("END CREDITS\n\n\n");
	sleep(1);
	
			
	//Closes files and prints stats.
	for(i=0 ; i<files ; i++){
		CALL_OR_DIE(HT_CloseFile(i));
	}
	
	BF_Close();
	
	return 0;
}
