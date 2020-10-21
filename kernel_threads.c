
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_cc.h"
#include "util.h"


/** 
  @brief Create a new thread in the current process.
  */
void start_threads()
{
  int exitval;
  PTCB *ptcb = CURTHREAD->owner_ptcb;
  
  Task call = ptcb->ptcb_thread_task;
  int argl = ptcb->argl;
  void* args = ptcb->args;
  exitval = call(argl,args);
  sys_ThreadExit(exitval);
}


Tid_t sys_CreateThread(Task task, int argl, void* args){
  
  TCB *curtcb = (TCB*)malloc(sizeof(TCB));

  CURPROC->amount_of_threads++;
  assert(CURPROC == CURTHREAD->owner_pcb);
  PTCB *curptcb = (PTCB*)malloc(sizeof(PTCB));
  curptcb->ref_count = 0;
  curptcb->detached = 0;
  curptcb->exited = 0;
  curptcb->ptcb_thread_task = task;
  curptcb->argl = argl;
  curptcb->cond_var_joiners = COND_INIT;
  curptcb->args = args;

  /**if(args!= NULL) {
    curptcb->args = args;
  } else{
    curptcb->args = NULL;**/
  
 
  


  if(task != NULL ) {
   curtcb = spawn_thread(CURPROC,start_threads);
   curtcb->owner_ptcb = curptcb;
   curptcb->ptcb_thread=curtcb;
   rlnode *ptcb_new = rlnode_init(&curptcb->ptcb_rlnode, curptcb);
   rlist_push_back(&CURPROC->ptcb_list, ptcb_new);
   wakeup(curtcb);
  }

  return ((Tid_t)curptcb->ptcb_thread);
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
  return (Tid_t)CURTHREAD ;
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
 
  TCB *tcb_target = (TCB*)tid;
  PTCB *ptcb_target = tcb_target->owner_ptcb; 

  assert(tid != sys_ThreadSelf());
  assert(ptcb_target->detached==0);

  ptcb_target->ref_count++; 
    while( ptcb_target->detached == 0 && ptcb_target->exited == 0 ){ /* I don't want to join with a detach thread */
 
      kernel_wait(& ptcb_target->cond_var_joiners,SCHED_USER); /* I want to place myself into the waiting list*/
  
}
       /* Increase the counter of my joiners*/
       if(CURTHREAD->state == EXITED ){
         return -1;
       }
       
  if(exitval){
    *exitval = ptcb_target->exitval;
  }


  return 0;
}


/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{

  PTCB *ptcb = NULL;
  int length = rlist_len(&CURPROC->ptcb_list);

  for(int i = 0; i<length; i++){
  rlnode *ptcb_find = rlist_pop_front(&CURPROC->ptcb_list);
    if (ptcb_find->ptcb->ptcb_thread == ((TCB*) tid))
    {
       ptcb = ptcb_find->ptcb;
    }
      rlist_push_back(&CURPROC->ptcb_list, &ptcb_find->ptcb->ptcb_rlnode);
  }

  if(ptcb != NULL){
    if(ptcb->detached != 1){
       ptcb->detached = 1;
       ptcb->ref_count = 0; /* Decrease the counter of my joiners*/
       kernel_broadcast(&ptcb->cond_var_joiners);
     
       return 0;
    }  
  }
	return -1;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{ 
    TCB *tcb_exit = (TCB*)sys_ThreadSelf();
    PTCB *ptcb = tcb_exit->owner_ptcb; 
    ptcb->ref_count--;
    CURPROC->amount_of_threads--;
    ptcb->exited = 1;
    ptcb->exitval = exitval;
    kernel_broadcast(&ptcb->cond_var_joiners);
    kernel_sleep(EXITED,SCHED_USER);

}



