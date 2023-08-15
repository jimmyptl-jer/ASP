#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

#define PORT 8089
#define _XOPEN_SOURCE 500
#define BUFSIZE 1024

char path[PATH_MAX];

// Define function prototypes for each command
void processclient(int socket);
void mirrorRedirection(int socket);
int validDate(const char *date);

void execute_tarfgetz_command(int sockfd, char *size1_str, char *size2_str, char *uZipFlag, char *command);
void execute_getdirf_command(int sockfd, char *date1_str, char *date2_str, char *uZipFlag, char *command);
void execute_fgets_command(int sockfd, const char *args);
void execute_tar_command(int sockfd, char *extensions[], char *uZipFlag);

void execute_tarfgetz_command(int sockfd, char *size1_str, char *size2_str, char *uZipFlag, char *command)
{
    if (size1_str == NULL || size2_str == NULL)
    {
        sprintf(command, "Invalid syntax. Please try again.\n");
    }
    else
    {
        long long size1 = atoll(size1_str); // Convert to long long for bytes
        long long size2 = atoll(size2_str);

        if (size1 < 0 || size2 < 0 || size1 > size2)
        {
            sprintf(command, "Invalid size range. Please try again.\n");
        }
        else
        {
            char root_path[1024];
            sprintf(root_path, "./");

            // Find files matching the size range in bytes
            char command_buf[BUFSIZE];
            sprintf(command_buf, "find %s -type f -size +%lldc -size -%lldc -print0 | xargs -0 tar -czf temp.tar.gz",
                    root_path, size1, size2);
            int status = system(command_buf);

            // Check if the files were found and tar created successfully
            if (status == 0)
            {
                wait(NULL);
                sprintf(command, "Tar file created successfully.... -u not provided\n");

                // Send tar created successfully message
                char tar_success_msg[] = "Tar file created successfully.\n";
                send(sockfd, tar_success_msg, strlen(tar_success_msg), 0);
            }
            else
            {
                sprintf(command, "No file found.\n");
            }
        }
    }
}

void execute_getdirf_command(int sockfd, char *date1_str, char *date2_str, char *unzip_flag, char *command)
{
    if (!validDate(date1_str) || !validDate(date2_str))
    {
        sprintf(command, "Invalid date format or values. YYYY-MM-DD format\n");
    }
    else
    {
        char root_path[1024];
        sprintf(root_path, "./");

        // Find files matching the date range
        char command_buf[BUFSIZE];
        sprintf(command_buf, "find %s -type f -newermt \"%s\" ! -newermt \"%s\" -print0 | xargs -0 tar -czf temp.tar.gz",
                root_path, date1_str, date2_str);
        int status = system(command_buf);

        // Check if the files were found and tar created successfully
        if (status == 0)
        {

            sprintf(command, "Files retrieved successfully. Use the -u flag to unzip.\n");

            // Send tar created successfully message
            char tar_success_msg[] = "Tar file created successfully.\n";
            send(sockfd, tar_success_msg, strlen(tar_success_msg), 0);
        }
        else
        {
            sprintf(command, "No file found.\n");
        }
    }
}

void execute_fgets_command(int sockfd, const char *args)
{
    char *file_tokens[4]; // Assuming a maximum of 4 file tokens (file1, file2, file3, file4)
    char *token = strtok(args, " ");
    int num_files = 0;

    while (token != NULL && num_files < 4)
    {
        file_tokens[num_files] = token;
        token = strtok(NULL, " ");
        num_files++;
    }

    // Constructing the files expression for find command
    char files_expr[BUFSIZE];
    strcpy(files_expr, "");
    for (int i = 0; i < num_files; i++)
    {
        if (i > 0)
        {
            strcat(files_expr, " -o ");
        }
        strcat(files_expr, "\\( -iname \"");
        strcat(files_expr, file_tokens[i]);
        strcat(files_expr, "\" \\)");
    }

    // Check if any of the specified files are present
    char command_buf[BUFSIZE];
    sprintf(command_buf, "find . -type f %s -print0 | xargs -0 tar -czf temp.tar.gz",
            files_expr);

    int status = system(command_buf);

    if (status == 0)
    {
        char success_message[] = "Tar created successfully.\n";
        send(sockfd, success_message, strlen(success_message), 0);
    }
    else
    {
        char not_found_message[] = "No files found or tar creation failed.\n";
        send(sockfd, not_found_message, strlen(not_found_message), 0);
    }
}

void execute_targzf_command(int sockfd, char *extensions[])
{
    char *root_path = "./";

    // Check if any of the specified files are present
    char command_buf[BUFSIZE];
    sprintf(command_buf, "find %s -type f \\( ", root_path);

    for (int i = 0; i < 6; i++)
    {
        if (extensions[i] != NULL)
        {
            sprintf(command_buf + strlen(command_buf), "-iname \"*.%s\" -o ", extensions[i]);
        }
    }

    sprintf(command_buf + strlen(command_buf), "-false \\) -print0 | xargs -0 tar -czf temp.tar.gz");

    int status = system(command_buf);

    // Check if the files were found
    if (status == 0)
    {
        // Send the tar created successfully message
        char tar_success_msg[] = "Tar file created successfully.\n";
        send(sockfd, tar_success_msg, strlen(tar_success_msg), 0);
    }
    else
    {
        char error_msg[] = "No file found.\n";
        send(sockfd, error_msg, strlen(error_msg), 0);
    }
}

