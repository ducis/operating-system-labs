
#ifdef __ACK__
.text
begtext:
#ifdef __ACK__
.rom
#else
.data
#endif
begrom:
.data
begdata:
.bss
begbss:
#endif

.extern _getuctx
.extern _setuctx
.extern _resumecontext

/* Offsets into ucontext_t structure. Keep in sync with <sys/ucontext.h>! */
#define UC_FLAGS	0 
#define UC_LINK		UC_FLAGS + 4
#define MCTX		UC_LINK + 4
#define MAGIC		MCTX
#define GS		MAGIC+4
#define FS		GS+2
#define ES		FS+2
#define DS		ES+2
#define DI		DS+2
#define SI		DI+4
#define BP		SI+4
#define ST		BP+4	/* Hole for another SP */
#define BX		ST+4
#define DX		BX+4
#define CX		DX+4
#define AX		CX+4
#define RETADR		AX+4
#define PC		RETADR+4
#define CS		PC+4
#define PSW		CS+4
#define SP		PSW+4
#define SS		SP+4


/* MCF_MAGIC value from <mcontext.h> */
#define MCF_MAGIC	0xc0ffee

/* Values from <sys/ucontext.h> */
#define UCF_IGNFPU	0x002
#define UCF_IGNSIGM	0x004


/* EINVAL from errno.h */
#define EFAULT		14
#define EINVAL 		22

/* int getcontext(ucontext_t *ucp) 
 *	Initialise the structure pointed to by ucp to the current user context
 *	of the calling thread. */


.text

.globl	_getcontext
.balign	16
_getcontext:
	/* In case a process does not use the FPU and is neither interested in
	 * saving its signal mask, then we can skip the context switch to
	 * PM and kernel altogether and only save general-purpose registers. */

	mov (%esp), %ecx	/* Save return address:
				 * When setcontext or swapcontext is called,
				 * we jump to this address and continue
				 * running. */

	mov 4(%esp), %edx		/* edx = ucp */
	/* Check null pointer */
	cmp $0, %edx			/* edx == NULL? */
	jne 3f				/* Not null, continue */
	movl $EFAULT, (_errno) 
	xor %eax, %eax
	dec %eax			/* return -1 */
	ret
	
3:	/* Check flags */
	push %ecx			/* save ecx */
	push %ebx			/* save ebx */
	lea UC_FLAGS(%edx), %ebx	/* ebx = &(ucp->uc_flags) */
	mov (%ebx), %ecx		/* ecx = ucp->uc_flags */
	mov $UCF_IGNFPU, %eax
	or $UCF_IGNSIGM, %eax
	cmp %eax, %ecx			/* is UCF_IGNFPU or UCF_IGNSIGM set? */
	pop %ebx			/* restore ebx */
	pop %ecx			/* restore ecx */
	jz 1f				/* Both are set, skip getuctx */

0:
	push %ecx			/* Save ecx */
	push %edx		 
	call _getuctx			/* getuctx(ucp) */
	pop %edx			/* clean up stack and restore edx */
	pop %ecx			/* Restore ecx */

1: 
	/* Save the context */
	mov 4(%esp), %edx		/* edx = ucp */
	pop %eax			/* retaddr */
	mov %eax, PC(%edx)		/* Save real RTA in mcp struct */
	mov %esp, SP(%edx)	/* Save stack pointer (now pointing to ucp) */
	/* Save GP registers */
	mov %ebp, BP(%edx)		/* Save EBP */
	mov %esi, SI(%edx)		/* Save ESI */
	mov %edi, DI(%edx)		/* Save EDI */
	mov %ebx, BX(%edx)		/* Save EBX */
	mov %ecx, CX(%edx)		/* Save ECX */
	movl $MCF_MAGIC, MAGIC(%edx)	/* Set magic value */
	push %eax			/* Restore retaddr */

	xor %eax, %eax			/* Return 0 */

