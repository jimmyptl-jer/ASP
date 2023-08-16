/**
 * File Client for Interacting with Server for File Manipulation and Searching
 * This program implements a client that interacts with a server to perform various file-related commands.
 *
 * Main Components and Functionalities:
 *
 * 1. Header Inclusions:
 *    - Necessary header files for system functions, socket handling, and string manipulation.
 *
 * 2. Macro Definitions:
 *    - PORT: The port number of the server to connect to.
 *    - mirror_port: The port number of the mirror server.
 *    - SERVER_IP: The IP address of the server to connect to.
 *    - BUFSIZE: Buffer size for character arrays.
 *
 * 3. Global Function: validDate(const char *date)
 *    - Validates a date string in the "YYYY-MM-DD" format.
 *
 * 4. Global Function: unzipTempFile()
 *    - Calls the system command to unzip the "temp.tar.gz" file.
 *
 * 5. Main Function:
 *    - Initializes variables for input and socket handling.
 *    - Creates a socket to connect to the server.
 *    - Reads user input and processes it for sending to the server.
 *    - Handles various commands:
 *      - "filesrch": Searches for a file in the server's filesystem.
 *      - "tarfgetz": Creates a tar file containing files within a size range.
 *      - "getdirf": Creates a tar file containing files within a date range.
 *      - "fgets": Sends user input as a command directly to the server.
 *      - "targzf": Creates a tar file containing files with specified extensions.
 *      - "quit": Closes the client connection and exits the program.
 *    - Sends the processed command to the server.
 *    - Reads the server's response and displays it.
 *    - Checks if the "-u" flag is set and performs unzipping.
 *    - Reconnects to a mirror server if indicated by the server's response.
 *    - Closes the client socket and exits the program.
 *
 * Note: The client code interacts with the server by sending commands and receiving responses.
 * The client also handles unzipping and reconnecting to a mirror server if indicated.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define mirror_port 8089
#define SERVER_IP "127.0.0.1"

#define BUFSIZE 1024

int validDate(const char *date)
{
    int year, month, day;
    if (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3)
    {
        return 0;
    }

    if (year < 0 || month < 1 || month > 12 || day < 1)
    {
        return 0;
    }

    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
    {
        daysInMonth[1] = 29;
    }

    if (day > daysInMonth[month - 1])
    {
        return 0;
    }

    return 1;
}

void unzipTempFile()
{
    system("tar -xzvf temp.tar.gz");
}

int main(int argc, char const *argv[])
{
    void unzipTempFile()
    {
        system("tar -xzvf temp.tar.gz");
    }

    char buff[BUFSIZE];
    char command[BUFSIZE];
    char userInput[2048];
    int isValid;

    int socket_fd;
    struct sockaddr_in serv_addr, mirror_addr;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        exit(1);
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        printf("\ninet_pton() has failed\n");
        exit(2);
    }

    if (connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nconnect() failed, exiting");
        exit(3);
    }

    printf("Server Connection: SUCCESS.\n");
    printf("Please enter a command:\n");

    while (1)
    {
        isValid = 1;
        memset(buff, 0, sizeof(buff));
        memset(command, 0, sizeof(command));

        fgets(buff, sizeof(buff), stdin);
        memcpy(userInput, buff, sizeof(buff));
        buff[strcspn(buff, "\n")] = 0;

        int unzipFlag = 0; // Initialize to 0; 1 indicates unzip should be performed

        char *command = strtok(buff, " ");
        if (command == NULL)
        {
            isValid = 0;
        }
        else if (strcmp(command, "filesrch") == 0)
        {
            char *searchFile = strtok(NULL, " ");
            if (searchFile == NULL)
            {
                isValid = 0;
            }
            else
            {
                sprintf(command, "filesrch %s", searchFile);
            }
        }
        else if (strcmp(command, "tarfgetz") == 0)
        {
            char *s1 = strtok(NULL, " ");
            char *s2 = strtok(NULL, " ");
            if (s1 == NULL || s2 == NULL || atoi(s1) < 0 || atoi(s2) < 0 || atoi(s2) < atoi(s1))
            {
                isValid = 0;
            }

            else
            {
                char *u = strtok(NULL, " ");
                if (u != NULL && strcmp(u, "-u") == 0)
                {
                    sprintf(command, "tarfgetz %s %s -u", s1, s2);
                    unzipFlag = 1;
                }
                else
                {
                    sprintf(command, "tarfgetz %s %s", s1, s2);
                }
            }
        }
        else if (strcmp(command, "getdirf") == 0)
        { // function deals with dgetfiles command
            char *d1 = strtok(NULL, " ");
            char *d2 = strtok(NULL, " ");
            if (d1 == NULL || d2 == NULL || !validDate(d1) || !validDate(d2))
            {
                isValid = 0;
            }
            else
            {
                char *u = strtok(NULL, " ");
                if (u != NULL && strcmp(u, "-u") == 0)
                {
                    sprintf(command, "getdirf %s %s -u", d1, d2);
                    unzipFlag = 1;
                }
                else
                {
                    sprintf(command, "getdirf %s %s", d1, d2);
                }
            }
        }
        else if (strcmp(command, "fgets") == 0)
        {
            userInput[strcspn(userInput, "\n")] = '\0';
            sprintf(command, userInput);
            printf("%s", command);
        }
        else if (strcmp(command, "targzf") == 0)
        {
            userInput[strcspn(userInput, "\n")] = '\0';
            sprintf(command, userInput);

            // Check if -u flag is present in the command
            char *u_flag = strstr(command, "-u");
            if (u_flag != NULL)
            {
                unzipFlag = 1;
            }
            else
            {
                printf("-u flag is not present.\n");
            }
        }

        else if (strcmp(command, "quit") == 0)
        {
            sprintf(command, "quit");
        }
        else
        {
            isValid = 0;
        }

        if (!isValid)
        {
            printf("Invalid syntax. Please try again.\n");
            continue;
        }

        // Send command to server
        send(socket_fd, command, strlen(command), 0);

        char reply[1024] = {0};
        int val = read(socket_fd, reply, sizeof(reply));

        reply[strcspn(reply, "\n")] = '\0';
        if (strcmp(reply, "8089") == 0)
        {
            close(socket_fd); // closing the current server connection
            printf("In mirror please enter the command again\n");
            // Create a new socket for the mirror server
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd == -1)
            {
                perror("socket");
                exit(EXIT_FAILURE);
            }

            struct sockaddr_in mirror_addr;
            memset(&mirror_addr, '\0', sizeof(mirror_addr));
            mirror_addr.sin_family = AF_INET;
            mirror_addr.sin_port = htons(8089);
            mirror_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

            if (connect(socket_fd, (struct sockaddr *)&mirror_addr, sizeof(mirror_addr)) == -1)
            {
                perror("Connection");
                exit(EXIT_FAILURE);
            }
        }
        else
            printf("%s\n", reply);

        // Check if unzipFlag is set and perform unzipping
        if (unzipFlag)
        {
            // Call the unzip function
            unzipTempFile();
        }

        if (strcmp(command, "quit") == 0)
        {
            break;
        }

        printf("Please enter command:\n");
    }

    close(socket_fd);
    printf("Connection closed.\n");

    return 0;
}
