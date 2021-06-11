#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hash_file.h"

#define RECORDS_NUM 30
#define BUCKETS_NUM 3
#define FILE_NAME "simpleData.db"


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

	int i, indexDesc, r, id, scanner;
	char name[15]="Dimitris", surname[20]="Verlekis", city[20]="Athens", mode[100];
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
	
	//Create a Hash File.
	CALL_OR_DIE(HT_Init());
	CALL_OR_DIE(HT_CreateIndex(FILE_NAME, BUCKETS_NUM));
	CALL_OR_DIE(HT_OpenIndex(FILE_NAME, &indexDesc));
	
	//Insert Entries.
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
	
	//Print Initial Entries.
	printf("Printing all initial entries.\n");
	CALL_OR_DIE(HT_PrintAllEntries(indexDesc, NULL));
	
	//Insert, Print, delete, and try to print again an extra entry.
	id = RECORDS_NUM;
	printf("Testing entry with id: %d.\n", id);
	record.id = id;
	memcpy(record.name, name, strlen(name) + 1);
	memcpy(record.surname, surname, strlen(surname) + 1);
	memcpy(record.city, city, strlen(city) + 1);
	printf("Insert entry with id: %d\n", id);
	CALL_OR_DIE(HT_InsertEntry(indexDesc, record));
	
	printf("Print entry with id: %d\n", id);
	CALL_OR_DIE(HT_PrintAllEntries(indexDesc, &id));
	
	printf("Delete Entry with id: %d\n" ,id);
	CALL_OR_DIE(HT_DeleteEntry(indexDesc, id));
	
	printf("Print Entry with id: %d\n", id); 
	CALL_OR_DIE(HT_PrintAllEntries(indexDesc, &id));
	
	//Start prompt.
	printf("Now it's your time to play!\n\n");
	
	do{
		//Explain Prompt.
		printf("\n\nTo insert an entry, press 'i'.\nTo print an entry, press 'p'.\nTo print all entries, press 'a'.\nTo delete an entry, press 'd'.\nIf you wish to quit, press q.\n");
		printf("Press your option, and hit enter.\n\n");
		
		//Fetch an input.
		scanner = scanf("%s", mode);
		
		//Insert
		if(strcmp(mode, "i") == 0){
		
			printf("Provide entry data, separated with a space. Then hit enter. Example:\n\"1500834 Anonymous Student Athens\"\n");
			scanner = scanf("%d %s %s %s", &id, name, surname, city);
			
			//If the input was complete.
			if(scanner == 4){
				
				//Check if id already exists.
				if(HT_SeekEntry(indexDesc, id, NULL, NULL, &record))
					printf("There is already an entry with this id. Name: %s, Surname: %s, City: %s\n", record.name, record.surname, record.city);
				
				else{
				
					record.id = id;
					memcpy(record.name, name, strlen(name) + 1);
					memcpy(record.surname, surname, strlen(surname) + 1);
					memcpy(record.city, city, strlen(city) + 1);
					printf("\nInserting entry with id: %d.\n\n", record.id);
					CALL_OR_DIE(HT_InsertEntry(indexDesc, record));
				}
			}
			else
				printf("Wrong input. Try again.\n");
		}

		//Print
		else if(strcmp(mode, "p") == 0){
			
			printf("Provide the desired id to print.\n");
			scanner = scanf("%d",&id);
			
			//If the input was complete.
			if(scanner == 1){
				printf("\nPrinting entry with id: %d.\n\n", id);
				CALL_OR_DIE(HT_PrintAllEntries(indexDesc, &id));
			}
			else
				printf("Wrong input. Try again.\n");			
		}
		
		//Print All
		else if(strcmp(mode, "a") == 0){
			
			printf("\nPrinting all entries in file.\n");
			CALL_OR_DIE(HT_PrintAllEntries(indexDesc, NULL));
		}
		
		//Delete
		else if(strcmp(mode, "d") == 0){
			
			printf("Provide the desired id to delete.\n");
			scanner = scanf("%d",&id);
			
			if(scanner == 1){
				printf("\nDeleting entry with id: %d.\n\n", id);
				CALL_OR_DIE(HT_DeleteEntry(indexDesc, id));
			}
			else
				printf("Wrong input. Try again.\n");			
		}
		
		//Quit Prompt
		else if(strcmp(mode, "q") == 0)	
			printf("\nFarewell dear user. It was nice to meet you. I shall terminate myself now...\n");
		
		//Incorrect Input
		else
			printf("Invalid input. Try again.\n\n");
	
	//Repeat, until user quits.
	}while(strcmp(mode, "q") != 0);

	//Close file and print stats.
	CALL_OR_DIE(HT_CloseFile(indexDesc));
	BF_Close();
	
	return 0;
}
