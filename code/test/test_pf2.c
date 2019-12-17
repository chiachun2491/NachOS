#include "syscall.h"

// Test for pageFault
main()
{
        int n = 0;
        static int A[800] = {0};
        for(;n<800;n++)
        {
                if(n > 0)
                        A[n] = A[n - 1] + 100;
                PrintInt(A[n]);
        }
        // return 0;
}