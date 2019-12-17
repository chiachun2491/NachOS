#include "syscall.h"

int main()
{
    int i, k;
	unsigned int j;

    int size = 200;

	static int arr[1000];
//	int brr[size];
   	

    PrintInt(0);
    for( i=1 ; i<size ; ++i )
    {
        
		arr[i] = arr[i] + i + 87;
        
    }

    PrintInt(arr[size-1]);

    return 0;
}