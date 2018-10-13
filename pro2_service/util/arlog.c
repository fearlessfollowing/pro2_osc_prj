#include <android/log.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>


static pthread_rwlock_t gLock = PTHREAD_RWLOCK_INITIALIZER;
static bool gInited = false;
static bool gSendToLogcat = false;
static bool gSendToFile = false;
static int gLogFileFd = -1;
static int gLogFileInitErr = 0;
static char *gLogFilePath = NULL;
static int gOriginStdoutFd = -1;
static int gOriginStderrFd = -1;
#define LOG_BUF_SIZE 10240
#define LOG_FILE_LIMIT (10 * 1024 * 1024)



static void init_logfile_if_need()
{
    if (!gSendToFile)
        return;
    if (gLogFileFd >= 0)
        return;
    if (gLogFileInitErr != 0 && gLogFileInitErr != EPERM)
        return;

    bool truncate = false;
    int retv;
    struct stat fileStat;
    retv = stat(gLogFilePath, &fileStat);
    if (retv != -1) {
        int64_t size = fileStat.st_size;
        if (size >= LOG_FILE_LIMIT) {
            __android_log_print(ANDROID_LOG_INFO, "Insta360", "log file size %lld, will be truncated",
                (long long) size);
            truncate = true;
        }
    }

    int fd;
	
    if (!truncate)
        fd = open(gLogFilePath, O_WRONLY | O_APPEND | O_CREAT, 0666);
    else
        fd = open(gLogFilePath, O_WRONLY | O_TRUNC | O_CREAT, 0666);

    if (fd < 0) {
        gLogFileInitErr = errno;
        __android_log_print(ANDROID_LOG_ERROR, "Insta360", "arlog failed open log file %s: %s\n", gLogFilePath,
            strerror(errno));
    }
    gLogFileFd = fd;
}


void arlog_close()
{
    if (gLogFileFd >= 0) {
        close(gLogFileFd);
        gLogFileFd = -1;
    }
}

void arlog_configure(bool sendToLogcat, bool sendToFile, const char *logFilePath, bool redirectStd)
{
    pthread_rwlock_wrlock(&gLock);
    free(gLogFilePath);
    gLogFilePath = NULL;
    arlog_close();
    gLogFileInitErr = 0;

    gSendToLogcat = sendToLogcat;
    gSendToFile = sendToFile;

    if (gSendToFile) {
        gLogFilePath = strdup(logFilePath);
        init_logfile_if_need();
    }

    if (redirectStd) {
        if (gLogFileFd >= 0) {
            __android_log_print(ANDROID_LOG_INFO, "Insta360", "arlog redirect stdout and stderr\n");

            if(gOriginStdoutFd == -1)
                gOriginStdoutFd = dup(fileno(stdout));
            if(gOriginStderrFd == -1)
                gOriginStderrFd = dup(fileno(stderr));

            setbuf(stdout, NULL);
            setbuf(stderr, NULL);
            dup2(gLogFileFd, fileno(stdout));
            dup2(gLogFileFd, fileno(stderr));

        } else {
            __android_log_print(ANDROID_LOG_ERROR, "Insta360", "arlog can't redirect stdout and stderr\n");
        }
    } else {
        int stdOutFd = gOriginStdoutFd;
        int stdErrFd = gOriginStderrFd;

        if (stdOutFd != -1 && stdErrFd != -1) {
            dup2(stdOutFd, fileno(stdout));
            dup2(stdErrFd, fileno(stderr));
        }
    }
    gInited = true;
    pthread_rwlock_unlock(&gLock);
}


int __arlog_log_vprint(int prio, const char *tag, const char *fmt, va_list ap)
{
    pthread_rwlock_rdlock(&gLock);
	
    if (gLogFileInitErr == EPERM) {
        pthread_rwlock_unlock(&gLock);

        pthread_rwlock_wrlock(&gLock);
        init_logfile_if_need();
        pthread_rwlock_unlock(&gLock);
        
        pthread_rwlock_rdlock(&gLock);
    }

    char logbuf[LOG_BUF_SIZE];

    struct timeval tv;
    time_t nowtime;
    struct tm nowtm;
    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    localtime_r(&nowtime, &nowtm);
    int len = (int)strftime(logbuf, LOG_BUF_SIZE, "%m-%d %H:%M:%S", &nowtm);

    pid_t pid = getpid();
    pid_t tid = getpid();

    len += snprintf(logbuf + len, (size_t)LOG_BUF_SIZE - len, ".%03d  %ld  %ld  %s  ",
        (int)(tv.tv_usec / 1000), (long)pid, (long)tid, tag);

    int messagePos = len;
    len += vsnprintf(logbuf + len, (size_t)LOG_BUF_SIZE - len, fmt, ap);

    if (gSendToFile && gLogFileFd >= 0) {
        logbuf[len] = '\n';
        write(gLogFileFd, logbuf, (size_t)len + 1);
    }
	
    if (gSendToLogcat) {
        logbuf[len] = '\0';
        __android_log_write(prio, tag, logbuf + messagePos);
    }

    pthread_rwlock_unlock(&gLock);
    return len;
}



int __arlog_log_print(int prio, const char *tag,  const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int retv = __arlog_log_vprint(prio, tag, fmt, ap);
    va_end(ap);
    return retv;
}