void searchFile(const char *filename, const char *currentPath, char *result)
{
    DIR *dir = opendir(currentPath);
    if (dir == NULL)
    {
        sprintf(result, "Failed to open directory.\n");
        return;
    }

    struct dirent *entry;
    struct stat fileStat;
    char filePath[MAX_PATH_LENGTH];

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        snprintf(filePath, sizeof(filePath), "%s/%s", currentPath, entry->d_name);

        if (stat(filePath, &fileStat) < 0)
        {
            sprintf(result, "Failed to get file information.\n");
            continue;
        }

        if (S_ISDIR(fileStat.st_mode))
        {
            searchFile(filename, filePath, result); // Recursive call for directories
        }
        else if (S_ISREG(fileStat.st_mode))
        {
            if (strcmp(entry->d_name, filename) == 0)
            {
                char file_info[BUFSIZE];
                sprintf(file_info, "File found: %s | Location: %s | Size: %ld bytes | Date created: %s", entry->d_name, filePath, fileStat.st_size, ctime(&fileStat.st_ctime));
                strcpy(result, file_info);
                closedir(dir);
                return;
            }
        }
    }

    sprintf(result, "File not found.\n");
    closedir(dir);
}

void execute_filesrch_command(const char *filename, int sockfd)
{
    char rootPath[MAX_PATH_LENGTH];
    strcpy(rootPath, getenv("HOME")); // Get the root directory path

    char result[BUFSIZE];
    searchFile(filename, rootPath, result);

    // Send the result back to the client
    send(sockfd, result, strlen(result), 0);
}

void quit(int sockfd)
{
    char response[BUFSIZE] = "Clossing client connection.\n";
    send(sockfd, response, strlen(response), 0);
    close(sockfd);
    exit(0);
}

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

void processclient(int sockfd)
{
    char buffer[1024] = {0};
    char command[1024] = {0};

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        memset(command, 0, sizeof(command));

        int valread = read(sockfd, buffer, sizeof(buffer));
        buffer[valread] = '\0';

        char *token = strtok(buffer, " ");
        if (token == NULL)
        {
            snprintf(command, sizeof(command), "Invalid syntax. Please try again.\n");
        }
        else if (strcmp(token, "filesrch") == 0)
        {
            char *filename = strtok(NULL, " ");
            if (filename == NULL)
            {
                sprintf(command, "Invalid syntax. Please try again.\n");
            }
            else
            {
                execute_filesrch_command(filename, sockfd); // Pass sockfd as the second argument
            }
        }
        else if (strcmp(token, "tarfgetz") == 0)
        {
            char *size1_str = strtok(NULL, " ");
            char *size2_str = strtok(NULL, " ");
            char *uZipFlag = strtok(NULL, " ");

            execute_tarfgetz_command(sockfd, size1_str, size2_str, uZipFlag, command);
        }
        else if (strcmp(token, "getdirf") == 0)
        {
            char *date1_str = strtok(NULL, " ");
            char *date2_str = strtok(NULL, " ");
            char *uZipFlag = strtok(NULL, " ");

            execute_getdirf_command(sockfd, date1_str, date2_str, uZipFlag, command);
        }
        else if (strcmp(token, "fgets") == 0)
        {
            char *args = strtok(NULL, "\n");
            execute_fgets_command(sockfd, args);
        }
        else if (strcmp(token, "targzf") == 0)
        {
            char *ext1 = strtok(NULL, " ");
            char *ext2 = strtok(NULL, " ");
            char *ext3 = strtok(NULL, " ");
            char *ext4 = strtok(NULL, " ");
            char *ext5 = strtok(NULL, " ");
            char *ext6 = strtok(NULL, " ");
            char *uZipFlag = strtok(NULL, " ");

            // Create an array of extensions for easier handling
            char *ext[6] = {ext1, ext2, ext3, ext4, ext5, ext6};

            // Call the execute_targzf_command function
            execute_targzf_command(sockfd, ext);
        }

        else if (strcmp(token, "quit") == 0)
        {
            quit(sockfd);
            break;
        }
        else
        {
            snprintf(command, sizeof(command), "Invalid syntax. Please try again.\n");
            send(sockfd, command, strlen(command), 0);
        }
    }

    close(sockfd);
    exit(0);
}

int main(int argc, char const *argv[])
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Attach socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("New client connected. Forking child process...\n");

        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        { // Child process
            close(server_fd);
            processclient(new_socket);
        }
        else
        { // Parent process
            close(new_socket);
            while (waitpid(-1, NULL, WNOHANG) > 0)
                ; // Clean up zombie processes
        }
    }

    return 0;
}
