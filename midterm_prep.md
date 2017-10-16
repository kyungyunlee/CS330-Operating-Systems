# 2017 Fall Operating System midterm prep

## Process, Threads, Synchronization

* Designing process system and thread system
* How to synchronize multiple processes/threads

### So what is operating systems?
Operating system
  * manage hardware (CPU, memory, I/O devices...)
  * interface between user application and hardware
  * allows user application to exploit hardware
  * user mode and kernel mode
  * driven by interrupts
  * OS will manage how to run multiple user applications, what to do when the user calls an illegal address and etc.(그래서 pintos에서 system call을 디자인하고...priority scheduling을 구현한거지...) 

  * Operating systems are higher level than ISAs and microarchitecture. 

### Process
* Process is an abstraction. It gives an illusion that each process has its own CPU, since each process seem to have its own registers, PC, SP, address space and files/resources.
* Process allows concurrency and also protection. Processes do not share data. Concurrency occurs through context switching, where this switch occurs by
  * timer interrupt
  * voluntary yield
  * I/O interrupt
  * trap

* Who creates new processes?
  * new processes are created by fork system call. 
* Who manages processes and how?
  * In our case, kernel manages processes
* What are the major data structures?
* What are the functions needed to manage processes?

* How to create new processes?
  * fork() vs creating from scratch
    * creating from scratch 
      * need new PCB (PCB is equivalent to context of a process), initialize with 0/null values
      * need new address space, new stack
      * put process in the ready list
    * fork()
      * copy PCB just with different pid
      * copy address space
      * put process in the ready list
      * Advantage of forking 
        * faster than trying to create entirely new set of process from scratch
        * reduce redundancy if parent and child process are going to be related closely (ex. web server)

* Process termination
  * resources related to the terminated process  must be freed
  * parent has to reap the child
  * parent waits for the child to terminate 
  * what if parent terminates before child?
    * child becomes an orphan, gets adopted by the init() process
  * what if child exits before parent?
    * child will notify its parent and be reaped by the parent
    * if not reaped, it becomes a zombie process just taking up memory



### Threads
* What are threads?
  * unit of scheduling and execution
  * normally threads are managed by processes
  * threads within a process share data, code, resources, heap
  * threads gets their own PC, SP, registers

* Why use threads?
  * threads are faster to create than processes
  * takes less memory space overall
  * faster to switch between threads. only need to save and restore registers, PC, SP

* Downside
  * threads don't provide protection between other threads

* How to create new thread
  * similar to forking in process but lighter
  * the process that this thread is contained in will have larger address space
  * copy some parts of PCB into new TCB

* Kernel level threads vs User level threads
  * kernel level : scheduling, management done by the kernel
    * Good
      * when one thread is blocked only this thread of the process gets blocked
    * Bad
      * slower to do context switching and scheduling since kernel does it
      * hard to port to other systems
  * user level : done by the user library
    * Good
      * faster to switch between threads, no need of the kernel
      * can port to many systems
    * Bad
      * when one thread is blocked, the entire process' threads get blocked as well
      * cant utilize multiple cores 

* Different types of thread models
  * one to one
  * one to many
  * many to many

#### Design of a thread system

* Data structure for threads
```
struct TCB {
  uint32_t tid;
  uint32_t registers[num_of_registers];
  uint32_t PC;
  uint32_t SP;
  uint32_t priority;
}

struct list ready_list;
struct list sleeping_list;

// use resource related data structure of the process
```

* Major functions 
  * thread_create()
  * thread_execute()
  * thread_exit()
  * thread_yield()
  * thread_schedule()
  * thread_switch()
  
* Who schedules threads? and how?
  * If kernel threads, kernel will schedule threads
  * priority scheduling may be used
    * higher priority thread will always run first
    * problems related to priority scheduling solved by priority donation
      * nested donation
      * multiple donation
* Orphan threads, zombie threads?



#### Process vs threads vs programs vs functions 
* process : unit of resource allocation
    Process has its own address space
* threads : unit of scheduling and execution 
    Within one process, threads share code, data, heap, open files, but they have their own registers, PC and SP.
    TCB
* program : this is just instruction (code)
    When program is executed through process, stack area is created along with PC and SP. Multiple processes can execute the same program in their own way.
* function : 
    When function is called, it does not get its own PC or SP. 

### Synchronization
* Why do we need to synchronize?
  * Prevent race conditions. 
    * race condition is when multiple processes are accessing a shared data and the order of their access changes the output. 
    * example : reading and writing to the same file, accessing the global ready_list in pintos
  * Critical section problem.
    * critical section refers to blocks of instruction that access global variables.

* Different types of synchronization method
  * Hardware level : TestAndSet, CompareAndSwap
  * Software level : Petersons solution
  * OS level : locks and semaphores
  * Programming language level : Monitors 

  * Hardware level
    This is where we make the instruction atomic. In case of TestAndSet, we test for condition and set the variable atomically, so no interrupts can occur during. If the other process tries to call TestAndSet, it will be busy waiting. 
    * Implement a simple spinlock using compare_and_swap()

