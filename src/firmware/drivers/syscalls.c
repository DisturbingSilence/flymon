void _exit (int) { while(1); }
int _read (int,char*,int) { return 0; }
int _write(int,char*,int) { return 0; }
void* _sbrk(int) { return 0; }
int _close(int) {	return -1; }
int _lseek(int,int,int) { return -1; }
void _init() {}
static int errno_value = 0;
int* __errno() {return &errno_value; }
