#ifdef CS333_P2
#include "uproc.h"
#endif

struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int halt(void);
#ifdef CS333_P1
int date(struct rtcdate*);
#endif
#ifdef CS333_P2
int setuid(uint);
int setgid(uint);
int getuid(void);
int getgid(void);
int getppid(void);
int getprocs(uint max, struct uproc *table);
#endif
#ifdef CS333_P3P4
int setpriority(int pid, int priority);
int p4test(void);
#endif
#ifdef CS333_P5
int chown(char*, int);
int chgrp(char*, int);
int chmod(char*, int);
#endif

// ulib.c
int stat(char*, struct stat*);
char* strcpy(char*, char*);
void *memmove(void*, void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, char*, ...);
char* gets(char*, int max);
uint strlen(char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
#ifdef CS333_P5
int atoo(const char*);
#endif
