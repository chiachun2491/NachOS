#include "syscall.h"
#define SIZE 1000

int main()
{
    int i, k;
	unsigned int j;


	static int arr[1000];
//	int brr[size];
   	

    PrintInt(0);
    for( i=1 ; i< 1000 ; ++i )
    {
        //PrintInt(i);
		arr[i] = arr[i] + i + 87;
        
    }

    // PrintInt(arr[size-1]);

    return 0;
}
