/*
  showdocs.c

 Simple C-based HTTP server that listens for GET requests
 and serves requested .html files from a server's directory.
 Returns a 404 page if the file is not found,

 Usage:
 - Edit showdocs.ini to configure server settings.
 - Run ./showdocs

 If the binary is renamed, the config file should match the new name

 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200112L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <libgen.h>
#include <stdarg.h>

// Version information - can be overridden at compile time
#ifndef VERSION
#define VERSION "1.0.0"
#endif
#ifndef BUILD_DATE
#define BUILD_DATE __DATE__
#endif
#ifndef BUILD_TIME
#define BUILD_TIME __TIME__
#endif
#ifndef GIT_COMMIT
#define GIT_COMMIT "unknown"
#endif

// Platform-specific includes and definitions
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#define close closesocket
typedef int socklen_t;
#define PATH_SEP '\\'
#define PATH_SEP_STR "\\"
#else
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define PATH_SEP '/'
#define PATH_SEP_STR "/"
#endif

#define BUFFER_SIZE 4096
#define MAX_PATH_LEN 512
#define MAX_CMD_LEN 1024

// Global variables for signal handling
static volatile int g_run = 1;
#ifdef _WIN32
static SOCKET g_server_sock = INVALID_SOCKET;
#else
static int g_server_sock = -1;
#endif

// Log levels
typedef enum
{
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG
} LogLevel;

// Get current timestamp string
void get_timestamp(char *buffer, size_t max_len)
{
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buffer, max_len, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *t = localtime(&tv.tv_sec);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);
    snprintf(buffer, max_len, "%s.%03d", time_str, (int)(tv.tv_usec / 1000));
#endif
}

// Logging function with timestamp and level
void log_message(LogLevel level, const char *format, ...)
{
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    const char *level_str;
    switch (level)
    {
    case LOG_INFO:
        level_str = "INFO";
        break;
    case LOG_WARN:
        level_str = "WARN";
        break;
    case LOG_ERROR:
        level_str = "ERROR";
        break;
    case LOG_DEBUG:
        level_str = "DEBUG";
        break;
    default:
        level_str = "UNKNOWN";
        break;
    }

    printf("[%s] [%s] ", timestamp, level_str);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
    fflush(stdout);
}

// Configuration structure
typedef struct
{
    int port;
    char listen_addr[64];
    char root_dir[MAX_PATH_LEN];
    char exec_start[MAX_CMD_LEN];
    char exec_start_win[MAX_CMD_LEN];
    char exec_start_linux[MAX_CMD_LEN];
    char exec_start_macos[MAX_CMD_LEN];
} Config; // Forward declarations

void send_http_response(int client_sock, const char *status_line, const char *filename,
                        const char *date_str, const char *root_dir);

// Display version information
void print_version(void)
{
    printf("v%s\n", VERSION);
    // Format the build date to be more readable
    char formatted_date[32];
    strncpy(formatted_date, BUILD_DATE, sizeof(formatted_date));
    for (char *p = formatted_date; *p; p++)
    {
        if (*p == '_')
            *p = ' ';
    }
    printf("Built: %s %s\n", formatted_date, BUILD_TIME);
    printf("Commit: %s\n", GIT_COMMIT);
#ifdef _WIN32
    printf("Platform: Windows\n");
#elif defined(__APPLE__) || defined(__MACH__)
    printf("Platform: macOS\n");
#elif defined(__linux__)
    printf("Platform: Linux\n");
#else
    printf("Platform: Unix\n");
#endif
    printf("\n");
}

// Trim whitespace from string
void trim(char *str)
{
    char *end;
    // Trim leading space
    while (isspace((unsigned char)*str))
        str++;
    if (*str == 0)
        return;
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';
    // Move trimmed string to beginning if needed
    if (str != end)
    {
        memmove(str - (str - end), str, strlen(str) + 1);
    }
}

// Parse INI file and populate config
int parse_config(const char *config_file, Config *config)
{
    FILE *fp = fopen(config_file, "r");
    if (!fp)
    {
        return 0; // Config file not found
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp))
    {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;

        // Skip empty lines and comments
        char *trimmed = line;
        while (isspace((unsigned char)*trimmed))
            trimmed++;
        if (*trimmed == 0 || *trimmed == ';' || *trimmed == '#')
            continue;

        // Skip section headers
        if (*trimmed == '[')
            continue;

        // Parse key=value
        char *equals = strchr(trimmed, '=');
        if (equals)
        {
            *equals = 0;
            char *key = trimmed;
            char *value = equals + 1;

            // Trim key and value
            char *key_end = key + strlen(key) - 1;
            while (key_end > key && isspace((unsigned char)*key_end))
                *key_end-- = 0;
            while (isspace((unsigned char)*value))
                value++;

            // Match keys (case insensitive)
            if (strcasecmp(key, "Port") == 0)
            {
                config->port = atoi(value);
            }
            else if (strcasecmp(key, "ListenAddr") == 0 || strcasecmp(key, "ListenAddress") == 0)
            {
                strncpy(config->listen_addr, value, sizeof(config->listen_addr) - 1);
                config->listen_addr[sizeof(config->listen_addr) - 1] = 0;
            }
            else if (strcasecmp(key, "RootDir") == 0)
            {
                strncpy(config->root_dir, value, MAX_PATH_LEN - 1);
                config->root_dir[MAX_PATH_LEN - 1] = 0;
            }
            else if (strcasecmp(key, "ExecStart") == 0)
            {
                strncpy(config->exec_start, value, MAX_CMD_LEN - 1);
                config->exec_start[MAX_CMD_LEN - 1] = 0;
            }
            else if (strcasecmp(key, "ExecStart_Win") == 0 || strcasecmp(key, "ExecStart_Windows") == 0)
            {
                strncpy(config->exec_start_win, value, MAX_CMD_LEN - 1);
                config->exec_start_win[MAX_CMD_LEN - 1] = 0;
            }
            else if (strcasecmp(key, "ExecStart_Linux") == 0)
            {
                strncpy(config->exec_start_linux, value, MAX_CMD_LEN - 1);
                config->exec_start_linux[MAX_CMD_LEN - 1] = 0;
            }
            else if (strcasecmp(key, "ExecStart_MacOS") == 0 || strcasecmp(key, "ExecStart_Darwin") == 0)
            {
                strncpy(config->exec_start_macos, value, MAX_CMD_LEN - 1);
                config->exec_start_macos[MAX_CMD_LEN - 1] = 0;
            }
        }
    }

    fclose(fp);
    return 1;
}

// Get executable name and construct config filename
void get_config_filename(const char *argv0, char *config_file, size_t max_len)
{
    char exe_path[MAX_PATH_LEN];
    strncpy(exe_path, argv0, MAX_PATH_LEN - 1);
    exe_path[MAX_PATH_LEN - 1] = 0;

    // Remove .exe extension on Windows
    char *ext = strrchr(exe_path, '.');
    if (ext && (strcasecmp(ext, ".exe") == 0))
    {
        *ext = 0;
    }

    // Truncate at first underscore if present (e.g., showdocs_Windows -> showdocs)
    char *underscore = strchr(exe_path, '_');
    if (underscore)
    {
        *underscore = 0;
    }

    snprintf(config_file, max_len, "%s.ini", exe_path);
} // Build full path from root_dir and relative path
void build_full_path(const char *root_dir, const char *relative_path, char *full_path, size_t max_len)
{
    if (root_dir[0] == 0)
    {
        // No root dir, use relative path as-is
        strncpy(full_path, relative_path, max_len - 1);
    }
    else
    {
        // Combine root_dir and relative_path
        size_t root_len = strlen(root_dir);
        int needs_sep = (root_dir[root_len - 1] != PATH_SEP && root_dir[root_len - 1] != '/');

        if (needs_sep)
        {
            snprintf(full_path, max_len, "%s%s%s", root_dir, PATH_SEP_STR, relative_path);
        }
        else
        {
            snprintf(full_path, max_len, "%s%s", root_dir, relative_path);
        }
    }
    full_path[max_len - 1] = 0;
}

// Function to get current GMT date in HTTP  friendly format
void get_gmt_date(char *date_buffer, size_t max_len)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = gmtime(&rawtime);

    strftime(date_buffer, max_len, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
}

// Initialize Winsock on Windows
void init_networking(void)
{
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(EXIT_FAILURE);
    }
#endif
}

// Cleanup networking resources
void cleanup_networking(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

// Signal handler for graceful shutdown
#ifdef _WIN32
BOOL WINAPI signal_handler(DWORD signal)
{
    static volatile int called = 0;

    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT || signal == CTRL_BREAK_EVENT)
    {
        if (called)
        {
            // Second signal, force exit
            return FALSE;
        }
        called = 1;

        log_message(LOG_INFO, "Received termination signal, shutting down gracefully...");
        g_run = 0;
        // Don't close socket here - let select() timeout handle it
        return TRUE;
    }
    return FALSE;
}
#else
void signal_handler(int signal)
{
    static volatile sig_atomic_t called = 0;

    if (signal == SIGINT || signal == SIGTERM)
    {
        if (called)
        {
            // Second signal, force exit
            _exit(0);
        }
        called = 1;

        log_message(LOG_INFO, "Received termination signal, shutting down gracefully...");
        g_run = 0;
        // Don't close socket here - let select() timeout handle it
    }
}
#endif // Setup signal handlers
void setup_signal_handlers(void)
{
#ifdef _WIN32
    if (!SetConsoleCtrlHandler(signal_handler, TRUE))
    {
        log_message(LOG_WARN, "Failed to set console control handler");
    }
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        log_message(LOG_WARN, "Failed to set SIGINT handler");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        log_message(LOG_WARN, "Failed to set SIGTERM handler");
    }
#endif
}

// Initialize configuration with defaults
void init_config(Config *config)
{
    config->port = 8080;
    strncpy(config->listen_addr, "127.0.0.1", sizeof(config->listen_addr) - 1);
    config->listen_addr[sizeof(config->listen_addr) - 1] = '\0';
    config->root_dir[0] = 0;
    config->exec_start[0] = 0;
    config->exec_start_win[0] = 0;
    config->exec_start_linux[0] = 0;
    config->exec_start_macos[0] = 0;
} // Load configuration from file and command line
void load_config(Config *config, int argc, char *argv[])
{
    // Handle --version flag
    if (argc > 1 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0))
    {
        exit(EXIT_SUCCESS);
    }

    char config_file[MAX_PATH_LEN];
    get_config_filename(argv[0], config_file, sizeof(config_file));

    if (parse_config(config_file, config))
    {
        log_message(LOG_INFO, "Loaded configuration from: %s", config_file);
    }
    else
    {
        log_message(LOG_WARN, "No config file found (%s), using defaults", config_file);
    }

    // Command line arguments override config file
    if (argc > 1)
    {
        if (strcmp(argv[1], "--port") == 0 && argc > 2)
        {
            config->port = atoi(argv[2]);
            if (config->port <= 0)
            {
                log_message(LOG_ERROR, "Invalid port number");
                cleanup_networking();
                exit(EXIT_FAILURE);
            }
        }
    }

    log_message(LOG_INFO, "Using port: %d", config->port);
    if (config->root_dir[0])
    {
        log_message(LOG_INFO, "Root directory: %s", config->root_dir);
    }
}

// Create and configure server socket
#ifdef _WIN32
SOCKET create_server_socket(Config *config)
{
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock == INVALID_SOCKET)
    {
#else
int create_server_socket(Config *config)
{
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
#endif
        log_message(LOG_ERROR, "Socket creation failed");
        cleanup_networking();
        exit(EXIT_FAILURE);
    }

    // Set SO_REUSEADDR to allow immediate reuse of the port
    int reuse = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR,
                   (const char *)&reuse, sizeof(reuse)) < 0)
    {
        log_message(LOG_WARN, "Failed to set SO_REUSEADDR");
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config->port);

    // Parse the listen address
    if (inet_pton(AF_INET, config->listen_addr, &server_addr.sin_addr) <= 0)
    {
        log_message(LOG_ERROR, "Invalid listen address: %s", config->listen_addr);
        close(server_sock);
        cleanup_networking();
        exit(EXIT_FAILURE);
    }

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        log_message(LOG_ERROR, "Bind failed on %s:%d", config->listen_addr, config->port);
        close(server_sock);
        cleanup_networking();
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 10) < 0)
    {
        log_message(LOG_ERROR, "Listen failed");
        close(server_sock);
        cleanup_networking();
        exit(EXIT_FAILURE);
    }

    log_message(LOG_INFO, "Web server started successfully on %s:%d", config->listen_addr, config->port);
    return server_sock;
} // Determine which ExecStart command to use


const char *get_exec_command(Config *config)
{
    const char *exec_cmd = NULL;

#ifdef _WIN32
    if (config->exec_start_win[0] != 0)
    {
        exec_cmd = config->exec_start_win;
    }
#elif defined(__APPLE__) || defined(__MACH__)
    if (config->exec_start_macos[0] != 0)
    {
        exec_cmd = config->exec_start_macos;
    }
#elif defined(__linux__)
    if (config->exec_start_linux[0] != 0)
    {
        exec_cmd = config->exec_start_linux;
    }
#endif

    // Fallback to generic ExecStart if platform-specific not set
    if (exec_cmd == NULL && config->exec_start[0] != 0)
    {
        exec_cmd = config->exec_start;
    }

    return exec_cmd;
}

// Execute startup command in background
void execute_startup_command(const char *exec_cmd)
{
    if (exec_cmd == NULL)
        return;

    log_message(LOG_INFO, "Executing startup command: %s", exec_cmd);

#ifdef _WIN32
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    char cmd_with_shell[MAX_CMD_LEN + 20];
    snprintf(cmd_with_shell, sizeof(cmd_with_shell), "cmd.exe /c %s", exec_cmd);

    if (!CreateProcess(NULL, cmd_with_shell, NULL, NULL, FALSE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        log_message(LOG_WARN, "Failed to execute startup command (error %lu)", GetLastError());
    }
    else
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#else
    pid_t pid = fork();
    if (pid == 0)
    {
        system(exec_cmd);
        exit(0);
    }
    else if (pid < 0)
    {
        log_message(LOG_WARN, "Failed to fork for startup command");
    }
#endif
}

// Handle incoming HTTP request
#ifdef _WIN32
int handle_request(SOCKET client_sock, Config *config)
{
#else
int handle_request(int client_sock, Config *config)
{
#endif
    char request[BUFFER_SIZE];
    memset(request, 0, sizeof(request));

    ssize_t bytes_received = recv(client_sock, request, sizeof(request) - 1, 0);
    if (bytes_received <= 0)
        return 1;

    strtok(request, " ");
    char *path = strtok(NULL, " ");

    // If no path or path is "/", default to index.html
    if (!path || strcmp(path, "/") == 0)
    {
        path = "index.html";
    }
    else if (path[0] == '/')
    {
        path++; // Remove leading '/'
    }

    log_message(LOG_INFO, "Request: %s", path);

    char date_buffer[128];
    get_gmt_date(date_buffer, sizeof(date_buffer));

    // Serve requested file or 404
    char full_path[MAX_PATH_LEN];
    build_full_path(config->root_dir, path, full_path, sizeof(full_path));
    FILE *fp = fopen(full_path, "rb");
    if (fp)
    {
        fclose(fp);
        send_http_response(client_sock, "HTTP/1.1 200 OK", path, date_buffer, config->root_dir);
        log_message(LOG_INFO, "200 OK: %s", path);
    }
    else
    {
        char full_404_path[MAX_PATH_LEN];
        build_full_path(config->root_dir, "404.html", full_404_path, sizeof(full_404_path));
        FILE *fp404 = fopen(full_404_path, "rb");
        if (fp404)
        {
            fclose(fp404);
            send_http_response(client_sock, "HTTP/1.1 404 Not Found", "404.html", date_buffer, config->root_dir);
            log_message(LOG_WARN, "404 Not Found: %s", path);
        }
        else
        {
            log_message(LOG_ERROR, "404 page not found and no 404.html available");
        }
    }

    return 1; // Continue running
}

// Sends the file contents plus a built HTTP header for status_line
void send_http_response(int client_sock,
                        const char *status_line,
                        const char *filename,
                        const char *date_str,
                        const char *root_dir)
{
    // Build full path with root directory
    char full_path[MAX_PATH_LEN];
    build_full_path(root_dir, filename, full_path, sizeof(full_path));

    // Attempt to open the file
    FILE *fp = fopen(full_path, "rb");
    if (!fp)
    {
        // If file can't be opened, return
        return;
    }

    // Finds file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    // Builds header string
    char header[512];
    snprintf(header, sizeof(header),
             "%s\r\n"
             "Content-Type: text/html\r\n"
             "Date: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n",
             status_line, date_str, file_size);

    // Sends header
    send(client_sock, header, strlen(header), 0);

    // Sendsfile contents
    char file_buffer[BUFFER_SIZE];

    size_t bytes_read;
    while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), fp)) > 0)
    {
        send(client_sock, file_buffer, bytes_read, 0);
    }

    fclose(fp);
}

int main(int argc, char *argv[])
{
    print_version();
    init_networking();

    Config config;
    init_config(&config);
    load_config(&config, argc, argv);

    g_server_sock = create_server_socket(&config);

    // Setup signal handlers for graceful shutdown
    setup_signal_handlers();

    const char *exec_cmd = get_exec_command(&config);
    execute_startup_command(exec_cmd);

    // Main server loop
    while (g_run)
    {
        // Use select() with timeout to allow checking g_run periodically
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(g_server_sock, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 1; // 1 second timeout
        timeout.tv_usec = 0;

        int select_result = select(g_server_sock + 1, &read_fds, NULL, NULL, &timeout);

        if (select_result < 0)
        {
            if (!g_run)
                break; // Interrupted by signal
            log_message(LOG_ERROR, "Select failed");
            break;
        }

        if (select_result == 0)
        {
            // Timeout - check if we should continue running
            continue;
        }

        // Socket is ready for accept
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
#ifdef _WIN32
        SOCKET client_sock = accept(g_server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == INVALID_SOCKET)
        {
            if (!g_run)
                break; // Interrupted by signal
#else
        int client_sock = accept(g_server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0)
        {
            if (!g_run)
                break; // Interrupted by signal
#endif
            log_message(LOG_ERROR, "Accept failed");
            break;
        }

        if (!handle_request(client_sock, &config))
        {
            g_run = 0;
        }
        close(client_sock);
    } // Cleanup
    log_message(LOG_INFO, "Server shutting down...");
#ifdef _WIN32
    if (g_server_sock != INVALID_SOCKET)
    {
        close(g_server_sock);
    }
#else
    if (g_server_sock >= 0)
    {
        close(g_server_sock);
    }
#endif
    cleanup_networking();
    return 0;
}