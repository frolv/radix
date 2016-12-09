/*
 * include/untitled/sched.h
 * Copyright (C) 2016 Alexei Frolov
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

#ifndef UNTITLED_SCHED_H
#define UNTITLED_SCHED_H

#include <untitled/list.h>
#include <untitled/sys.h>
#include <untitled/types.h>

struct task {
	pid_t pid;
	uid_t uid;
	gid_t gid;
	mode_t umask;
	char **cmdline;
	char *cwd;
	int priority;
	int exit_code;
	struct regs regs;
	struct list queue;
};

extern struct task *current_task;

#endif /* UNTITLED_SCHED_H */
