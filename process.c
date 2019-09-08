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
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

typedef int bool;
#define true 1
#define false 0
#define READ_END 0
#define WRITE_END 1
#define MAXIMUM_WORDS 10000
#define MAXIMUM_WORD_LENGTH 100
#define MAXIMUM_NUMBER_OF_DIGITS_OF_WORD_NUMBER 5
#define MAXIMUM_FILE_LENGTH 255
#define SPACE_CHARACTER_SIZE 1
#define NULL_CHARACTER_SIZE 1
#define BUFFER_SIZE (MAXIMUM_WORD_LENGTH + SPACE_CHARACTER_SIZE + MAXIMUM_NUMBER_OF_DIGITS_OF_WORD_NUMBER + NULL_CHARACTER_SIZE)



//struct to store a word and its frequency.

typedef struct {
    char word[101];
    int freq;
} WordArray;

//struct to store file info.

typedef struct {
    char fileName[256];
    char medianWord[101];
    size_t wordCount;

} fileInfo;

void printUsageMessage();

void printUnableToOpenFileMessage(char *fileName);

void printResults(fileInfo *fileInformations[], int numberOfFiles);

int calculateNumberOfWords(FILE *sourceFilePointer);

char *getNextWord(FILE *sourceFilePointer);

int indexOfWordInArray(char *word, WordArray *arrayOfWords[], size_t numberOfDistinctWords);

int wordComparator(const void *a, const void *b);

int fileComparator(const void *a, const void *b);

char *findMedianWord(WordArray *arrayOfWords[], int numberOfWordsInTheArray);

void copyMessageContentsToStruct(char *message, fileInfo *fileInformation, char *fileName);

void processFile(FILE *sourceFilePointer, char *medianWord, size_t *wordCount);

int main(int argc, char *argv[]) {
    if (argc == 1) { // If no input is given print usage message.
        printUsageMessage();
        return 1; // End program with an input error.
    }
    int numberOfFiles = argc - 1;
    int numberOfValidFiles = 0;
    fileInfo * fileInformations[numberOfFiles]; // An array to store the file informations.
    int i; // Loop invariant.
    pid_t pids[numberOfFiles]; // An array to store IDs of child processes.
    pid_t pid; // Process ID
    int fd[numberOfFiles][2];

    for (i = 1; i <= numberOfFiles; i++) { // change this
        if (pipe(fd[i - 1]) == -1) { //Creates pipe for communication.
            fprintf(stderr, "Pipe failed.\n");
            return (2); // End program with a pipe error.
        }

        pid = fork();

        if (pid < 0) {
            fprintf(stderr, "Fork failed.\n");
            return (3); // End program with a fork error.
        } else if (pid == 0) { // Child processes will continue here.
            FILE *sourceFilePointer = fopen(argv[i], "rb");
            if (sourceFilePointer != NULL) {
                char *medianWord = malloc(MAXIMUM_WORD_LENGTH + NULL_CHARACTER_SIZE);
                size_t wordCount;
                processFile(sourceFilePointer, medianWord, &wordCount);
                fclose(sourceFilePointer);
                char *message = malloc(BUFFER_SIZE); // Message to send to the main process.
                snprintf(message, BUFFER_SIZE, "%s %zd", medianWord, wordCount); // Prepare the message.
                close(fd[i - 1][READ_END]); // Close the unused end.
                write(fd[i - 1][WRITE_END], message, BUFFER_SIZE); // Write the message to the pipe.
                close(fd[i - 1][WRITE_END]); // Close the write end.
                free(medianWord);
                free(message);
                exit(0); // Terminate process with no errors.
            } else { // Terminate process if file is not valid. i.e. does not exist.
                close(fd[i - 1][READ_END]); // Close the unused end.
                close(fd[i - 1][WRITE_END]); // Close the unused end.
                exit(1); // Terminate process with file error.
            }
        } else { // Main process will continue here.
            pids[i - 1] = pid; // Store the child process' ID.
            continue; // Explicit statement to show that the loop will continue.
        }
    }
    /* Checks if each process is finished and creates the fileInformations array.*/
    for (i = 0; i < numberOfFiles; i++) {
        pid_t pid = pids[i];
        int status; // Status returned by the child process.
        waitpid(pid, &status, 0); // Wait for child process and write its status into status variable.
        if (status == 0) {
            char *message = malloc(BUFFER_SIZE);
            close(fd[i][WRITE_END]); // Close the unused end.
            read(fd[i][READ_END], message, BUFFER_SIZE); // Read the message from the pipe.
            close(fd[i][READ_END]); // Close the read end.
            fileInformations[numberOfValidFiles] = malloc(sizeof (fileInfo)); // Create a fileInformation entry.
            copyMessageContentsToStruct(message, fileInformations[numberOfValidFiles], argv[i + 1]);
            numberOfValidFiles++; // Increment the number of valid files.
            free(message);
        } else if (WEXITSTATUS(status) == 1) { // If the child process was given an invalid file...
            close(fd[i][READ_END]); // Close the unused end.
            close(fd[i][WRITE_END]); // Close the unused end.
            printUnableToOpenFileMessage(argv[i + 1]);
        }
    }

    qsort(fileInformations, (size_t) numberOfValidFiles, sizeof (fileInfo *), fileComparator);
    printResults(fileInformations, numberOfValidFiles);

    // Clean garbage.
    for (i = 0; i < numberOfValidFiles; i++) {
        free(fileInformations[i]);
    }

    return 0; // Exit program with no errors.

}

