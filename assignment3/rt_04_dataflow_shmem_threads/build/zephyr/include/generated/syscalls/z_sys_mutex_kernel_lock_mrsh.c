/* auto-generated by gen_syscalls.py, don't edit */

#include <syscalls/mutex.h>

extern int z_vrfy_z_sys_mutex_kernel_lock(struct sys_mutex * mutex, k_timeout_t timeout);
uintptr_t z_mrsh_z_sys_mutex_kernel_lock(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
		uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, void *ssf)
{
	_current->syscall_frame = ssf;
	(void) arg3;	/* unused */
	(void) arg4;	/* unused */
	(void) arg5;	/* unused */
	union { uintptr_t x; struct sys_mutex * val; } parm0;
	parm0.x = arg0;
	union { struct { uintptr_t lo, hi; } split; k_timeout_t val; } parm1;
	parm1.split.lo = arg1;
	parm1.split.hi = arg2;
	int ret = z_vrfy_z_sys_mutex_kernel_lock(parm0.val, parm1.val);
	_current->syscall_frame = NULL;
	return (uintptr_t) ret;
}

