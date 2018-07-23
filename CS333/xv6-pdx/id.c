#ifdef CS333_P2
#include "types.h"

/*
 * Functions related to process, user, and
 * group id.
 */


// Return user id of current process
uint
getuid(void){
	return p->uid;
}

// return group id of current process
uint
getgid(void){
	return p->gid;
}

// return process id of parent process
uint
getppid(void){
	return p->parent->pid;
}

// set uid - returns 0 if id out of range
int
setuid(uint id){
	if (id < 32768) {
		p->uid = id;
		return 1;
	} else {
		return 0;
}

// set gid - returns 0 if out of range
int
setgid(uint id){
	if (id < 32768) {
		p->gid = id;
		return 1;
	} else {
		return 0;
}
#endif
