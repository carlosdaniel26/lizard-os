#pragma once
/* Minimal errno subset, shared by kernel and userspace. Values match Linux. */

#define EPERM   1  /* operation not permitted     */
#define ENOENT  2  /* no such file or directory   */
#define EIO     5  /* I/O error                   */
#define EBADF   9  /* bad file descriptor         */
#define ECHILD 10  /* no child processes          */
#define ENOMEM 12  /* out of memory               */
#define EFAULT 14  /* bad address                 */
#define EEXIST 17  /* file exists                 */
#define ENOTDIR 20 /* not a directory             */
#define EISDIR 21  /* is a directory              */
#define EINVAL 22  /* invalid argument            */
#define ERANGE 34  /* result too large            */
#define ENAMETOOLONG 36 /* file name too long     */
#define ENOSYS 38  /* function not implemented    */
#define ENOTEMPTY 39 /* directory not empty       */
