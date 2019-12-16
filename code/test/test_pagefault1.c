#include "syscall.h"

int main()
{
    int i;
    int size = 100;
    int arr[size];
    arr[0] = 0;
    PrintInt(1);
    for( i=1 ; i<size ; ++i )
    {
        arr[i] = arr[i-1] + 1;
        // PrintInt(i);
    }

    // PrintInt(arr[size-1]);

    return 0;
}
