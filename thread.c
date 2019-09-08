/*
# Family Name: Cakmak

# Given Name: Ege

# Section: E / EECS 3221

# Student Number: 215173131

# CSE Login: cakmake
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

typedef int bool;
#define true 1
#define false 0

// struct to store a word and its frequency.
typedef struct {
    char word[101];
    int freq;
} WordArray;

// struct to store file info.
typedef struct {
    char fileName[256];
    char medianWord[101];
    size_t wordCount;
} fileInfo;

// struct to store parameters for the thread
typedef struct{
     FILE *currentFile;
     fileInfo *currentFileInfo;
} threadParams;

void *fileThread(void *params);

void printUsageMessage();

void printUnableToOpenFileMessage(char *fileName);

void printResults(fileInfo **fileInformations, int numberOfFiles);

int calculateNumberOfWords(FILE *sourceFilePointer);

char *getNextWord(FILE *sourceFilePointer);

int indexOfWordInArray(char *word, WordArray *arrayOfWords[], size_t numberOfDistinctWords);

int wordComparator(const void *a, const void *b);

int fileComparator(const void *a, const void *b);

void processFile(FILE *sourceFilePointer, char *medianWord, size_t *wordCount);

char *findMedianWord(WordArray *arrayOfWords[], int numberOfWordsInTheArray);

int main(int argc, char *argv[]) {
    if (argc == 1) { // If no input is given print usage message.
        printUsageMessage();
        return 1; // Exit with error code 1.
    }
    int numberOfFiles = argc - 1;
    int numberOfValidFiles = 0;
    char *validFiles[numberOfFiles];
    FILE *validFilePointers[numberOfFiles];
    int i;
    /* Checks all inputted files and stores the names and pointers for them 
    * in the corresponding arrays.*/
    for (i = 1; i <= numberOfFiles; i++) {
         FILE *sourceFilePointer = fopen(argv[i], "rb");
         if (sourceFilePointer != NULL) {
             validFilePointers[numberOfValidFiles] = sourceFilePointer;
             validFiles[numberOfValidFiles] = argv[i];
             numberOfValidFiles++;
         } else {
             printUnableToOpenFileMessage(argv[i]);
         }
    }

    // A dynamic array of pointers to fileInfo structs.
    fileInfo **fileInformations = malloc(numberOfValidFiles * sizeof(fileInfo *));
    pthread_t threadIDs[numberOfValidFiles]; // An array to store the thread IDs.
    
    // Process each valid file concurrently using threads and initializes their fileInfo structs.
    for (i = 0; i < numberOfValidFiles; i++) {
        FILE *currentFile = validFilePointers[i];
        *(fileInformations + i) = malloc(sizeof(fileInfo)); // Allocates memory for struct at ith position.
        fileInfo *currentFileInfo = *(fileInformations + i);
        strncat(currentFileInfo->fileName, validFiles[i], 255); // Copies name of the file to its struct.
        
        pthread_t *currentThreadID = &threadIDs[i];
        pthread_attr_t attr; // Attribute for thread.
        pthread_attr_init(&attr); // Initializes attribute.
        /* We need to pass multiple parameters to the thread we will create.
         * To do so, we need to create a struct including all the parameters and
         * pass it to the thread instead of parameters. */
        threadParams *parameters = malloc(sizeof(threadParams));
        (*parameters).currentFile = currentFile;
        (*parameters).currentFileInfo = currentFileInfo;
        // Creates the thread.
        pthread_create(currentThreadID,&attr,fileThread,(void *) parameters);
    }
    
    // Waits for all the threads to be done and closes their file pointers.
    for(i = 0; i < numberOfValidFiles; i++){
        FILE *currentFile = validFilePointers[i];
        pthread_join(threadIDs[i], NULL); // Waits for thread ID i.
        fclose(currentFile);
    }
    
    // Sort output information about files by their number of distinct words and print them.
    qsort(fileInformations, (size_t) numberOfValidFiles, sizeof(fileInfo *), fileComparator);
    printResults(fileInformations, numberOfValidFiles);
    
    // Clean garbage.
    for (i = 0; i < numberOfValidFiles; i++) {
        fileInfo *current = *(fileInformations + i);
        free(current);
    }

    // Clean some more garbage.
    free(fileInformations);

    return 0; // Exit with no problems
}

// This is the function that will run in each thread.
void *fileThread(void *params) {
    threadParams *parameters = (threadParams *) params;
//    fprintf(stderr,"startingthread\n");
    char *medianWord = malloc(101);
    size_t wordCount;
    processFile(parameters->currentFile, medianWord, &wordCount);
    strncat(parameters->currentFileInfo->medianWord, medianWord, 100);
    parameters->currentFileInfo->wordCount = wordCount;
    free(medianWord);
    free(params);
//    fclose(parameters->currentFile);
//    sleep(1);
//    fprintf(stderr,"endingthread\n");
    pthread_exit(0);
}


// Prints instructions on how to use the program.
void printUsageMessage() {
    fprintf(stderr, "You did not input any files. \n");
    fprintf(stderr, "You can input as many files as you want. \n");
    fprintf(stderr, "Usage: \n");
    fprintf(stderr, "thr arg1 arg2 ... argn \n");
}

// Prints an error message in case a file can not be opened.
void printUnableToOpenFileMessage(char *fileName) {
    fprintf(stderr, "Could not open file %s. \n", fileName);
}

