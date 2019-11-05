# HW2 - CPU Scheduler

## Work division

+ `TODO` B10615013 李聿鎧
+ `TODO` B10615024 李韋宗
+ `TODO` B10615043 何嘉峻
---
## Source Code
Github: https://github.com/chiachun2491/NachOS.git

---
## Report
### 1. Multi-Programming
1.  在`userporg/addrspace.h`新增變數，紀錄Physical page使用狀況
```cpp
【userporg/addrspace.h】

class AddrSpace {
  public:
    // record used state of the main memory page
    static bool usedPhyPage[NumPhysPages];	
    /* 略 */
};
```
2. 在`usrprog/addrspace.cc`將Physical page初始化為0
```cpp
【usrprog/addrspace.cc】

#include /* 略 */
// initial usedPhyPage to zero
bool AddrSpace::usedPhyPage[NumPhysPages] = {0};
```

3. 使用完Page後，必須要釋放資源
```cpp
【usrprog/addrspace.cc】

//---------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Deallocate an address space and release the phyPage.
//---------------------------------------------------------------------
AddrSpace::~AddrSpace()
{
   for(int i = 0; i < numPages; i++)
        AddrSpace::usedPhyPage[pageTable[i].physicalPage] = false;
   delete pageTable;
}
```

4. 程序運行時，我們就依順序找一個可用記憶體位置給它。
並要更改讀取Code以及Data的位置
```cpp
【usrprog/addrspace.cc】

bool 
AddrSpace::Load(char *fileName) 
{
    /* 略 */
   // create pageTable and record usedPhyPage
    pageTable = new TranslationEntry[numPages];
    for(unsigned int i = 0, j = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;
        while(j < NumPhysPages && AddrSpace::usedPhyPage[j] == true)
            j++;
        AddrSpace::usedPhyPage[j] = true;
        pageTable[i].physicalPage = j;
        pageTable[i].valid = true;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false;
    }
    /* 略 */
    if (noffH.code.size > 0) {
        DEBUG(dbgAddr, "Initializing code segment.");
	DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);
	// read memory address in pageTable	
        executable->ReadAt(
		&(kernel->machine->mainMemory[pageTable[noffH.code.virtualAddr/PageSize].physicalPage * PageSize + (noffH.code.virtualAddr%PageSize)]), 
			noffH.code.size, noffH.code.inFileAddr);
    }
	if (noffH.initData.size > 0) {
        DEBUG(dbgAddr, "Initializing data segment.");
	DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);
	// read memory address in pageTable 
        executable->ReadAt(
		&(kernel->machine->mainMemory[pageTable[noffH.initData.virtualAddr/PageSize].physicalPage * PageSize + (noffH.code.virtualAddr%PageSize)]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
    /* 略 */
}
```
#### Screenshot
如下圖所示，`test1`及`test2`同時運行不會互相影響。
![MP](https://i.imgur.com/1AejwVP.png)

---
### 2. CPU Scheduler
1. 幫每種排程建立不同的SortedList，並給予不同的Compare Function
```cpp
【threads/addrspace.cc】

Scheduler::Scheduler()
{
    Scheduler(RR);
}

Scheduler::Scheduler(SchedulerType type)
{
    schedulerType = type;
    switch(schedulerType) {
    case RR:
        readyList = new List<Thread *>;
        break;
    case SJF:
        readyList = new SortedList<Thread *>(SJFCompare);
        break;
    case Priority:
        readyList = new SortedList<Thread *>(PriorityCompare);
        break;
    case FIFO:
        readyList = new SortedList<Thread *>(FIFOCompare);
        break;
    case SRTF:
        readyList = new SortedList<Thread *>(SRTFCompare);
        break;
    }
    toBeDestroyed = NULL;
}

```


#### 先進先出排程 (First-Come-First-Service, FCFS)
```cpp
【threads/addrspace.cc】

int FIFOCompare(Thread *a, Thread *b) {
    return 1;
}
```
![FCFS](https://i.imgur.com/yc3Dz10.png)

#### 最短工作優先排程 (Shortest-Job-First, SJF)
```cpp
【threads/addrspace.cc】

int SJFCompare(Thread *a, Thread *b) {
    if(a->getArrivalTime() == b->getArrivalTime())
    {
        if (a->getBurstTime() == b->getBurstTime())
            return 0;
        return a->getBurstTime() > b->getBurstTime() ? 1 : -1;
    }
    return a->getArrivalTime() > b->getArrivalTime() ? 1 : -1;
}
```
/* TODO screenshot */


#### 最短剩餘時間排程 (shortest remaining time first ,SRTF)
```cpp
【threads/addrspace.cc】

int SRTFCompare(Thread *a, Thread *b) {
    /* TODO */
}
```
/* TODO screenshot */