2:	
	add $4, %esp			/* Remove stale (setcontext) RTA */
	jmp *%ecx			/* Restore return address */
	

/* int setcontext(const ucontext_t *ucp)
 *	Restore the user context pointed to by ucp. A successful call to
 *	setcontext does not return; program execution resumes at the point
 *	specified by the ucp argument. If ucp was created with getcontext(), 
 *	program execution continues as if the corresponding call of getcontext()
 *	had just returned. If ucp was created with makecontext(), program
 *	execution continues with the function passed to makecontext(). */
 
.text

.globl	_setcontext
.balign	16
_setcontext:
	/* In case a process does not use the FPU and is neither interested in
	 * restoring its signal mask, then we can skip the context switch to
	 * PM and kernel altogether and restore state here. */

	mov 4(%esp), %edx		/* edx = ucp */

	/* Check null pointer */
	cmp $0, %edx			/* edx == NULL? */
	jnz 3f				/* Not null, continue */
	movl $EFAULT, (_errno)
	xor %eax, %eax
	dec %eax			/* return -1 */
	ret
	
3:	/* Check flags */
	push %ebx			/* save ebx */
	lea MAGIC(%edx), %ebx		/* ebx = &(ucp->mc_context.mc_magic) */
	mov (%ebx), %ecx		/* ecx = ucp->mc_context.mc_magic */
	pop %ebx			/* restore ebx */
	cmp $MCF_MAGIC, %ecx	/* is the magic value set (is context valid)?*/
	jz 4f				/* is set, proceed */
	movl $EINVAL, (_errno)		/* not set, return error code */
	xor %eax, %eax
	dec %eax			/* return -1 */
	ret	


4:	push %ebx			/* save ebx */
	lea UC_FLAGS(%edx), %ebx 	/* ebx = &(ucp->uc_flags) */
	mov (%ebx), %ecx		/* ecx = ucp->uc_flags */
	pop %ebx			/* restore ebx */
	mov $UCF_IGNFPU, %eax
	or $UCF_IGNSIGM, %eax
	cmp %eax, %ecx		/* Are UCF_IGNFPU and UCF_IGNSIGM flags set? */
	jz 1f			/* Both are set, so don't bother restoring FPU
				 * state and signal mask */

0:	push %ecx			/* Save ecx */
	push %edx		 
	call _setuctx			/* setuctx(ucp) */
	pop %edx			/* Clean up stack and restore edx */
	pop %ecx			/* Restore ecx */

1:	/* Restore the registers */
	mov 4(%esp), %edx		/* edx = ucp */
	mov CX(%edx), %ecx		/* Restore ECX */
	mov BX(%edx), %ebx		/* Restore EBX */
	mov DI(%edx), %edi		/* Restore EDI */
	mov SI(%edx), %esi		/* Restore ESI */
	mov BP(%edx), %ebp		/* Restore EBP */
	mov SP(%edx), %esp		/* Restore stack pointer */

2:
	jmp *PC(%edx)  	/* Push RTA onto stack so we can return to it */


/* void ctx_start((void *func)(int arg1, ..., argn), arg1, ..., argn,
 *		  ucontext_t *ucp)
 *	A wrapper to start function `func'. ESI register will contain a pointer
 *	to ucp on the stack. By setting ESP to ESI, we effectively 'remove' all
 *	arguments to `func' from the stack. Finally, a call to resumecontext
 *	will start the next context in the linked list (or exit the program if
 *	there is no context). */

.text

.globl	_ctx_start
.balign	16
_ctx_start:
	/* 0(esp) -> func
	 * 4(esp) -> arg1
	 * ...
	 * 4*n(esp) -> argn
	 * 4*(n+1)(esp) -> ucp */

	pop %eax			/* eax = func */
	call *%eax			/* func(arg1, ..., argn) */
	mov %esi, %esp			/* Clean up stack */
	/* ucp is now at the top of the stack again */
	call _resumecontext	/* resumecontext(ucp) */
	ret			/* never reached */


