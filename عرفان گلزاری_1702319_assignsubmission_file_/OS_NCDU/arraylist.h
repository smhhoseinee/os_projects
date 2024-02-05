struct task {
    int filesCount;
    int directoryCount;
    int extensionsCount;
    unsigned long long dirSize;
    struct iFile maxSize;
    struct iFile minSize;
    struct iDirectory directory[10];
    struct iFile files[10];
    struct extensionCount extensions[10];
};


struct thread_arg{
    struct task* task;
    struct task* threadTask;
    char* path;
};

void appendToExtensionArray(struct task *array, struct extensionCount extension) {
    array->extensions[array->extensionsCount] = extension;
    array->extensionsCount++;
}

void appendToFileArray(struct task *array, struct iFile newFile) {
    array->files[array->filesCount] = newFile;
    array->filesCount++;
}

void appendToDirectoryArray(struct task *array, struct iDirectory newDirectory) {
    array->directory[array->directoryCount] = newDirectory;
    array->directoryCount++;
}

void extensionTypes(char extensionName[], struct task *array) {

    int flag = 0;
    for (int i = 0; i < array->extensionsCount; i++) {
        if (strcmp(array->extensions[i].extension, extensionName) == 0) {
            array->extensions[i].count++;
            flag = 1;
        }
    }
    if(flag == 0){
        struct extensionCount ex;
        strcpy(ex.extension, extensionName);
        if(ex.count > 1)
            ex.count = ex.count + 1;
        appendToExtensionArray(array, ex);
    }
}

void extensionTypesWithCount(struct extensionCount extension, struct task *array) {

    int flag = 0;
    for (int i = 0; i < array->extensionsCount; i++) {
        if (strcmp(array->extensions[i].extension, extension.extension) == 0) {
            array->extensions[i].count += extension.count;
            flag = 1;
        }
    }
    if(flag == 0){
        appendToExtensionArray(array, extension);
    }
}