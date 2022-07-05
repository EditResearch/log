/**
** @file log.c
** @brief Logging library
** @author Petr Horáček
*/

#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdarg.h>


struct Log
{
    pthread_mutex_t lock;
    bool quiet;

    size_t f_list_length;
    FILE * f_list[];
};


typedef enum
{
    Debug
    , Warning
    , Error
    , LogLevelNumber
}LogLevel;


const char * log_level_label[LogLevelNumber] =
{
    "Debug"
    , "Warning"
    , "Error"
};


static char *
_log_current_time();


static void
_log_process(
    Log * log
    , const char * log_level
    , const char * format
    , va_list * p);


Log *
log_new(
    size_t f_list_length
    , FILE * f_list[f_list_length])
{
    size_t f_list_byte_size = (sizeof(FILE*)*f_list_length);

    Log * log = malloc(sizeof(Log) + f_list_byte_size);
    
    if(log != NULL)
    {
        memcpy(log->f_list, f_list, f_list_byte_size);
        log->quiet = false;
        log->f_list_length = f_list_length;

        if (pthread_mutex_init(&log->lock, NULL) != 0)
        {
            free(log);
            return NULL;
        }

        log_debug(log, "Log init success");
    }

    return log;
}


void
log_debug(
    Log * log
    , char * format
    , ...)
{
    if(log->quiet == false)
    {
        va_list p;
        va_start(p, format);

        _log_process(log, log_level_label[Debug], format, &p);

        va_end(p);
    }
}


void
log_warning(
    Log * log
    , char * format
    , ...)
{
    if(log->quiet == false)
    {
        va_list p;
        va_start(p, format);

        _log_process(log, log_level_label[Warning], format, &p);

        va_end(p);
    }
}


void
log_error(
    Log * log
    , char * format
    , ...)
{
    {
        va_list p;
        va_start(p, format);

        _log_process(log, log_level_label[Error], format, &p);

        va_end(p);
    }
}



void 
log_set_quiet(
    Log * log
    , bool enable)
{
    log->quiet = enable;
}


bool
log_is_quiet(Log * log)
{
    return log->quiet;
}


void
log_delete(Log * log)
{
    if(log != NULL)
    {
        pthread_mutex_destroy(&log->lock);
        free(log);
    }
}


static void
_log_process(
    Log * log
    , const char * log_level
    , const char * format
    , va_list * p)
{
    pthread_mutex_lock(&log->lock);

    for(size_t i = 0; i < log->f_list_length; i++)
    {
        va_list iter;
        va_copy(iter, *p);

        fprintf(log->f_list[i], "%s [%s] - ", _log_current_time(), log_level);
        vfprintf(log->f_list[i], format, iter);
        fprintf(log->f_list[i], "\n");
    }

    pthread_mutex_unlock(&log->lock);
}


static char *
_log_current_time()
{
    static char str_time[128] = {0};

    time_t rawtime;
    struct tm * timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    sprintf(
        str_time
        , "%d.%d.%d - %d:%d:%d"
        , timeinfo->tm_mday
        , timeinfo->tm_mon + 1
        , timeinfo->tm_year + 1900
        , timeinfo->tm_hour
        , timeinfo->tm_min
        , timeinfo->tm_sec);

    return str_time;
}