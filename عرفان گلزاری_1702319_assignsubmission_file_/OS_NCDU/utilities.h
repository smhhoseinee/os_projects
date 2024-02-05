//
// Created by Erfan on 12/23/2023.
//
struct iFile{
    int valid;
    unsigned long size;
    char name[255];
    char extension[10];
    char path[1000];
};

struct iDirectory{
    int valid;
    char name[255];
    char path[1000];
};

struct extensionCount {
    char extension[10];
    int count;
};




long int findSize(char file_name[])
{
    FILE* fp = fopen(file_name, "r");
    if (fp == NULL) {
        printf("File Not Found!\n");
        return -1;
    }

    fseek(fp, 0L, SEEK_END);
    long int res = ftell(fp);
    fclose(fp);

    return res;
}
char *get_extension(const char* fileName){
    char *dot = strrchr(fileName,'.');
    if(!dot || dot == fileName) return "";
    return dot + 1 ;
}

char *parser(const char *s, const char *oldW,
             const char *newW) {
    char *result;
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);

    // Counting the number of times old word
    // occur in the string
    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], oldW) == &s[i]) {
            cnt++;

            // Jumping to index after the old word.
            i += oldWlen - 1;
        }
    }

    // Making new string of enough length
    result = (char *) malloc(i + cnt * (newWlen - oldWlen) + 1);

    i = 0;
    while (*s) {
        // compare the substring with the result
        if (strstr(s, oldW) == s) {
            strcpy(&result[i], newW);
            i += newWlen;
            s += oldWlen;
        } else
            result[i++] = *s++;
    }

    result[i++] = '\0';
    return result;
}
