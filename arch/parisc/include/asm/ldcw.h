#ifndef __PARISC_LDCW_H
#define __PARISC_LDCW_H

/* Because kmalloc only guarantees 8-byte alignment for kmalloc'd data,
   and GCC only guarantees 8-byte alignment for stack locals, we can't
   be assured of 16-byte alignment for atomic lock data even if we
   specify "__attribute ((aligned(16)))" in the type declaration.  So,
   we use a struct containing an array of four ints for the atomic lock
   type and dynamically select the 16-byte aligned int from the array
   for the semaphore. */

/* From: "Jim Hull" <jim.hull of hp.com>
   I've attached a summary of the change, but basically, for PA 2.0, as
   long as the ",CO" (coherent operation) completer is implemented, then the
   16-byte alignment requirement for ldcw and ldcd is relaxed, and instead
   they only require "natural" alignment (4-byte for ldcw, 8-byte for
   ldcd).

   Although the cache control hint is accepted by all PA 2.0 processors,
   it is only implemented on PA8800/PA8900 CPUs. Prior PA8X00 CPUs still
   require 16-byte alignment. If the address is unaligned, the operation
   of the instruction is undefined. The ldcw instruction does not generate
   unaligned data reference traps so misaligned accesses are not detected.
   This hid the problem for years. So, restore the 16-byte alignment dropped
   by Kyle McMartin in "Remove __ldcw_align for PA-RISC 2.0 processors". */

#define __PA_LDCW_ALIGNMENT	16
#define __PA_LDCW_ALIGN_ORDER	4
#define __ldcw_align(a) ({					\
	unsigned long __ret = (unsigned long) &(a)->lock[0];	\
	__ret = (__ret + __PA_LDCW_ALIGNMENT - 1)		\
		& ~(__PA_LDCW_ALIGNMENT - 1);			\
	(volatile unsigned int *) __ret;			\
})

#ifdef CONFIG_PA20
#define __LDCW	"ldcw,co"
#else
#define __LDCW	"ldcw"
#endif

/* LDCW, the only atomic read-write operation PA-RISC has. *sigh*.
   We don't explicitly expose that "*a" may be written as reload
   fails to find a register in class R1_REGS when "a" needs to be
   reloaded when generating 64-bit PIC code.  Instead, we clobber
   memory to indicate to the compiler that the assembly code reads
   or writes to items other than those listed in the input and output
   operands.  This may pessimize the code somewhat but __ldcw is
   usually used within code blocks surrounded by memory barriors.  */
#define __ldcw(a) ({						\
	unsigned __ret;						\
	__asm__ __volatile__(__LDCW " 0(%1),%0"			\
		: "=r" (__ret) : "r" (a) : "memory");		\
	__ret;							\
})

#ifdef CONFIG_SMP
# define __lock_aligned __attribute__((__section__(".data..lock_aligned")))
#endif

#endif /* __PARISC_LDCW_H */
