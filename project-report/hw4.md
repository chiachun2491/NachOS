# HW4 - Virtual memory

## Work division

+ `寫報告 & 撰寫測試程式` B10615013 李聿鎧
+ `研究如何 pagefault & debug` B10615024 李韋宗
+ `編寫程式碼 & debug` B10615043 何嘉峻
---
## Source C
Github: https://github.com/chiachun2491/NachOS.git

---
## Report
### Virtual memory & Page fault

+ **`code/machine/machine.h`**

這裡加入 Replacement 演算法的種類，並在 `Machine` 中加入變數成員 `replacementType` 儲存。
```cpp
/*...*/
const unsigned int NumPhysPages = 64;	// 32 is too small

enum ReplacementType { 
  	Replace_FIFO,
	Replace_LRU
};
/*...*/

class Machine {
  public:
/*...*/
	ReplacementType replacementType;
/*...*/

};
```

+ **`code/userprog/addrspace.h`**

這裡宣告哪些 page 被使用的狀態陣列，並且用 `mainTable[NumPhys Pages]` 紀錄 `pageTable` 中的 row 確保接下來能夠更改對應的參數，`pt_is_load` 紀錄是否建表。
```cpp
class AddrSpace {
  public:
    /*...*/
    static bool usedPhyPage[NumPhysPages];	// record used state of the main memory page
    static bool usedVirPage[NumPhysPages];  // record used state of the virtual memory page
    static TranslationEntry *mainTable[NumPhysPages]; 
    static int fifo;  // for fifo

  private:
    /* ... */
    bool pt_is_load;
};
```

+ **`code/userprog/addrspace.cc`**

先初始化 static 變數成員們。
```cpp
bool AddrSpace::usedPhyPage[NumPhysPages] = {0};
bool AddrSpace::usedVirPage[NumPhysPages] = {0};  // record used state of the virtual memory page
TranslationEntry *AddrSpace::mainTable[NumPhysPages] = {NULL}; 
int AddrSpace::fifo = 0;   
```

Deconstructor 將紀錄 physical memory 與 virtual memory 的陣列重設。
```cpp
//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Deallocate an address space and release the phyPage.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   for(int i = 0; i < numPages; i++) {
        AddrSpace::usedPhyPage[pageTable[i].physicalPage] = false;
        AddrSpace::usedVirPage[pageTable[i].virtualPage] = false;
    }
   delete pageTable;
}

```
`if (noffH.code.size > 0)` 中利用迴圈逐次要求 physical memory 中的 page，若 page 皆使用中則執行內部的 else，剩餘的將向 virtual memory 索取；索取記憶體時，皆需要更改 `pageTable` 中的各種參數。
```cpp
//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool 
AddrSpace::Load(char *fileName) 
{
    /* ... */
    
// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
//	cout << "number of pages of " << fileName<< " is "<<numPages<<endl;

   // create pageTable
    pageTable = new TranslationEntry[numPages];

    size = numPages * PageSize;

    // ASSERT(numPages <= NumPhysPages);		// check we're not trying
	// 					// to run anything too big --
	// 					// at least until we have
	// 					// virtual memory

    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);

// then, copy in the code and data segments into memory
	if (noffH.code.size > 0) {
        DEBUG(dbgAddr, "Initializing code segment.");
	    DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);

        for(unsigned int i = 0, j = 0, k = 0 ; i < numPages; i++) {
            pageTable[i].virtualPage = i;
            while(j < NumPhysPages && AddrSpace::usedPhyPage[j] == true)
                j++;

            // if memory is enough -> main memory
            if (j < NumPhysPages) {
                AddrSpace::usedPhyPage[j] = true;
                AddrSpace::mainTable[j] = &pageTable[i];
                pageTable[i].physicalPage = j;
                pageTable[i].valid = true;
                pageTable[i].use = false;
                pageTable[i].dirty = false;
                pageTable[i].readOnly = false;

                // read memory address in pageTable	
                executable->ReadAt(&(kernel->machine->mainMemory[j * PageSize]), 
                    PageSize, noffH.code.inFileAddr + (i * PageSize));
            }
            // else -> virtual memory (Disk)
            else {
                char *buf = new char[PageSize];
                while(AddrSpace::usedVirPage[k] != false)
                    k++;
                
                pageTable[i].virtualPage = k;
                pageTable[i].valid = false;
                pageTable[i].use = false;
                pageTable[i].dirty = false;
                pageTable[i].readOnly = false;
                
                // read memory address in pageTable	
                executable->ReadAt(buf, PageSize, noffH.code.inFileAddr + (i * PageSize));
                kernel->virtualMem_disk->WriteSector(k, buf);
            }
        }
    }
	if (noffH.initData.size > 0) {
        DEBUG(dbgAddr, "Initializing data segment.");
	    DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);
	    // read memory address in pageTable 
        executable->ReadAt(&(kernel->machine->mainMemory[noffH.initData.virtualAddr]),
            noffH.initData.size, noffH.initData.inFileAddr);
    }
    
    /* ... */
}
```
在執行程式前，確保讀檔成功 `pt_is_load == true`。
```cpp
//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program.  Load the executable into memory, then
//	(for now) use our own thread to run it.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

void 
AddrSpace::Execute(char *fileName) 
{
    pt_is_load = false;

    if (!Load(fileName)) {
	cout << "inside !Load(FileName)" << endl;
	return;				// executable not found
    }

    //kernel->currentThread->space = this;
    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register

    pt_is_load = true;
    
    kernel->machine->Run();		// jump to the user progam

    ASSERTNOTREACHED();			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}
```
有確定讀檔成功才能執行 `SaveState()`。
```cpp
//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
    if (pt_is_load) {
        pageTable=kernel->machine->pageTable;
        numPages=kernel->machine->pageTableSize;
    }
}
```

