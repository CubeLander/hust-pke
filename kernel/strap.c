/*
 * Utility functions for trap handling in Supervisor mode.
 */

#include "strap.h"
#include "memlayout.h"
#include "pmm.h"
#include "process.h"
#include "riscv.h"
#include "string.h"
#include "syscall.h"
#include "util/functions.h"
#include "vmm.h"

#include "spike_interface/spike_utils.h"

//
// handling the syscalls. will call do_syscall() defined in kernel/syscall.c
//
static void handle_syscall(trapframe *tf) {
  // tf->epc points to the address that our computer will jump to after the trap
  // handling. for a syscall, we should return to the NEXT instruction after its
  // handling. in RV64G, each instruction occupies exactly 32 bits (i.e., 4
  // Bytes)
  tf->epc += 4;

  // TODO (lab1_1): remove the panic call below, and call do_syscall (defined in
  // kernel/syscall.c) to conduct real operations of the kernel side for a
  // syscall. IMPORTANT: return value should be returned to user app, or else,
  // you will encounter problems in later experiments!
  tf->regs.a0 = do_syscall(tf->regs.a0, tf->regs.a1, tf->regs.a2, tf->regs.a3,
                           tf->regs.a4, tf->regs.a5, tf->regs.a6, tf->regs.a7);
  // panic( "call do_syscall to accomplish the syscall and lab1_1 here.\n" );
}

//
// global variable that store the recorded "ticks". added @lab1_3
static uint64 g_ticks = 0;
//
// added @lab1_3
//
void handle_mtimer_trap() {
  sprint("Ticks %d\n", g_ticks);
  // TODO (lab1_3): increase g_ticks to record this "tick", and then clear the
  // "SIP" field in sip register. hint: use write_csr to disable the SIP_SSIP
  // bit in sip. panic( "lab1_3: increase g_ticks by one, and clear SIP field in
  // sip register.\n" );
  g_ticks++;
  write_csr(sip, read_csr(sip) & ~SIP_SSIP);
}

//
// the page fault handler. added @lab2_3. parameters:
// sepc: the pc when fault happens;
// stval: the virtual address that causes pagefault when being accessed.
//
void handle_user_page_fault(uint64 mcause, uint64 sepc, uint64 stval) {
  sprint("handle_page_fault: %lx\n", stval);
  switch (mcause) {
  case CAUSE_STORE_PAGE_FAULT:
    if (stval < USER_STACK_TOP) {
      // 有新增的栈请求
      // 同时需要大于用户栈的栈底（未实现）
      void *pa = alloc_page();
      if (pa == 0)
        panic("uvmalloc mem alloc failed\n");

      memset((void *)pa, 0, PGSIZE);
      // 如果这里不做ROUNDDOWN，会导致对于一个非对齐的地址，最终map_pages分配两个页，在之后的分配中就会出现问题。
      user_vm_map((pagetable_t)current->pagetable, ROUNDDOWN(stval, PGSIZE),
                  PGSIZE, (uint64)pa, prot_to_type(PROT_WRITE | PROT_READ, 1));
    }

    break;
  default:
    sprint("unknown page fault.\n");
    break;
  }
}

//
// kernel/smode_trap.S will pass control to smode_trap_handler, when a trap
// happens in S-mode.
//
void smode_trap_handler(void) {
  // make sure we are in User mode before entering the trap handling.
  // we will consider other previous case in lab1_3 (interrupt).
  if ((read_csr(sstatus) & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  assert(current);
  // save user process counter.
  current->trapframe->epc = read_csr(sepc);

  // if the cause of trap is syscall from user application.
  // read_csr() and CAUSE_USER_ECALL are macros defined in kernel/riscv.h
  uint64 cause = read_csr(scause);

  // use switch-case instead of if-else, as there are many cases since lab2_3.
  switch (cause) {
  case CAUSE_USER_ECALL:
    handle_syscall(current->trapframe);
    break;
  case CAUSE_MTIMER_S_TRAP:
    handle_mtimer_trap();
    break;
  case CAUSE_STORE_PAGE_FAULT:
  case CAUSE_LOAD_PAGE_FAULT:
    // the address of missing page is stored in stval
    // call handle_user_page_fault to process page faults
    handle_user_page_fault(cause, read_csr(sepc), read_csr(stval));
    break;
  default:
    sprint("smode_trap_handler(): unexpected scause %p\n", read_csr(scause));
    sprint("            sepc=%p stval=%p\n", read_csr(sepc), read_csr(stval));
    panic("unexpected exception happened.\n");
    break;
  }

  // continue (come back to) the execution of current process.
  switch_to(current);
}
