/* Type definitions used in RS.
 */
#ifndef RS_TYPE_H
#define RS_TYPE_H

/* Definition of an entry of the boot image priv table. */
struct boot_image_priv {
  endpoint_t endpoint;         /* process endpoint number */
  char label[RS_MAX_LABEL_LEN]; /* label to assign to this service */

  int flags;                   /* privilege flags */
  short trap_mask;             /* allowed system call traps */
  int ipc_to;                  /* send mask protection */
  endpoint_t sig_mgr;          /* signal manager */
  int *k_calls;                /* allowed kernel calls */
  int *vm_calls;               /* allowed vm calls */
  long period;                 /* heartbeat period (or zero) */
};

/* Definition of an entry of the boot image sys table. */
struct boot_image_sys {
  endpoint_t endpoint;         /* process endpoint number */

  int flags;                   /* system flags */
};

/* Definition of an entry of the boot image dev table. */
struct boot_image_dev {
  endpoint_t endpoint;         /* process endpoint number */

  int flags;                   /* device flags */
  dev_t dev_nr;                /* major device number */
  int dev_style;               /* device style */
  int dev_style2;              /* device style for next major device number */
};

/* Definition of an entry of the system process table. */
struct rproc {
  struct rprocpub *r_pub;       /* pointer to the corresponding public entry */
  struct rproc *r_old_rp;       /* pointer to the slot with the old version */
  struct rproc *r_new_rp;       /* pointer to the slot with the new version */
  struct rproc *r_prev_rp;      /* pointer to the slot with the prev replica */
  struct rproc *r_next_rp;      /* pointer to the slot with the next replica */
  pid_t r_pid;			/* process id, -1 if the process is not there */

  int r_restarts;		/* number of restarts (initially zero) */
  long r_backoff;		/* number of periods to wait before revive */
  unsigned r_flags; 		/* status and policy flags */

  clock_t r_check_tm;		/* timestamp of last check */
  clock_t r_alive_tm;		/* timestamp of last heartbeat */
  clock_t r_stop_tm;		/* timestamp of SIGTERM signal */
  endpoint_t r_caller;		/* RS_LATEREPLY caller */
  int r_caller_request;		/* RS_LATEREPLY caller request */

  char r_cmd[MAX_COMMAND_LEN];	/* raw command plus arguments */
  char r_args[MAX_COMMAND_LEN];	/* null-separated raw command plus arguments */
  char *r_argv[MAX_NR_ARGS+2];  /* parsed arguments vector */
  int r_argc;  			/* number of arguments */
  char r_script[MAX_SCRIPT_LEN]; /* name of the restart script executable */

  char *r_exec;			/* Executable image */ 
  size_t r_exec_len;		/* Length of image */

  int r_set_resources;		/* set when resources must be set. */
  struct priv r_priv;		/* Privilege structure to be passed to the
				 * kernel.
				 */
  uid_t r_uid;
  int r_nice;

  char r_ipc_list[MAX_IPC_LIST];
  int r_nr_control;
  char r_control[RS_NR_CONTROL][RS_MAX_LABEL_LEN];
};

/* Definition of the global update descriptor. */
struct rupdate {
  int flags;               /* flags to keep track of the status of the update */
  clock_t prepare_tm;      /* timestamp of when the update was scheduled */
  clock_t prepare_maxtime; /* max time to wait for the process to be ready */
  struct rproc *rp;        /* the process under update */
};

#endif /* RS_TYPE_H */
