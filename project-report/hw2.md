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
### 2. CPU Scheduler.
1. 從command中記錄各種排程
```cpp
ThreadedKernel::ThreadedKernel(int argc, char **argv) {
        /* in for lopp */
        else if(strcmp(argv[i], "-RR") == 0) {
            type = RR;
        } else if (strcmp(argv[i], "-FCFS") == 0) {
            type = FIFO;
        } else if (strcmp(argv[i], "-PRIORITY") == 0) {
            type = Priority;
        } else if (strcmp(argv[i], "-SJF") == 0) {
            type = SJF;
        } else if (strcmp(argv[i], "-SRTF") == 0) {
            type = SRTF;
        }
}
```

2. 幫每種排程建立不同的SortedList，並給予不同的Compare Function
```cpp
【threads/addrspace.cc】

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
【threads/scheduler.cc】

int FIFOCompare(Thread *a, Thread *b) {
    return 1;
}
```

#### Screenshot
如下圖所示，`test1`全部執行完成後`test2`才執行。
![FCFS](https://i.imgur.com/yc3Dz10.png)

#### 最短工作優先排程 (Shortest-Job-First, SJF)
我們優先考慮`burstTime`，接著才考慮`arrivalTime`。
```cpp
【threads/scheduler.cc】

int SJFCompare(Thread *a, Thread *b) {
    if(a->getBurstTime() == b->getBurstTime())
    {
        if (a->getArrivalTime() == b->getArrivalTime())
            return 0;
        return a->getArrivalTime() > b->getArrivalTime() ? 1 : -1;
    }
    return a->getBurstTime() > b->getBurstTime() ? 1 : -1;
}
```
模擬排成如下
```cpp
【threads/thread.cc】

void Thread::SelfTest() {
    const int number 	 = 5;
    char *name[number] 	 = {"A", "B", "C", "D", "E"};
    int burst[number] 	 = {1, 2, 3, 2, 1};
    int priority[number] = {4, 5, 3, 1, 2};
    int arrival[number] = {0, 1, 2, 3, 4};
    /* 略 */
}
```
#### Screenshot
如下圖所示，5個模擬thread正確執行。
![SJF](https://imgur.com/WPJovH2.png)


#### 最短剩餘時間排程 (shortest remaining time first ,SRTF)
因為演算法相似，我們使用SJF的compare function。
```cpp
【threads/scheduler.cc】

int SRTFCompare(Thread *a, Thread *b) {
    return SJFCompare(a, b);
}
```

在`CallBack`中新增SRTF為可以被preempt的一種排成。

```cpp
【threads/alarm.cc】
void Alarm::CallBack() {
    /* 略 */
    if(kernel->scheduler->getSchedulerType() == RR ||
            kernel->scheduler->getSchedulerType() == Priority ||
            kernel->scheduler->getSchedulerType() == SRTF) {
		interrupt->YieldOnReturn();
	}
}
```

模擬排成如下
```cpp
void Thread::SelfTest() {
    const int number 	 = 3;
    char *name[number] 	 = {"A", "B", "C"};
    int burst[number] 	 = {1, 5, 1};
    int priority[number] = {4, 5, 3};
    int arrival[number] = {0, 1, 3};
    /* 略 */
}
```

#### Screenshot
如下圖所示，3個模擬thread正確執行。
![SRTF](https://imgur.com/O5iNy3W.png)

---
### 3. SRTF與SJF的詳細實作方法
1. 在thread.h中加入模擬時間
```cpp
【threads/thread.h】

static int currentTime;
```

2. 修改SRTF排成執行`Yield`而非`OneTick`，為了每次都檢查是否有優先權更高的thread可以執行
```cpp
【threads/thread.cc】

int Thread::currentTime = 0;

static void SimpleThread() {
    while(thread->getBurstTime() > 0) {
    /* 略 */
    Thread::currentTime++;
        if (kernel->scheduler->getSchedulerType() == SRTF) {
            {
                kernel->currentThread->Yield();
            }
            else
            {
                kernel->interrupt->OneTick();
            }
        }
    }
}
```

3. 修改`FindNextToRun`
```cpp
【threads/scheduler.cc】

Thread * Scheduler::FindNextToRun (bool advance)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (readyList->IsEmpty()) 
    {
	    return NULL;
    } 
```

* 由於SRTF與SJF我們都有實作考慮`arrivalTime`，故需要與其他排成分開
```cpp
	if (kernel->scheduler->getSchedulerType() == SRTF || kernel->scheduler->getSchedulerType() == SJF)
    {
        ListIterator<Thread *> *iter = new ListIterator<Thread *>(readyList);
        Thread * smallest = iter->Item();

        DEBUG(dbgThread, "SRTF Estimate FindNextToRun: " << smallest->getName());
```

* 在`readyList`中尋找已經到達且優先權最高的thread 

`/* todo 我不確定底下的if是要怎麼解釋 */`
```cpp
        while (iter->Item()->getArrivalTime() > Thread::currentTime)
        {
            DEBUG(dbgThread, "Estimated Error: ");
            DEBUG(dbgThread, "\tCompare Arrival Time Failed: " 
                            << smallest->getName() << " :" << iter->Item()->getArrivalTime() 
                            << " vs Current Time:" << Thread::currentTime);

            iter->Next();

            if (iter->IsDone()) break;
            if (iter->Item()->getArrivalTime() < smallest->getArrivalTime()) 
            {
                smallest = iter->Item();
                DEBUG(dbgThread, "\tAgain Estimate FindNextToRun: " << smallest->getName());
            }
        }
```

* 如果找到優先權更高的Thread就將他回傳並從`readyList`中移除
```cpp
        if (!iter->IsDone())
        {
            Thread *t = iter->Item(); // Backup
            readyList->Remove(iter->Item());
            DEBUG(dbgThread, "Remove from readyList and return: " << t->getName());
            return t;
        }
```

* 若沒有找到且不需要`advance`才回傳NULL；需要`advance`時，將`currentTime`設定為最近要到達的Thread的`arrivalTime`，並且將其回傳並從`readyList`中移除
```cpp
        else
        {
            if (advance)
            {
                DEBUG(dbgThread, "Advance ON: Fast Forward Time to " << smallest->getArrivalTime());
                // cout << "-----------------Time:" << Thread::currentTime << "----------------"<< endl;
                Thread::currentTime = smallest->getArrivalTime();
                readyList->Remove(smallest);
                DEBUG(dbgThread, "Remove from readyList and return: " << smallest->getName());
                return smallest;
            }
            else
            {
                DEBUG(dbgThread, "Not Found any Thread and return NULL.");
                return NULL;
            }
        }
    }
```

* 非SJF與SRTF的其他排成方法沿用原本的回傳方式
```cpp
    else
    {
    	return readyList->RemoveFront();
    }
}
```

4. 修改`Yield`

```cpp
void
Thread::Yield ()
{
    Thread *nextThread;
    IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);
    
    ASSERT(this == kernel->currentThread);
    
    DEBUG(dbgThread, "Yielding thread: " << name);
```

* Thread只是暫時停下，故`advance`須傳入false
```cpp
    nextThread = kernel->scheduler->FindNextToRun(false);
```

* 只有SRTF會須要使用`Yield`判斷preempt所以只需要判別他
* 若目前Thread的`burstTime`比`FindNextToRun`中找到的還小，則不發生context switch
```cpp
    if (nextThread != NULL)
    {
        if (kernel->scheduler->getSchedulerType() == SRTF)
        {
            if (this->getBurstTime() <= nextThread->getBurstTime())
            {
                DEBUG(dbgThread, "Priority of Next thread is low: " << nextThread->name);
                DEBUG(dbgThread, "Put back to readyList");
			    kernel->scheduler->ReadyToRun(nextThread);                               
                nextThread = this;
            }
```
* 若上面的判斷皆執行完發現`nextThread`並非目前的Thread，代表發生preempt須要執行context switch
```cpp
            if (nextThread != this) 
            {
                DEBUG(dbgThread, "Priority of Next thread is high: " << nextThread->name);
                DEBUG(dbgThread, "Run and Put " << this->name << " back to readyList");
			    kernel->scheduler->ReadyToRun(this);                               
			    kernel->scheduler->Run(nextThread, FALSE);
            }
        }
```
* 非SRTF的其他排成方法沿用原本的執行方式
```cpp
        else
        {
            kernel->scheduler->ReadyToRun(this);
	        kernel->scheduler->Run(nextThread, FALSE);
        }
        
    }

    (void) kernel->interrupt->SetLevel(oldLevel);
}
```

5. 修改`Sleep`
* 一個Thread執行完畢後，為避免Nachos進入`Idle`而自行關閉，故將`advance`設為ture超前判斷`readyList`中的Thread的可執行狀況
```cpp
【threads/thread.cc】

void Thread::Sleep (bool finishing) {
    /* 略 */
    while ((nextThread = kernel->scheduler->FindNextToRun(true)) == NULL)
	kernel->interrupt->Idle();	// no one to run, wait for an interrupt
    /* 略 */
}
```
