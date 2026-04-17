
#ifndef MY_LOGGER_H_
#define MY_LOGGER_H_

#include <stdio.h>
#include <time.h>
#include <sys/time.h>  // For gettimeofday() to get microsecond precision
#include <stdarg.h>    // Required for va_list
#include <string.h>
#include <stdlib.h>

/* Keep one persistent FILE* per log filename so that fopen/fclose are not
 * called on every log_message() invocation.  The MAC scheduler runs on a
 * real-time thread with sub-millisecond deadlines; repeated fopen/fclose
 * (~3 blocking syscalls + filesystem metadata each) blow those deadlines and
 * cause UE connection failures.
 * Up to LOG_MSG_MAX_FILES distinct filenames are supported. */
#define LOG_MSG_MAX_FILES 18

typedef struct {
    const char *name;
    FILE       *fp;
} log_file_entry_t;

static inline FILE *log_get_file(const char *filename)
{
    static log_file_entry_t log_files[LOG_MSG_MAX_FILES];
    static int              log_files_count = 0;

    /* Fast path: match by pointer (callers pass the same global char* every time) */
    for (int i = 0; i < log_files_count; i++) {
        if (log_files[i].name == filename)
            return log_files[i].fp;
    }
    /* Slow path: also try string comparison in case caller uses a different pointer */
    for (int i = 0; i < log_files_count; i++) {
        if (strcmp(log_files[i].name, filename) == 0)
            return log_files[i].fp;
    }
    /* New file: open once and cache */
    if (log_files_count >= LOG_MSG_MAX_FILES)
        return NULL; /* table full */
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Error opening log file");
        return NULL;
    }
    log_files[log_files_count].name = filename;
    log_files[log_files_count].fp   = fp;
    log_files_count++;
    return fp;
}

static inline void log_message(const char *filename, const char *format, ...) {
    FILE *file = log_get_file(filename);
    if (!file)
        return;

    // Get current time with microsecond precision
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm *local_time = localtime(&tv.tv_sec);

    char timestamp[100];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time);

    char timestamp_with_microsec[120];
    snprintf(timestamp_with_microsec, sizeof(timestamp_with_microsec),
             "%s.%06ld", timestamp, tv.tv_usec);

    va_list args;
    va_start(args, format);
    char formatted_message[1024];
    vsnprintf(formatted_message, sizeof(formatted_message), format, args);
    va_end(args);

    fprintf(file, "[%s] %s\n", timestamp_with_microsec, formatted_message);
    /* No fflush/fclose here — the file stays open. Data is flushed when the
     * process exits normally or you call fflush() explicitly at shutdown. */
}

#endif /* MY_LOGGER_H_ */