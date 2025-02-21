/*
 * define the syscall numbers of PKE OS kernel.
 */
#ifndef _SYSCALL_H_
#define _SYSCALL_H_

// syscalls of PKE OS kernel. append below if adding new syscalls.
#define SYS_user_base 64
#define SYS_user_print (SYS_user_base + 0)
#define SYS_user_exit (SYS_user_base + 1)
// added @lab2_2
#define SYS_user_malloc (SYS_user_base + 2)
#define SYS_user_free (SYS_user_base + 3)


// added @lab3_1
#define SYS_user_fork (SYS_user_base + 4)
#define SYS_user_yield (SYS_user_base + 5)
#define SYS_user_wait (SYS_user_base + 6)
//lab1_challenge1
#define SYS_user_print_backtrace (SYS_user_base + 20)

long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7);
ssize_t sys_user_yield();
#endif
