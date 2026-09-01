#pragma once
/* Minimal errno subset, shared by kernel and userspace. Values match Linux. */

#define EPERM   1  /* operation not permitted     */
#define ENOENT  2  /* no such file or directory   */
#define EBADF   9  /* bad file descriptor         */
#define ENOMEM 12  /* out of memory               */
#define EFAULT 14  /* bad address                 */
#define EINVAL 22  /* invalid argument            */
#define ENOSYS 38  /* function not implemented    */