// Prints results for inputted files.
void printResults(fileInfo **fileInformations, int numberOfFiles) {
    int i;
    for (i = 0; i < numberOfFiles; i++) {
        fileInfo *currentFileInfo = *(fileInformations + i);
        printf("%s %d %s\n", currentFileInfo->fileName, (int) (currentFileInfo->wordCount),
               currentFileInfo->medianWord);
    }
}

// Calculates the total number of words in a file.
int calculateNumberOfWords(FILE *sourceFilePointer) {
    bool isScanningWord = false;
    int wordCounter = 0;
    int ch;
    while ((ch = getc(sourceFilePointer)) != EOF) { // Read file until reaching the end of it.
        if (ch == ' ' || ch == '\n' || ch == '\t') { // If looking at non letter ...
            if (isScanningWord) { // Stop counting the number of letters in that word and continue.
                isScanningWord = false;
                wordCounter++;
            }
        } else {
            if (!isScanningWord) {// Start counting the number of letters in that word and continue.
                isScanningWord = true;
            }
        }
    }
    return wordCounter;
}

// Gets the next word separated by spaces.
char *getNextWord(FILE *sourceFilePointer) {
    char *word = malloc(101);
    char *wordptr = word;
    bool isScanningWord = false;
    int ch;
    while ((ch = getc(sourceFilePointer)) !=
           EOF) { // Read file until reaching the end of it. Unless loop is broken inside the loop.
        if (ch == ' ' || ch == '\n' || ch == '\t') { // If looking at non letter ...
            if (isScanningWord) { // Stop getting the word and break.
                break;
            }
        } else {
            if (!isScanningWord) { // Start getting the word and continue.
                isScanningWord = true;
            }
            *wordptr = (char) ch;
            wordptr++;
        }
    }
    *wordptr = '\0'; //Add null to the end of the word.
    return word;
}

// Gets the index of an existing word in the array of distinct words. If word doesn't exist in it, returns -1.
int indexOfWordInArray(char *word, WordArray *arrayOfWords[], size_t numberOfDistinctWords) {
    int j;
    for (j = 0; j < numberOfDistinctWords && (arrayOfWords[j])->word != NULL; j++) {
        int compareResult = strcmp((arrayOfWords[j])->word, word);
        if (compareResult == 0) {
            return j;
        }
    }
    return -1;
}

// Compares words by their frequencies and ascii values.
int wordComparator(const void *a, const void *b) {
    const WordArray *first = *(const WordArray **) a;
    const WordArray *second = *(const WordArray **) b;
    int difference = second->freq - first->freq;
    if (difference > 0) { // If first is greater than the second ...
        return 1;
    } else if (difference < 0) { // If first is less than the second ...
        return -1;
    } else { // If frequencies are equal, compare the words by their ASCII values.
        return strcmp(first->word, second->word);
    }
}

// Compares two files by their word numbers.
int fileComparator(const void *a, const void *b) {
    const fileInfo *first = *(const fileInfo **) a;
    const fileInfo *second = *(const fileInfo **) b;
    int difference = ((int) second->wordCount) - ((int) first->wordCount);
    if (difference > 0) { // If first is greater than the second ...
        return 1;
    } else { // If first is less than or equal to the second ...
        return -1;
    }
}

// Finds the mean word.
char *findMedianWord(WordArray *arrayOfWords[], int numberOfWordsInTheArray) {
    int median;
    if (numberOfWordsInTheArray % 2 == 0) { // If the number of distinct words are even get the nth word.
        median = numberOfWordsInTheArray / 2;
    } else { // If the number of distinct words are odd get the word at position n + 1.
        median = (numberOfWordsInTheArray / 2) + 1;
    }
    median--; // Set the median number to the corresponding array index.
    return (arrayOfWords[median])->word;
}

// Processes the file
void processFile(FILE *sourceFilePointer, char *medianWord, size_t *wordCount) {
    int numberOfWords = calculateNumberOfWords(sourceFilePointer); // Total number of words in the file.
    rewind(sourceFilePointer); // Rewinds the sourceFilePointer to be able to read from it again.
    WordArray *arrayOfDistinctWords[numberOfWords]; // An array of pointers pointing to word structures to store distinct words.
    size_t numberOfDistinctWordsInArray = 0; // Number of distinct words in the file.
    int i = 0;

    for (i = 0; i < numberOfWords; i++) { // Get each word in the file and process.
        char *word = getNextWord(sourceFilePointer);
        int index = indexOfWordInArray(word, arrayOfDistinctWords, numberOfDistinctWordsInArray);
        if (index > -1) { // If word exists in arrayOfDistinctWords increase its frequency.
            (arrayOfDistinctWords[index])->freq++;
        } else { // If it doesn't exist insert it into arrayOfDistinctWords and initialize freq as 1.
            arrayOfDistinctWords[numberOfDistinctWordsInArray] = malloc(sizeof(WordArray));
            strcpy((arrayOfDistinctWords[numberOfDistinctWordsInArray])->word, word);
            arrayOfDistinctWords[numberOfDistinctWordsInArray]->freq = 1;
            numberOfDistinctWordsInArray++;
        }
        free(word);
    }

    qsort(arrayOfDistinctWords, numberOfDistinctWordsInArray, sizeof(WordArray *), wordComparator);

    //Pass the results to calling function.
    *wordCount = numberOfDistinctWordsInArray;
    strcpy(medianWord, findMedianWord(arrayOfDistinctWords, (int) numberOfDistinctWordsInArray));

    for (i = 0; i <
                numberOfDistinctWordsInArray; i++) { // Free the memories at each of the pointers are pointing to in the distinct word array.
        free(arrayOfDistinctWords[i]);
    }
}