+ **`code/userprog/userkernel.h`**

宣告名為 `virtualMem_disk` 的 SynchDisk，當主記憶體不夠用時，就會存到 SynchDisk。
```cpp
class UserProgKernel : public ThreadedKernel {
  public:
 /*...*/
    // Use for virtual memory
    SynchDisk *virtualMem_disk;
/*...*/
 private:
    bool debugUserProg;		// single step user program
    ReplacementType replacementType; // record replacement algortihm
/*...*/
}
```

+ **`code/userprog/userkernel.cc`**

解析輸入指令，確定要執行哪種演算法 (FIFO&LRU)。
如果輸入 `-RP` 後卻沒有指定要哪種演算法，會預設執行 FIFO (但會跳出警告)。
```cpp
UserProgKernel::UserProgKernel(int argc, char **argv)
	: ThreadedKernel(argc, argv)
{
/*...*/
    else if (strcmp(argv[i], "-RP") == 0)
    {

        if (strcmp(argv[i + 1], "FIFO") == 0)
        {
            replacementType = Replace_FIFO;
            cout << "Replacement Algorithm set FIFO." << endl;
        }

        else if (strcmp(argv[i + 1], "LRU") == 0)
        {
            replacementType = Replace_LRU;
            cout << "Replacement Algorithm set LRU." << endl;
        }
        else
        {
            cout << "Wrong Replacement Algorithm Setting, now using FIFO." << endl;
        }
    }
}
```
接到設定的演算法後，在執行 `Initialize()` 時，會設定到 `machine->replacementType`，並且初始化 Disk。
```cpp
void UserProgKernel::Initialize()
{
	ThreadedKernel::Initialize(); // init multithreading

	machine = new Machine(debugUserProg);
	machine->replacementType = this->replacementType;
	fileSystem = new FileSystem();
    
    // Virtual Memory
	virtualMem_disk = new SynchDisk("New Disk");
    /*...*/
}
```


### Page Replacement Algorithm

#### 如何切換演算法
+ **FIFO (Default) :** `./nachos -RP FIFO`
+ **LRU :** `./nachos -RP LRU`
+ 範例: 執行很多一般程式(ex. `test1` & `test2`)，再搭配這次的測試程式 `test_pagefault`
```
$ ./nachos -RP FIFO -e ../test/test1 -e ../test/test_2 -e ../test/test2 -e ../test/test_1 -e ../test/test_2 -e ../test/test_pagefault 
$ ./nachos -RP LRU -e ../test/test1 -e ../test/test_2 -e ../test/test2 -e ../test/test_1 -e ../test/test_2 -e ../test/test_pagefault 
```