// Prints instructions on how to use the program.

void printUsageMessage() {
    fprintf(stderr, "You did not input any files. \n");
    fprintf(stderr, "You can input as many files as you want. \n");
    fprintf(stderr, "Usage: \n");
    fprintf(stderr, "prs arg1 arg2 ... argn \n");
}

// Prints an error message in case a file can not be opened.

void printUnableToOpenFileMessage(char *fileName) {
    fprintf(stderr, "Could not open file %s. \n", fileName);
}

// Prints results for inputted files.

void printResults(fileInfo *fileInformations[], int numberOfFiles) {
    int i;
    for (i = 0; i < numberOfFiles; i++) {
        printf("%s %d %s\n", (fileInformations[i])->fileName, (int) (fileInformations[i])->wordCount,
                (fileInformations[i])->medianWord);
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

// Copies the content of the message read from the pipe to fileInformations struct.

void copyMessageContentsToStruct(char *message, fileInfo *fileInformation, char *fileName) {
    char *medianWord;
    char *wordCountString;
    medianWord = strtok(message, " "); // Copy the word in message to medianWord.
    wordCountString = strtok(NULL, " "); // Copy the word count in message to wordCountString.
    size_t wordCount = (size_t) atoi(wordCountString); // Convert the copied word count from string to size_t.
    strncat(fileInformation->medianWord, medianWord, MAXIMUM_WORD_LENGTH);
    strncat(fileInformation->fileName, fileName, MAXIMUM_FILE_LENGTH);
    fileInformation->wordCount = wordCount;

}


// Processes the file

void processFile(FILE *sourceFilePointer, char *medianWord, size_t *wordCount) {
    int numberOfWords = calculateNumberOfWords(sourceFilePointer); // Total number of words in the file.
    rewind(sourceFilePointer); // Rewinds the sourceFilePointer to be able to read from it again.
    WordArray * arrayOfDistinctWords[numberOfWords]; // An array of pointers pointing to word structures to store distinct words.
    size_t numberOfDistinctWordsInArray = 0; // Number of distinct words in the file.
    int i = 0;

    for (i = 0; i < numberOfWords; i++) { // Get each word in the file and process.
        char *word = getNextWord(sourceFilePointer);
        int index = indexOfWordInArray(word, arrayOfDistinctWords, numberOfDistinctWordsInArray);
        if (index > -1) { // If word exists in arrayOfDistinctWords increase its frequency.
            (arrayOfDistinctWords[index])->freq++;
        } else { // If it doesn't exist insert it into arrayOfDistinctWords and initialize freq as 1.
            arrayOfDistinctWords[numberOfDistinctWordsInArray] = malloc(sizeof (WordArray));
            strncat((arrayOfDistinctWords[numberOfDistinctWordsInArray])->word, word, MAXIMUM_WORD_LENGTH);
//            strcpy((arrayOfDistinctWords[numberOfDistinctWordsInArray])->word, word);
            arrayOfDistinctWords[numberOfDistinctWordsInArray]->freq = 1;
            numberOfDistinctWordsInArray++;
        }
        free(word);
    }

    qsort(arrayOfDistinctWords, numberOfDistinctWordsInArray, sizeof (WordArray *), wordComparator);

    //Pass the results to calling function.
    *wordCount = numberOfDistinctWordsInArray;
    strncat(medianWord, findMedianWord(arrayOfDistinctWords, (int) numberOfDistinctWordsInArray), MAXIMUM_WORD_LENGTH);
//    strcpy(medianWord, findMedianWord(arrayOfDistinctWords, (int) numberOfDistinctWordsInArray));

    for (i = 0; i < numberOfDistinctWordsInArray; i++) { // Free the memories at each of the pointers are pointing to in the distinct word array.
        free(arrayOfDistinctWords[i]);
    }
}
