/*
 * include/errno.h
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

#ifndef UNTITLED_ERRNO_H
#define UNTITLED_ERRNO_H

#define E2BIG           1       /* arugment list too long */
#define EACCES          2       /* permission denied */
#define EADDRINUSE      3       /* address in use */
#define EADDRNOTAVAIL   4       /* address not available */
#define EFNOSUPPORT     5       /* address family not supported */
#define EAGAIN          6       /* resource unavailable, try again */
#define EALREADY        7       /* connection already in progress */
#define EBADF           8       /* bad file descriptor */
#define EBADMSG         9       /* bad message */
#define EBUSY           10      /* device or resource busy */
#define ECANCELLED      11      /* operation cancelled */
#define ECHILD          12      /* no child processes */
#define ECONNABORTED    13      /* connection aborted */
#define ECONNREFUSED    14      /* connection refused */
#define ECONNRESET      15      /* connection reset */
#define EDEADLK         16      /* resources deadlock would occur */
#define EDESTADDRREQ    17      /* destination address required */
#define EDOM            18      /* math argument out of function domain */
#define EDQUOT          19      /* reserved */
#define EEXIST          20      /* file exists */
#define EFAULT          21      /* bad address */
#define EFBIG           22      /* file too large */
#define EHOSTUNREACH    23      /* unreachable host */
#define EIDRM           24      /* identifier removed */
#define EILSEQ          25      /* illegal byte sequence */
#define EINPROGRESS     26      /* operation in progress */
#define EINTR           27      /* interrupted function */
#define EINVAL          28      /* invalid argument */
#define EIO             29      /* i/o error */
#define EISCONN         30      /* socket is connected */
#define EISDIR          31      /* is a directory */
#define ELOOP           32      /* too many levels of symbolic links */
#define EMFILE          33      /* too many open files */
#define EMLINK          34      /* too many links */
#define EMSGSIZE        35      /* message too large */
#define EMULTIHOP       36      /* reserved */
#define ENAMETOOLONG    37      /* filename too long */
#define ENETDOWN        38      /* network is down */
#define ENETRESET       39      /* connection aborted by network */
#define ENETUNREACH     40      /* network unreachable */
#define ENFILE          41      /* too many open files */
#define ENOBUFS         42      /* no buffer space available */
#define ENODATA         43      /* no message avaliable on STREAM read queue */
#define ENODEV          44      /* no such device */
#define ENOENT          45      /* no such file or directory */
#define ENOEXEC         46      /* executable format error */
#define ENOLCK          47      /* no locks available */
#define ENOLINK         48      /* reserved */
#define ENOMEM          49      /* not enough space */
#define ENOMSG          50      /* no message of the desired type */
#define ENOPROTOOPT     51      /* protocol not available */
#define ENOSPC          52      /* no space left on device */
#define ENOSR           53      /* no stream resources */
#define ENOSTR          54      /* not a stream */
#define ENOSYS          55      /* function not supported */
#define ENOTCONN        56      /* socket not connected */
#define ENOTDIR         57      /* not a directory */
#define ENOTEMPTY       58      /* directory not empty */
#define ENOTSOCK        59      /* not a socket */
#define ENOTSUP         60      /* not supported */
#define ENOTTY          61      /* inappropriate i/o control operation */
#define ENXIO           62      /* no such device or address */
#define EOPNOTSUPP      63      /* operation not supported on socket */
#define EOVERFLOW       64      /* value too large to be stored */
#define EPERM           65      /* operation not permitted */
#define EPIPE           66      /* broken pipe */
#define EPROTO          67      /* protocol error */
#define EPROTONOSUPPORT 68      /* protocol not supported */
#define EPROTOTYPE      69      /* wrong protocol type for socket */
#define ERANGE          70      /* result too large */
#define EROFS           71      /* read-only file system */
#define ESPIPE          72      /* invalid seek */
#define ESRCH           73      /* no such process */
#define ESTALE          74      /* reserved */
#define ETIME           75      /* stream ioctl timeout */
#define ETIMEDOUT       76      /* connection timed out */
#define ETXTBSY         77      /* text file busy */
#define EWOULDBLOCK     78      /* operation would block */
#define EXDEV           79      /* cross-device link */

extern int errno;

#endif /* UNTITLED_ERRNO_H */
