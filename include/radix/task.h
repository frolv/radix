/*
 * include/radix/task.h
 * Copyright (C) 2021 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RADIX_TASK_H
#define RADIX_TASK_H

#include <radix/asm/regs.h>
#include <radix/cpumask.h>
#include <radix/list.h>
#include <radix/mm_types.h>
#include <radix/percpu.h>
#include <radix/types.h>

#include <stdbool.h>

struct vmm_space;

enum task_state {
    // The task is ready to be scheduled.
    TASK_READY,

    // The task is currently running on a CPU.
    TASK_RUNNING,

    //
    // States > RUNNING are considered inactive.
    //

    // The task is waiting on a resource and unschedulable.
    TASK_BLOCKED,

    // The task has completed execution and exited.
    TASK_FINISHED,

    // Currently unused.
    TASK_ZOMBIE,
};

// A single task (process/kthread) in the system.
//
// Rearranging the members of this struct requires changes to be made to the
// switch_task function for every supported architecture.
struct task {
    enum task_state state;
    int priority;
    int prio_level;
    uint32_t flags;
    pid_t pid;
    uid_t uid;
    gid_t gid;
    mode_t umask;
    struct regs regs;
    struct list queue;
    struct vmm_space *vmm;
    void *stack_top;
    size_t stack_size;
    struct task *parent;
    cpumask_t cpu_affinity;
    cpumask_t cpu_restrict;
    uint64_t sched_ts;
    uint64_t remaining_time;
    char **cmdline;
    char *cwd;
    int errno;
    int exit_status;
};

#ifdef __cplusplus
extern "C" {
#endif

static inline bool task_is_active(const struct task *t)
{
    return t->state <= TASK_RUNNING;
}

#define TASK_FLAGS_IDLE   (1 << 0)
#define TASK_FLAGS_ON_CPU (1 << 1)

// Compares two tasks in terms of priority.
//
// Returns a negative number if A is higher priority, positive if B is higher,
// or 0 if the two are equal.
int task_comparator(const struct task *a, const struct task *b);

DECLARE_PER_CPU(struct task *, current_task);
#define current_task() (this_cpu_read(current_task))

struct task *task_alloc(void);
void task_free(struct task *task);

void task_exit(struct task *task, int status);

// Creates a new user mode task running the executable located at a specified
// file path.
//
// This initializes a new task with its own address space, loads and maps the
// executable into that address space, and sets up the basic task parameters
// and registers required for it to run. The task is not started; it must be
// added to the scheduler separately, and may be modified by the creator prior
// to execution.
//
// Returns an ERR_PTR to the initialized task.
struct task *task_create(const char *path);

// Sets up the registers and stack for a kernel thread to start executing
// function `func` with argument `arg`. Implemented by individual architectures.
void kthread_reg_setup(struct regs *regs,
                       addr_t stack_top,
                       addr_t func,
                       addr_t arg);

// Sets up the registers and stack for a user task to start executing from
// address `entry`. Implemented by individual architectures.
//
// `stack` is the base physical address of the initially allocated page for the
// user stack. It is already mapped into the task's address space.
int user_task_setup(struct task *task, paddr_t stack, addr_t entry);

// Switches from running task old to task new. Implemented separately by each
// architecture. Once the function call returns, `new` should be executing.
//
// Current register state should be saved into `old` and loaded from `new`.
// Additionally, TASK_FLAGS_ON_CPU should be cleared from old's flags once it
// is no longer running, and set in new's flags as soon as it first starts.
void switch_task(struct task *old, struct task *new);

void tasking_init(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // RADIX_TASK_H
