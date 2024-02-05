# Application Usage Guide

Follow the steps below to compile and execute the application successfully.

## Compilation

1. Navigate to the directory containing the `main.c` file of the application using the terminal:

    ```bash
    cd path/to/your/directory
    ```

2. Compile the application using `gcc` with the `-pthread` flag:

    ```bash
    gcc main.c -o app -pthread
    ```

## Execution

1. Run the compiled `app` file from the terminal:

    ```bash
    ./app
    ```

2. The application will prompt you to enter the address or path of the directory you want it to work on. Provide the address as requested by the application when prompted.