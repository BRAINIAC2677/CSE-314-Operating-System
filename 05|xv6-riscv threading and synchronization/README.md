# Threading & Synchronization

## 1. Threading
- ### proc.c
    - `modified freeproc`
    ```cpp
    static void
    freeproc(struct proc *p)
    {
    if (p->trapframe)
        kfree((void *)p->trapframe);
    p->trapframe = 0;
    if (p->pagetable)
    {
        if (p->is_thread)
        {
        thread_freepagetable(p->pagetable, p->sz);
        }
        else
        {
        proc_freepagetable(p->pagetable, p->sz);
        }
    }
    p->pagetable = 0;
    p->sz = 0;
    p->pid = 0;
    p->parent = 0;
    p->name[0] = 0;
    p->chan = 0;
    p->killed = 0;
    p->xstate = 0;
    p->state = UNUSED;
    }
    ```