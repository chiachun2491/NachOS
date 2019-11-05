# HW2 - CPU Scheduler

## Work division

+ `MultiProgramming & FIFO` B10615013 李聿鎧
+ `SJF & SRTF Scheduling` B10615024 李韋宗
+ `SJF & SRTF Scheduling` B10615043 何嘉峻
---
## Source Code
Github: https://github.com/chiachun2491/NachOS.git

---
## Report
### 1. Multi-Programming
1.  在 `userporg/addrspace.h` 新增變數，紀錄 Physical page 使用狀況
```cpp
// userporg/addrspace.h

class AddrSpace {
  public:
    // record used state of the main memory page
    static bool usedPhyPage[NumPhysPages];	
    /* ... */
};
```
2. 在 `usrprog/addrspace.cc` 將 Physical page 初始化為 0
```cpp
// usrprog/addrspace.cc

// initial usedPhyPage to zero
bool AddrSpace::usedPhyPage[NumPhysPages] = {0};
```

3. 使用完 Page 後，必須要釋放資源
```cpp
// usrprog/addrspace.cc

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
並要更改讀取 Code 以及 Data 的位置
```cpp
// usrprog/addrspace.cc

bool 
AddrSpace::Load(char *fileName) 
{
    /* ... */
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
    /* ... */
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
    /* ... */
}
```
#### Screenshot
如下圖所示，`test1` 及 `test2` 同時運行不會互相影響。
![MP](https://i.imgur.com/1AejwVP.png)

---
### 2. CPU Scheduler
1. 從 command 中記錄各種排程
```cpp
// threads/kernel.cc

