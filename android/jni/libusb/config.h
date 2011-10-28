#define ENABLE_LOGGING
#define OS_LINUX
#define API_EXPORTED

#ifndef TIMESPEC_TO_TIMEVAL
#define TIMESPEC_TO_TIMEVAL(tv, ts)                                     \
        do {                                                            \
                (tv)->tv_sec = (ts)->tv_sec;                            \
                (tv)->tv_usec = (ts)->tv_nsec / 1000;                   \
        } while (0)
#endif 
