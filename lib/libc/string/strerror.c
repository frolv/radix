/*
 * lib/libc/string/strerror.c
 * Copyright (C) 2016-2017 Alexei Frolov
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

#include <errno.h>
#include <string.h>

static const char *errors[] = {
	[E2BIG] = "Argument list too long",
	[EACCES] = "Permission denied",
	[EADDRINUSE] = "Address in use",
	[EADDRNOTAVAIL] = "Address not available",
	[EFNOSUPPORT] = "Address family not supported",
	[EAGAIN] = "Resource unavailable, try again",
	[EALREADY] = "Connection already in progress",
	[EBADF] = "Bad file descriptor",
	[EBADMSG] = "Bad message",
	[EBUSY] = "Device or resource busy",
	[ECANCELLED] = "Operation cancelled",
	[ECHILD] = "No child processes",
	[ECONNABORTED] = "Connection aborted",
	[ECONNREFUSED] = "Connection refused",
	[ECONNRESET] = "Connection reset",
	[EDEADLK] = "Resources deadlock would occur",
	[EDESTADDRREQ] = "Destination address required",
	[EDOM] = "Math argument out of function domain",
	[EDQUOT] = "Reserved",
	[EEXIST] = "File exists",
	[EFAULT] = "Bad address",
	[EFBIG] = "File too large",
	[EHOSTUNREACH] = "Unreachable host",
	[EIDRM] = "Identifier removed",
	[EILSEQ] = "Illegal byte sequence",
	[EINPROGRESS] = "Operation in progress",
	[EINTR] = "Interrupted function",
	[EINVAL] = "Invalid argument",
	[EIO] = "I/O error",
	[EISCONN] = "Socket is connected",
	[EISDIR] = "Is a directory",
	[ELOOP] = "Too many levels of symbolic links",
	[EMFILE] = "Too many open files",
	[EMLINK] = "Too many links",
	[EMSGSIZE] = "Message too large",
	[EMULTIHOP] = "Reserved",
	[ENAMETOOLONG] = "Filename too long",
	[ENETDOWN] = "Network is down",
	[ENETRESET] = "Connection aborted by network",
	[ENETUNREACH] = "Network unreachable",
	[ENFILE] = "Too many open files",
	[ENOBUFS] = "No buffer space available",
	[ENODATA] = "No message avaliable on STREAM read queue",
	[ENODEV] = "No such device",
	[ENOENT] = "No such file or directory",
	[ENOEXEC] = "Executable format error",
	[ENOLCK] = "No locks available",
	[ENOLINK] = "Reserved",
	[ENOMEM] = "Out of memory",
	[ENOMSG] = "No message of the desired type",
	[ENOPROTOOPT] = "Protocol not available",
	[ENOSPC] = "No space left on device",
	[ENOSR] = "No stream resources",
	[ENOSTR] = "Not a stream",
	[ENOSYS] = "Function not supported",
	[ENOTCONN] = "Socket not connected",
	[ENOTDIR] = "Not a directory",
	[ENOTEMPTY] = "Directory not empty",
	[ENOTSOCK] = "Not a socket",
	[ENOTSUP] = "Not supported",
	[ENOTTY] = "Inappropriate I/O control operation",
	[ENXIO] = "No such device or address",
	[EOPNOTSUPP] = "Operation not supported on socket",
	[EOVERFLOW] = "Value too large to be stored",
	[EPERM] = "Operation not permitted",
	[EPIPE] = "Broken pipe",
	[EPROTO] = "Protocol error",
	[EPROTONOSUPPORT] = "Protocol not supported",
	[EPROTOTYPE] = "Wrong protocol type for socket",
	[ERANGE] = "Result too large",
	[EROFS] = "Read-only file system",
	[ESPIPE] = "Invalid seek",
	[ESRCH] = "No such process",
	[ESTALE] = "Reserved",
	[ETIME] = "Stream ioctl timeout",
	[ETIMEDOUT] = "Connection timed out",
	[ETXTBSY] = "Text file busy",
	[EWOULDBLOCK] = "Operation would block",
	[EXDEV] = "Cross-device link"
};

const char *strerror(int errno)
{
	return errors[errno];
}
