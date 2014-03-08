#include <stdio.h>

#include "syslib.h"

#define CHECKEUID if(geteuid()){ return EPERM; }

PUBLIC int sys_p2_set_policy(int policy){
	message m;
	CHECKEUID;
	m.VM_P2_OP = VM_P2_SET_POLICY;
	m.VM_P2_POLICY = policy;
	return(_syscall(VM_PROC_NR, VM_P2, &m));
}

PUBLIC int sys_p2_get_stat(vm_p2_stats *out){
	message m;
	CHECKEUID;
	m.VM_P2_ENDPT = getprocnr();
	m.VM_P2_OP = VM_P2_GET_STAT;
	m.VM_P2_BUF = (char*)out;
	/*fprintf(stdout,"sys_p2_get_stat() %d\n",m.VM_P2_ENDPT);*/
	return(_syscall(VM_PROC_NR, VM_P2, &m));
}

PUBLIC int sys_p2_echo(void){
	message m;
	CHECKEUID;
	m.VM_P2_OP = VM_P2_ECHO;
	return(_syscall(VM_PROC_NR, VM_P2, &m));
}

PUBLIC int sys_p2_save_bitmap(char *buf, int buflen){
	message m;
	CHECKEUID;
	m.VM_P2_ENDPT = SELF;
	m.VM_P2_OP = VM_P2_SAVE_BITMAP;
	m.VM_P2_BUF = buf;
	m.VM_P2_BUFLEN = buflen;
	return(_syscall(VM_PROC_NR, VM_P2, &m));
}
