#include "syscall.h"
#define SIZE 1000

int main()
{
	int i;
	static int arr[SIZE];

    PrintInt(0);
    for( i = 1 ; i< 1000 ; ++i )
    {
	arr[i] = arr[i] + i + 87;
    }

    PrintInt(arr[SIZE - 1]);

    return 0;
}
