#include "syscall.h"

int main()
{
    int i;
    int arr[20];
    PrintInt(1);
    for( i=1 ; i<20 ; ++i )
    {
        arr[i] = arr[i-1] + 1;
        PrintInt(arr[i]);
    }

    // PrintInt(arr[size-1]);

    return 0;
}
