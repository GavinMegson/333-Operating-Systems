#ifdef CS333_P2
#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
	uint uid, gid, ppid;

        printf(1,"ID tests: UID, GID, PPID\n");

	uid = getuid();
	gid = getgid();
	ppid = getppid();

	printf(1,"Current UID is: %d\n",uid);
	printf(1,"Current GID is: %d\n",gid);
	printf(1,"Current PPID is: %d\n",ppid);

	// Test UID, GID numbers
	uint tests[5] = {0,1,100,32767,32768}

	for (int i = 0; i < 5; i++) {
		printf(2,"Setting UID to %d.\n",tests[i]);
		if ( setuid(tests[i]) < 0 ) {
			printf(2, "Error: UID could not be set.\n");
		} else {
			uid = getuid();
			printf(2,"Current UID is: %d\n",uid);
		}

		printf(2,"Setting GID to %d.\n",tests[i]);
		if ( setgid(tests[i]) < 0 ) {
			printf(2, "Error: GID could not be set.\n");
		} else {
			gid = getgid();
			printf(2,"Current GID is: %d\n",gid);
		}

	printf(1,"Finished.\n");

	return 0;
}
/*
int
testshell(void) {
	
*/
#endif