#### PageFault Handle & FIFO / LRU 實作
+ **`code/machine/translate.cc`**

`target` 存放準備更改的目標 index；將在原先拋出 PageFaultException註解，直接在該區段中實作，其中if(j < NumPhysPages)代表 page 只存在 virtual memory 中，所以只需swap in；else代表 physical memory中已滿，所以要swap in 及 swap up，其中 `buf_1`, `buf_2`用來暫存要交換的部分，另外因為牽涉到 replacement 演算法所以要對`target`進行不同的運算
```cpp
ExceptionType
Machine::Translate(int virtAddr, int* physAddr, int size, bool writing)
{
    /* ... */
	int target; 	// target page to swap out

    /* ... */
    
    if (tlb == NULL) {		// => page table => vpn is index into table
	if (vpn >= pageTableSize) {
	    DEBUG(dbgAddr, "Illegal virtual page # " << virtAddr);
	    return AddressErrorException;
	} 
	else if (!pageTable[vpn].valid) {
	    DEBUG(dbgAddr, "Invalid virtual page # " << virtAddr);
	    // return PageFaultException;
		kernel->stats->numPageFaults++;

		int j = 0;
		while(AddrSpace::usedPhyPage[j] == true && j < NumPhysPages) {
			j++;
		}
		
		if (j < NumPhysPages) {
			char *buf = new char[PageSize]; // Save temp Page
			AddrSpace::usedPhyPage[j] = true;
			AddrSpace::mainTable[j] = &pageTable[vpn];

			pageTable[vpn].physicalPage = j;
			pageTable[vpn].valid = true;
			pageTable[vpn].count++;

			kernel->virtualMem_disk->ReadSector(pageTable[vpn].virtualPage, buf);
			bcopy(buf, &mainMemory[j * PageSize], PageSize);
		}
		else {
			char *buf_1 = new char[PageSize];
			char *buf_2 = new char[PageSize];

			// FIFO
			if (kernel->machine->replacementType == Replace_FIFO) {
				target = AddrSpace::fifo % NumPhysPages;
			}
			// LRU
			else if (kernel->machine->replacementType == Replace_LRU) {
				int min = pageTable[0].count;
				target = 0;
				for (int i = 0; i < NumPhysPages; i++) {
					if (min > pageTable[i].count) {
						min = pageTable[i].count;
						target = i;
					}
				}
				pageTable[target].count++;
			}
			// avoid didn't set FIFO/LRU -> default FIFO
			else {
				target = AddrSpace::fifo % NumPhysPages;
			}

			cout << "Number = " << target << "page swap out." << endl;

			bcopy(&mainMemory[target * PageSize], buf_1, PageSize);
			kernel->virtualMem_disk->ReadSector(pageTable[vpn].virtualPage, buf_2);
			bcopy(buf_2, &mainMemory[target * PageSize], PageSize);
			kernel->virtualMem_disk->WriteSector(pageTable[vpn].virtualPage, buf_1);
			
			AddrSpace::mainTable[target]->virtualPage = pageTable[vpn].virtualPage;
			AddrSpace::mainTable[target]->valid = false;
			
			
			// save
			pageTable[vpn].valid = true;
			pageTable[vpn].physicalPage = target;
			AddrSpace::mainTable[target] = &pageTable[vpn];
			AddrSpace::fifo++;

			cout << "Page replacement finished" << endl;
		}
	}
	entry = &pageTable[vpn];
    } 
    /* ... */
}
```

### Screenshot

#### 測試程式
> 宣告足夠大的 int array，並對array進行一些操作並輸出。

```cpp
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
```

#### Before
> 因為 main memory 不夠大而造成 segment fault。

![](https://i.imgur.com/KRUW9io.png)

#### After-FIFO
![](https://i.imgur.com/UeF8HkG.png)

#### After-LRU
![](https://i.imgur.com/dzBpT8I.png)

---
###### tags: `NachOS`
