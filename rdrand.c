/*-
 * Copyright (c) 2017 Luis Alberto Gonzalez 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: releng/11.1/sys/dev/null/null.c 274366 2014-11-11 04:48:09Z pjd $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/disk.h>
//#include <sys/bus.h>
#include <sys/filio.h>

#include <machine/bus.h>
#include <machine/vmparam.h>


#define RETRY_COUNT 10
#define RDRAND_MAX_BUFFER 1024*1024

/* For use with destroy_dev(9). */
static struct cdev *rdrand_dev;
static d_read_t rdrand_read;
static d_ioctl_t rdrand_ioctl;


static struct cdevsw rdrand_cdevsw = {
	.d_version = 	D_VERSION,
	.d_read =	rdrand_read,
	.d_write =	(d_write_t*)nullop,
	.d_ioctl =	rdrand_ioctl,
	.d_name	=	"rdrand",
};

/* ARGSUSED */

static int
rdrand_ioctl(struct cdev *dev __unused, u_long cmd, caddr_t data __unused,
	   int flags __unused, struct thread *td)
{
	int error = 0;

	switch (cmd) {
	case FIONBIO:
		break;
	case FIOASYNC:
		if (*(int *)data != 0)
			error = EINVAL;
		break;
	default:
		error = ENOIOCTL;
	}
	return (error);
}

static inline int
ivy_rng_store(u_long *buf)
{
#ifdef __GNUCLIKE_ASM
	u_long rndval;
	int retry;

	retry = RETRY_COUNT;
	__asm __volatile(
	    "1:\n\t"
	    "rdrand	%1\n\t"	/* read randomness into rndval */
	    "jc		2f\n\t" /* CF is set on success, exit retry loop */
	    "dec	%0\n\t" /* otherwise, retry-- */
	    "jne	1b\n\t" /* and loop if retries are not exhausted */
	    "2:"
	    : "+r" (retry), "=r" (rndval) : : "cc");
	*buf = rndval;
	return (retry);
#else /* __GNUCLIKE_ASM */
	return (0);
#endif
}


/* ARGSUSED */
static int 
rdrand_read(struct cdev *dev __unused, struct uio *uio,int flags __unused)
{
	int len_ulong = 0;
	//unsigned char buffer[RDRAND_MAX_BUFFER];
	u_long rdrandval;
	ssize_t len;
	//ssize_t offset;
	int error = 0;
	len_ulong = sizeof(u_long);	
	while(uio->uio_resid > 0 && error == 0)	{
		ivy_rng_store(&rdrandval);
		len =  uio->uio_resid;
		if( len > len_ulong)
			len = len_ulong;
		error = uiomove(&rdrandval, len, uio);
	}
	return (error);
}

/* ARGSUSED */
static int
devrdrand_modevent(module_t mod __unused, int type, void *data __unused)
{
	switch(type) {
	case MOD_LOAD:
		if (bootverbose)
			printf("rdrand: < Intel rdrand device>\n");

		rdrand_dev = make_dev_credf(MAKEDEV_ETERNAL_KLD, &rdrand_cdevsw,0,
		    NULL, UID_ROOT, GID_WHEEL, 0666, "rdrand");
		break;

	case MOD_UNLOAD:

		destroy_dev(rdrand_dev);
		break;

	case MOD_SHUTDOWN:
		break;

	default:
		return (EOPNOTSUPP);
	}

	return (0);
}

DEV_MODULE(devrdrand, devrdrand_modevent, NULL);
MODULE_VERSION(devrdrand, 1);
