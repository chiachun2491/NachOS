// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------

void ExceptionHandler(ExceptionType which)
{
	int type = kernel->machine->ReadRegister(2);
	int val;
	int target; 	// target page to swap out
    unsigned int vpn, offset;
	int j;

	switch (which)
	{
	case SyscallException:
		switch (type)
		{
		case SC_Halt:
			DEBUG(dbgAddr, "Shutdown, initiated by user program.\n");
			kernel->interrupt->Halt();
			break;
		case SC_PrintInt:
			val = kernel->machine->ReadRegister(4);
			cout << "Print integer:" << val << endl;
			return;
		case SC_Sleep:
			val = kernel->machine->ReadRegister(4);
			cout << "Sleep time:" << val << "ms" << endl;
			kernel->alarm->WaitUntil(val);
			return;
		/*		case SC_Exec:
			DEBUG(dbgAddr, "Exec\n");
			val = kernel->machine->ReadRegister(4);
			kernel->StringCopy(tmpStr, retVal, 1024);
			cout << "Exec: " << val << endl;
			val = kernel->Exec(val);
			kernel->machine->WriteRegister(2, val);
			return;
*/
		case SC_Exit:
			DEBUG(dbgAddr, "Program exit\n");
			val = kernel->machine->ReadRegister(4);
			cout << "return value:" << val << endl;
			kernel->currentThread->Finish();
			break;
		default:
			cerr << "Unexpected system call " << type << "\n";
			break;
		}
		break;

	case PageFaultException:
		kernel->stats->numPageFaults++;

		j=0;
		while (kernel->machine->usedPhyPage[j] == true && j < NumPhysPages)
		{
			j++;
		}

		if (j < NumPhysPages)
		{
			char *buf = new char[PageSize]; // Save temp Page
			kernel->machine->usedPhyPage[j] = true;
			kernel->machine->mainTable[j] = &kernel->machine->pageTable[vpn];

			kernel->machine->pageTable[vpn].physicalPage = j;
			kernel->machine->pageTable[vpn].valid = true;
			kernel->machine->pageTable[vpn].count++;

			kernel->virtualMem_disk->ReadSector(kernel->machine->pageTable[vpn].virtualPage, buf);
			bcopy(buf, &kernel->machine->mainMemory[j * PageSize], PageSize);
		}
		else
		{
			char *buf_1 = new char[PageSize];
			char *buf_2 = new char[PageSize];

			if (kernel->machine->replacementType == Replace_FIFO)
			{
				target = kernel->machine->fifo % NumPhysPages;
			}
			else if (kernel->machine->replacementType == Replace_LRU)
			{
				int min = kernel->machine->pageTable[0].count;
				target = 0;
				for (int i = 0; i < NumPhysPages; i++)
				{
					if (min > kernel->machine->pageTable[i].count)
					{
						min = kernel->machine->pageTable[i].count;
						target = i;
					}
				}
				kernel->machine->pageTable[target].count++;
			}
			else
			{
				target = kernel->machine->fifo % NumPhysPages;
			}

			cout << "Number = " << target << "page swap out." << endl;

			bcopy(&kernel->machine->mainMemory[target * PageSize], buf_1, PageSize);
			kernel->virtualMem_disk->ReadSector(kernel->machine->pageTable[vpn].virtualPage, buf_2);
			bcopy(buf_2, &kernel->machine->mainMemory[target * PageSize], PageSize);
			kernel->virtualMem_disk->WriteSector(kernel->machine->pageTable[vpn].virtualPage, buf_1);

			kernel->machine->mainTable[target]->virtualPage = kernel->machine->pageTable[vpn].virtualPage;
			kernel->machine->mainTable[target]->valid = false;

			// save
			kernel->machine->pageTable[vpn].valid = true;
			kernel->machine->pageTable[vpn].physicalPage = target;
			kernel->machine->mainTable[target] = &kernel->machine->pageTable[vpn];
			kernel->machine->fifo++;

			cout << "Page replacement finished" << endl;
			break;
		}
		default:
			cerr << "Unexpected user mode exception" << which << "\n";
			break;
		}
		ASSERTNOTREACHED();
	}
