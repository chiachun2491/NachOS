#include "syscall.h"
#define SECOND 300000000
main()
{
	int n;
	for (n = 1; n < 5; ++n)
    {
        Sleep(1 * SECOND);
		Example(n);
    }
}
