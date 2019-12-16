#include "syscall.h"

int main()
{
    int i;
    //PrintInt(0);
    int size = 1000;
    //PrintInt(0);
    int arr[size];
    //PrintInt(0);
    //arr[0] = 0;
    PrintInt(0);
    for( i=1 ; i<size ; ++i )
    {
        arr[i] = arr[i-1] + 1;
        // PrintInt(i);
    }

    // PrintInt(arr[size-1]);

    return 0;
}
