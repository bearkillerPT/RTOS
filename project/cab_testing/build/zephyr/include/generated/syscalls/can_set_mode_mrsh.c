/* auto-generated by gen_syscalls.py, don't edit */

#include <syscalls/can.h>

extern int z_vrfy_can_set_mode(const struct device * dev, enum can_mode mode);
uintptr_t z_mrsh_can_set_mode(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
		uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, void *ssf)
{
	_current->syscall_frame = ssf;
	(void) arg2;	/* unused */
	(void) arg3;	/* unused */
	(void) arg4;	/* unused */
	(void) arg5;	/* unused */
	union { uintptr_t x; const struct device * val; } parm0;
	parm0.x = arg0;
	union { uintptr_t x; enum can_mode val; } parm1;
	parm1.x = arg1;
	int ret = z_vrfy_can_set_mode(parm0.val, parm1.val);
	_current->syscall_frame = NULL;
	return (uintptr_t) ret;
}