ThreadedKernel::ThreadedKernel(int argc, char **argv) 
{
        /* ... */
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

2. 幫每種排程建立不同的 SortedList，並給予不同的 Compare Function
```cpp
// threads/addrspace.cc

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
// threads/scheduler.cc

int FIFOCompare(Thread *a, Thread *b) {
    return 1;
}
```

#### Screenshot
如下圖所示，`test1` 全部執行完成後 `test2` 才執行。

![FCFS](https://i.imgur.com/yc3Dz10.png)

#### 最短工作優先排程 (Shortest-Job-First, SJF)
我們優先考慮 `burstTime`，接著才考慮 `arrivalTime`。
```cpp
// threads/scheduler.cc

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
測試排程如下
```cpp
// threads/thread.cc

void Thread::SelfTest() {
    const int number = 5;
    char *name[number] = {"A", "B", "C", "D", "E"};
    int burst[number] = {1, 2, 3, 2, 1};
    int priority[number] = {4, 5, 3, 1, 2};
    int arrival[number] = {0, 1, 2, 3, 4};
    /* ... */
}
```
#### Screenshot
如下圖所示，5 個模擬 thread 正確執行。

![SJF](https://imgur.com/WPJovH2.png)


#### 最短剩餘時間排程 (shortest remaining time first ,SRTF)
因為與 SJF 差別在於 preemptive，我們直接延用 `SJFCompare`。
```cpp
// threads/scheduler.cc

int SRTFCompare(Thread *a, Thread *b) {
    return SJFCompare(a, b);
}
```

在`CallBack`中新增 SRTF 為可以被 preempt 的一種排程。

```cpp
// threads/alarm.cc

void Alarm::CallBack() {
    /* ... */
    if(kernel->scheduler->getSchedulerType() == RR ||
        kernel->scheduler->getSchedulerType() == Priority ||
        kernel->scheduler->getSchedulerType() == SRTF) 
        {
            interrupt->YieldOnReturn();
        }
}
```

模擬排程如下
```cpp
// threads/thread.cc

void Thread::SelfTest() {
    const int number 	 = 3;
    char *name[number] 	 = {"A", "B", "C"};
    int burst[number] 	 = {1, 5, 1};
    int priority[number] = {4, 5, 3};
    int arrival[number] = {0, 1, 3};
    /* ... */
}
```

#### Screenshot
如下圖所示，3 個模擬 thread 正確執行。

![SRTF](https://imgur.com/O5iNy3W.png)

---
### 3. SRTF 與SJF 的詳細實作方法
1. 在 `thread.h` 中加入 `static` 變數 `currentTime` 並初始為 0
```cpp
// threads/thread.cc

//----------------------------------------------------------------------
// Thread::currentTime
// 	Set for SJF & SRTF Scheduler to determine thread is arrival or not.
//----------------------------------------------------------------------

int
Thread::currentTime = 0;
```

2. 在 SRTF 排程中，為了要每次都檢查是否有優先權更高的 thread 可以執行，因此改為直接呼叫 `Yield()` 而非 `OneTick()`
```cpp
// threads/thread.cc

static void SimpleThread() {
    while(thread->getBurstTime() > 0) {
    /* ... */
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

3. 修改 `FindNextToRun`

    1. 由於 SRTF 與 SJF 我們都有實作考慮`arrivalTime`，故需要與其他排程分開
    
    3. 宣告 `smallest` 作為預估傳回的 `nextThread`


    5. 因為我們 `readyList` 是依照 burstTime 排序，因此我們要判斷從 `readyList` 從頭開始是否有 thread 已經到達，並且可以 preempt，若 `readyList` 中第一個 thread 尚未到達，我們必須要往後檢查是否有 burstTime 較大但已經到達的 thread 可以執行，如果有找到，將 `smallest` 替換成該 thread，若都沒有找到則離開 while 迴圈


    7. 如果有找到 thread 就從`readyList`中移除並回傳


    9. 沒有找到且不需要`advance`才回傳 `NULL`；需要`advance`時，將 `Thread::currentTime` 設定為 `smallest.arrivalTime`，並且從 `readyList` 中移除並回傳


    11. 非 SJF 與 SRTF 的其他排成方法沿用原本的回傳方式
```cpp
// threads/scheduler.cc

Thread * Scheduler::FindNextToRun (bool advance)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (readyList->IsEmpty()) 
    {
	    return NULL;
    } 
    
    // Note 1 ↓
    if (kernel->scheduler->getSchedulerType() == SRTF || kernel->scheduler->getSchedulerType() == SJF)
    {
        ListIterator<Thread *> *iter = new ListIterator<Thread *>(readyList);
        // Note 2 ↓
        Thread * smallest = iter->Item();

        DEBUG(dbgThread, "SRTF Estimate FindNextToRun: " << smallest->getName());
        
        // Note 3 ↓
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
        
        // Note 4 ↓
        if (!iter->IsDone())
        {
            Thread *t = iter->Item(); // Backup
            readyList->Remove(iter->Item());
            DEBUG(dbgThread, "Remove from readyList and return: " << t->getName());
            return t;
        }
        
        // Note 5 ↓
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
    
    // Note 6 ↓
    else
    {
    	return readyList->RemoveFront();
    }
}
```

4. 修改 `Yield()`
    1. 會呼叫 `Yield()` 是每次 `SimpleThread()` 進行 `currentThread.burstTime -1`，表示 `currentThread` 仍在運行，我們只需檢查有無 thread 可以 preempt 或接著做，因此在 `FindNextToRun()` `advance`須傳入 `false`


    3. 因為在 SRTF 中，傳回的 `nextThread` 有可能 burstTime 還是比 `currentThread` 的大，因此需要判斷是否要做 context switch


    5. 若 `currentThread` 的 `burstTime` 比 `FindNextToRun` 中找到的還小，則不進行 context switch


    7. 若需要 context switch 的話，我們須將 `currentThread` 丟回 `readyList`，並改成 run `nextThread`


    9. 非 SRTF 的其他排成方法沿用原本的執行方式
```cpp
// threads/scheduler.cc

void
Thread::Yield ()
{
    Thread *nextThread;
    IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);
    
    ASSERT(this == kernel->currentThread);
    
    DEBUG(dbgThread, "Yielding thread: " << name);
    
    // Note 1 ↓
    nextThread = kernel->scheduler->FindNextToRun(false);

    if (nextThread != NULL)
    {
        if (kernel->scheduler->getSchedulerType() == SRTF)
        {
            // Note 2, 3 ↓
            if (this->getBurstTime() <= nextThread->getBurstTime())
            {
                DEBUG(dbgThread, "Priority of Next thread is low: " << nextThread->name);
                DEBUG(dbgThread, "Put back to readyList");
                kernel->scheduler->ReadyToRun(nextThread);                               
                nextThread = this;
            }

            // Note 4 ↓
            if (nextThread != this) 
            {
                DEBUG(dbgThread, "Priority of Next thread is high: " << nextThread->name);
                DEBUG(dbgThread, "Run and Put " << this->name << " back to readyList");
			    kernel->scheduler->ReadyToRun(this);                               
			    kernel->scheduler->Run(nextThread, FALSE);
            }
        }
        
        // Note 5 ↓
        else
        {
            kernel->scheduler->ReadyToRun(this);
	    kernel->scheduler->Run(nextThread, FALSE);
        }
    }

    (void) kernel->interrupt->SetLevel(oldLevel);
}
```

5. 修改 `Sleep`

* 一個 Thread 執行完畢後中間有 Idle 情況發生，為避免 NachOS 進入`Idle()` 而**自行關閉**，則在呼叫檢查 `FindNextToRun` 時將`advance` 設為 `ture`，讓 `Thread::currentTime` 加速到 `arrivalTime` 最接近 `currentTime` 的 `nextThread`
```cpp
// threads/thread.cc

void Thread::Sleep (bool finishing) {
    /* ... */
    while ((nextThread = kernel->scheduler->FindNextToRun(true)) == NULL)
	kernel->interrupt->Idle();	// no one to run, wait for an interrupt
    /* ... */
}
```


---
###### tags: `NachOS`
