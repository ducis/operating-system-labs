#define _POSIX_SOURCE
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

char line_buf[1024];
char *arg_list[512];
char arg_buf[1024];
char *input_redir;
char *output_redir;
unsigned flags;
int dangling_fd;
int demon_pipe[2];
pid_t wait_stack[512];
pid_t *p_wait_stack_top;

#define DONE 1
#define NO_WAIT 2
#define BAIL_OUT 8

#define ERROR_RETURN { flags|=BAIL_OUT; return; }
#define ERROR_BREAK { flags|=BAIL_OUT; break; }
void execute(void){
    int in_fd = dangling_fd;
    int out_fd = -1;
    /*{
        char **p = arg_list;
        printf("excute");
        while(*p){ printf(" ::%s::",*p++); }
        if(input_redir){ printf(" input_redir:%s",input_redir); }
        if(output_redir){ printf(" output_redir:%s",output_redir); }
        printf("\n");
    }*/
    dangling_fd = -1;
    if(!(flags&BAIL_OUT)) {if(input_redir){
        if(0!=in_fd){
            printf( "'<' can only be used on the first command\n" );
            flags|=BAIL_OUT;
        }else{
            assert(0==in_fd);
            in_fd = open(input_redir,O_RDONLY);
            if(-1 == in_fd){
                printf( "failed to redirect input\n" );
                flags|=BAIL_OUT;
            }
        }
    }}
	if(!(flags&BAIL_OUT)) {if(0==in_fd && -1!=demon_pipe[0]){
		in_fd = demon_pipe[0];
		demon_pipe[0] = -1;
	}}
    assert(-1 == out_fd);
    if(!(flags&BAIL_OUT)){ if(output_redir){
        if(flags&DONE){
            out_fd = open(output_redir,O_WRONLY|O_CREAT|O_TRUNC);
            if(-1 == out_fd){
				printf( "failed to redirect output, path = *%s*, length = %d\n",output_redir,strlen(output_redir) );
                flags|=BAIL_OUT;
            }
        }else{
            printf( "'>' can only be used on the last command\n" );
            flags|=BAIL_OUT;
        }
    }else{
        if(flags&DONE){
            out_fd = 1;
        }else{
            int pfd[2];
            if(-1 == pipe(pfd)){
                printf( "failed to pipe()\n" );
                flags|=BAIL_OUT;
            }else{
                assert(-1 == dangling_fd);
                dangling_fd = pfd[0];
                out_fd = pfd[1];
            }
        }
    }}
    assert(1!=in_fd);
    assert((flags & BAIL_OUT)||(-1!=in_fd&&-1!=out_fd));
    if(!(flags&BAIL_OUT)) {
        pid_t pid = fork();
        if(-1 == pid){
            printf( "failed to fork()\n" );
            flags |= BAIL_OUT;
        }else if(!pid){
            if(0!=in_fd){
                close(0);
                if(-1 == dup2(in_fd,0)){
                    printf( "failed to dup2(in_fd,0)\n" );
                    exit(EXIT_FAILURE);
                }else{
                    close(in_fd);
                }
            }
            if(1!=out_fd){
                close(1);
                if(-1 == dup2(out_fd,1)){
                    printf( "failed to dup2(out_fd,1)\n" );
                    exit(EXIT_FAILURE);
                }else{
                    close(out_fd);
                }
            }
			if(demon_pipe[1]!=-1)	close(demon_pipe[1]);
			if(demon_pipe[0]!=-1)	close(demon_pipe[0]);
			if(dangling_fd!=-1)		close(dangling_fd);
            execvp(arg_list[0],arg_list);
			/*{
				char *e = getenv("PWD");
				assert(e);
				execve(arg_list[0],arg_list,e);
			}*/
			printf( "failed to exec()\n" );
			exit(EXIT_FAILURE);
		}else if(!(flags & NO_WAIT)){
			*p_wait_stack_top++ = pid;
		}
    }
    assert((flags & BAIL_OUT)||(-1!=in_fd&&-1!=out_fd));
    if(-1!=in_fd&&0!=in_fd) close(in_fd);
    if(-1!=out_fd&&1!=out_fd) close(out_fd);
}
void interact(void){
    char *itrBuf;
    char **itrList;
    char literal_guard;
	char *itrLineBuf;
	while(!fgets(line_buf,1000,stdin)){
		if(feof(stdin)) return;
		else clearerr(stdin);
	}
	itrLineBuf = line_buf;
	itrBuf = arg_buf;
	itrList = arg_list;
	literal_guard = 0;
    input_redir = 0;
    output_redir = 0;
    flags = 0;
    dangling_fd = 0;
    *itrList = itrBuf;
	p_wait_stack_top = wait_stack;
	demon_pipe[0] = demon_pipe[1] = -1;
	{
		char *p = line_buf;
		while(*p){
			if(literal_guard){
				if(*p==literal_guard){
					literal_guard = 0;
				}
			}else if((flags & NO_WAIT) && *p!=' ' && *p!='\t' && *p!='\n'){
				printf("non-whitespace after '&'\n");
			}else if('&'==*p){
				flags |= NO_WAIT;
				*p = '\0';
				if(pipe(demon_pipe)){
					printf("failed to create demon pipe\n");
					return;
				}
			}
			++p;
		}
		if(literal_guard){
			printf("unfinished literal\n");
			return;
		}
	}
	/*printf("[%d]interact,flags%d,cmdlen%d\n",getpid(),flags,itrBuf-arg_buf);*/
#define END_VAR_LIST    \
    if(*itrList){ \
        if(itrBuf!=*itrList) { \
            assert(itrBuf>*itrList); \
            *itrBuf++ = '\0'; \
            *++itrList = 0; \
        } else{ *itrList = 0; } \
    }
#define EXECUTE if(arg_list[0]){ execute(); input_redir = output_redir = 0; *(itrList = arg_list) = itrBuf = arg_buf; }
	while(!(flags & DONE)){
        int ch;
		/*printf("(");*/
		ch = *itrLineBuf++;
		/*printf(")");*/
		/*printf("[%d]parse%d,flags%d,cmdlen%d\n",getpid(),ch,flags,itrBuf-arg_buf);*/
		if(literal_guard){
		    if(ch == literal_guard){
		        literal_guard = 0;
		    }else{
		        *itrBuf++ = ch;
		    }
		}else switch(ch){
            case '\'':
            case '\"':
                literal_guard = ch;
		    case '\t':
		    case ' ':
                if(itrBuf!=*itrList){
                    *itrBuf++ = '\0';
					*++itrList = itrBuf;
				}
                break;
			case '\0':
            case '\n':
                if(flags & BAIL_OUT) break;
                flags |= DONE;
            case '|':
                END_VAR_LIST;
                EXECUTE;
				/*printf("flagsA%d\n",flags);*/
                break;
		    case '<':
                END_VAR_LIST;
                if(input_redir){
                    printf("too many '<'s\n");
                    ERROR_BREAK;
                }
                input_redir = *++itrList = itrBuf;
                break;
		    case '>':
                END_VAR_LIST;
                if(output_redir){
                    printf("too many '>'s\n");
                    ERROR_BREAK;
                }
                output_redir = *++itrList = itrBuf;
                break;
		    default:
                *itrBuf++ = ch;
		}
		if(flags & BAIL_OUT) break;
		/*printf("flagsB%d\n",flags);*/
	};
    assert((flags & BAIL_OUT)||-1==dangling_fd||0==dangling_fd);
	if(demon_pipe[1]!=-1){
		close(demon_pipe[1]);
		assert(flags & NO_WAIT);
	}
	if(demon_pipe[0]!=-1){
		close(demon_pipe[0]);
		assert(demon_pipe[1]!=-1);
	}
	if(flags & BAIL_OUT){
        if(dangling_fd>0){
            close(dangling_fd);
        }
		/*printf("[%d]bailout,flags%d,cmdlen%d\n",getpid(),flags,itrBuf-arg_buf);*/
	}else if(flags & NO_WAIT){
	}else{
		while(p_wait_stack_top>wait_stack){
			pid_t p = *--p_wait_stack_top;
			while( -1==waitpid(p,0,0) && errno!=ECHILD ){
				assert(EINTR==errno);
			}
		}
    }
}

void handleSIGCHLD(int i){
	/*pid_t p = */wait(0);
	/*assert(p!=-1);*/
	/*printf("child termination\n");*/
}

int main(void){
	{
		/*struct sigaction sa = {&handleSIGCHLD,0,0}; */
		struct sigaction sa;
		memset(&sa,0,sizeof(sa));
		sa.sa_handler = &handleSIGCHLD;
		sigaction(SIGCHLD, &sa, 0);
	}
    while(!feof(stdin)){
        printf("shell: ");
        interact();
    }
	return 0;
}