```
    int compare_and_swap (int *a, int old, int new){
      if (*a == old) {
        *a = new;
        return 1;
      } else {
        return 0;
      }
    }
    
    struct lock {
      int val;
      struct thread * holder;
    }

    // initialize lock->val to 0
    void acquire(struct lock * lock){
      while (!compare_and_swap(lock->val, 0, 1)) {
        ;;
      }
      lock->holder = current_thread();
    }

    void release(struct lock * lock){
      compare_and_swap(lock->val, 1, 0);
      lock->holder = NULL;
    }

```

  * Software level : Petersons solution (교수님 제발..)
    * Petersons solutions provides a software level sync that assures mutual exclusion and starvation-free progress. 
    
```
// for process i 
do {
  int flag[i] = TRUE;
  int victim = i;
  while (flag[j] && victim == i); // do nothing, wait
  //critical section
  flag[i] = FALSE;
  //remainder section
} while (TRUE);
```

   * Prove mutual exclusion is always satisfied
      * We need to show that no 2 processes are in the critical section at the same time. 
      * Lets say that process i is in the critical section. This means that either 1. flag[j] = FALSE or 2. victim = j. Also, flag[i] = TRUE, since process i is in the critical section.
        * Assume flag[j] == FALSE;
          * It means that process j is in the remainder section or has not entered the entry section. If it tries to get into cs, flag[i] == TRUE, so it cannot get in. 
        * Assume victim == j;
          * It means that process j is after line 2. But since flag[i] == TRUE && victim == j, it cannot get into cs.
    * Prove that this program is starvation free
      * Let's say the program is not starvation free. This means 
        * one process stays in the entry forever
          * this cannot happen because if process i is waiting in the while loop, victim will change at some point and i can proceed to cs
        * one process stays in the remainder forever
          * if process i exits, flag[i] = FALSE and another process can enter cs
        * one process is continuously exiting and entering the cs. 
          * if process i exits and tries to enter again, it sets flag[i] == TRUE and victim == i. Then, if another process was already waiting to get in, it can enter cs eventually.
  
  * OS level
    * semaphores 
      * counting semaphores, binary semaphores
      * multiple processes can access the semaphore
      * can be implemented without busy waiting through thread_block() (pintos)
    * locks
      * only have values 0 or 1, like a binary semaphore
      * used for mutual exclusion

  * Programming Language level (머리아파...) 
    * Monitor 
      * provide a higer level synchronization method (simpler to implement in user point of view. prevents user's stupid errors)
      * it provides mutual exclusion by default
      * it prevents deadlock
      * only one process can enter the monitor (use the procedures within the monitor) at a time
      * condition variables are implemented with semaphores to make the process wait for certain resources

      * design a monitor (hoare style vs messa style but here, hoare style is used) 
        * hoare style : signal and wait
        * 3 queues needed at least  
          * entry queue : waiting to enter the monitor
          * condition queue : waiting for condition
          * signal queue : a process that has called signal() on the condition will wake up a process in the condition queue before it exits. If there is a process to be woken up, it will wait in this queue until that process exits the monitor or goes into another condition queue.

```
<<<<<<< HEAD
monitor Monitor {
  semaphore mutex; // for entering monitor
  semaphore x_sem; // for cond var x
  int x_count;
  semaphore signal_queue; // for processes that called signal
  int sig_queue_count;
  
  Procedure do_something {
    wait(mutex);
    // body
    x.wait();
    // end of body 
    if (sig_queue_count > 0) {
      signal(signal_queue); // wake up one in signal queue
    }
    else 
      signal(mutex);
  }
  
  Procedure do_something_else {
    wait(mutex);
    // body
    x.signal();
    // end of body
    if(sig_queue_count > 0) {
      signal(signal_queue);
    } else {
      signal(mutex);
    }
  }


  initialize();
  
}

/* for cond var x, x.wait() will be :
  여기서 키포인트는 현재 프로세스가 블락되면 모니터를 안쓰게 되니까 그럼 다른 프로세스가 쓰게 해주자라는것이다. 
  그럼 queue가 3개인데 누구한테 쓰라고 하면 되나. 일단 cond var queue는 아니겠지, 왜냐면 그거 누가 쓰고 있으니까 지금 프로세스가 블락되는거자나. 
  그럼 entry queue vs signal queue인데, signal queue에 있는 프로세스들은 이미 모니터안에 들어와있었으니까 걔네들한테 우선순위를 주는게 맞지 않겠나. 
  그래서 만약에 signal queue에 있는 프로세스가 있다면 걔네들을 먼저 실행시켜준다. 만약 아무도 없었다면 entry queue에 있던 친구를 실행시켜주면 되겠지.
  x_count ++;
  // before the current process gets blocked, run other processes
  if (signal_queue_count > 0 ) {
    signal(signal_queue);
  } else {
    signal(mutex);
  }
  wait(x_sem); // current process blocked 
  x_count --;
*/

/* for cond var x, x.signal() will be :
여기의 포인트는 내가 resource를 free시키면 x.wait()에 기다리던 아이를 깨우게 된다. 
근데 그럼 모니터 안에 프로세스가 2개가 동시에 실행되게 돼는데 이건 모니터의 룰을 어기는 것이라 불법이다.
그래서 시그널을 보낸 애를 기다리게 하자라는 것이다.
근데 만약에 이 resource (즉 x)를 기다리던 아이가 없었다면 상관없다. 
하지만 x를 기다리고 있어서 깨워지는 친구가 있다면 나는 signal queue에 들어가서 기다릴것이다. 
그럼 다시 어떤 프로세스가 resource를 기다리게 되거나 (x.wait()을 불러서) 어떤 프로세스가 모니터를 다 사용하고 나갈때 이 프로세스를 깨워주게 될것이다.
  if (x_count > 0) {
    signal_queue_count ++;
    signal(x_sem);
    wait(signal_queue);
    sigal_queue_count --;
  }

*/
```

* Famous synchronization problems : 
  * Bounded buffer problem  : solved with 1 mutex lock, 2 semaphores
    * producer wait if buffer is full
    * consumer wait if buffer is empty
```
semaphore mutex; // init to 0
semaphore filled // init to 0
semaphore empty // init to N
producer(){
  wait(mutex);
  wait(empty); // if empty == 0, wait, empty--
  // do something
  signal(filled); // filled++
  signal(mutex);
}
consumer(){
  wait(mutex);
  wait(filled); // if filled == 0, wait, filled--
  // do something
  signal(empty); // empty--
  signal(mutex);
}
```
  * Read Writer problem : solved with 1 mutex lock, 1 semaphore, 1 counter
    * reader : wait if writer is doing work
    * writer : wait if writer or any reader is doing work
```
semaphore mutex; // init to 0
int read_count = 0;
semaphore writing;

reader(){
  wait(mutex);
  if (read_count ==0) // if no one was reading, check if writer is doing work
    wait(writing);
  read_count++;
  signal(mutex);
  // do work
  wait(mutex);
  if (read_count == 1) // if i am the last one reading, notify the writer to do work
    signal(writing);
  read_count --;
  signal(mutex);
}

writer(){
  wait(writing);
  // do work
  signal(writing);
}

```

* Dining philosophers problem : solved with monitor
  * 5 philosophers and 5 chopsticks. only philosophers with both chopsticks can eat, else think
  * dining philosopher problem with sempahores/mutex leads to deadlock. why?
```
philosopher(){
  wait(left_chopstick);
  wait(right_chopstick);
  //eat
  signal(left_chopstick);
  signal(right_chopstick);
}
```
  * if interrupt happens after first wait, then all philosophers grab their left chopstickand try to grab the right one. -> deadlock
  * resolve deadlock by 
    * change the order of grabbing chopstick for some philosophers (right -> left)
    * only pickup chopstick if both chopsticks are available
    * solve by using monitor...

* Critical section problem should satisfy 
  * mutual exclusion : only one process in the cs
  * progress : if no one is in the cs, a waiting process can get in some time
  * bounded waiting : there is certain time limit to when the process can go into the cs
  * no progress means no threads can get into cs, no bounded waiting is when a process is waiting indefinitely since another one is continuously entering and exiting


### Other : 

#### Concurrency vs parallelism
* These two ideas have greatly impacted computer science
* Need to distinguish single core and multicore processors
* A system can be concurrent without parallelism.
  This happens with single core processors with multiple processes running concurrently. Concurrency does not mean that they are running simultaneously at the same time. It means that multiple processes are running, but with context switching, which means that only one process is taking CPU resources at a time. 
  ex. threads in single core processor
* A system can be parallel without concurrency. 
  This happens with multi core processor, where each core runs one process at a time from start to end and then runs the next one. There is no concurrency since only one process runs on one core, but multiple processes run simultaneously on different cores.
  ex. running one matrix multiplication
* A system can be parallel and concurrent, obviously..
  Multicore processor with concurrency, as in many modern computers


#### Mechanism vs Policy
* policy : a rule to follow
* mechanism : a way to implement following the policy

#### Design vs Knowledge 
* design : coming up with novel ideas, being critical, knowing why
* knowledge : just what you know, fact

### HW1
* kebab house
  * cook : goes to sleep if no customers
  * customer : goes home if all waiting seats are full
```
int chair= N; // init to N chairs 
semaphore mutex; // mutex for accessing chair variable
semaphore kebab; // init to 0 kebabs 

cook(){
  while (chair == N) sleep();
  make_kebab();
}

customer(){
  if (chair == 0) leave();
  get_kebab_and_leave();
}

make_kebab(chair){
  // cook
  signal(kebab); // kebab ++
}

get_kebab_and_leave(){
  wait(mutex);
  chair--;
  signal(mutex);

  wait(kebab); // if kebab == 0, wait, then kebab--

  wait(mutex);
  chair++;
  signal(mutex);
}

```

## GOOD LUCK
