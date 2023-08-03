/*
 * tsh - A tiny shell program with job control
 *
 * <Put your name and login ID here>
 */
#include <bits/types/sigset_t.h>
// #include <cstddef>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
int nextjid = 1;         /* next job ID to allocate */
char sbuf[MAXLINE];      /* for composing sprintf messages */

struct job_t {           /* The job struct */
  pid_t pid;             /* job PID */
  int jid;               /* job ID [1, 2, ...] */
  int state;             /* UNDEF, BG, FG, or ST */
  char cmdline[MAXLINE]; /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

void Execve(const char *__path, char *const *__argv, char *const *__envp) {
  if (execve(__path, __argv, __envp) < 0) {
    printf("%s: Command not found\n", __argv[0]);
  }
}

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv) {
  char c;
  char cmdline[MAXLINE];
  int emit_prompt = 1; /* emit prompt (default) */

  /* Redirect stderr to stdout (so that driver will get all output
   * on the pipe connected to stdout) */
  dup2(1, 2);

  /* Parse the command line */
  while ((c = getopt(argc, argv, "hvp")) != EOF) {
    switch (c) {
    case 'h': /* print help message */
      usage();
      break;
    case 'v': /* emit additional diagnostic info */
      verbose = 1;
      break;
    case 'p':          /* don't print a prompt */
      emit_prompt = 0; /* handy for automatic testing */
      break;
    default:
      usage();
    }
  }

  /* Install the signal handlers */

  /* These are the ones you will need to implement */
  Signal(SIGINT, sigint_handler);   /* ctrl-c */
  Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
  Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */

  /* This one provides a clean way to kill the shell */
  Signal(SIGQUIT, sigquit_handler);

  /* Initialize the job list */
  initjobs(jobs);

  /* Execute the shell's read/eval loop */
  while (1) {
    /* Read command line */
    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }
    if ((fgets(cmdline, MAXLINE, stdin) == NULL)&& ferror(stdin) ){
      app_error("fgets error");
    }
    if (feof(stdin)) { /* End of file (ctrl-d) */
      fflush(stdout);
      exit(0);
    }
    /* Evaluate the command line */
    eval(cmdline);
    fflush(stdout);
    fflush(stdout);
  }

  exit(0); /* control never reaches here */
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
 */
void eval(char *cmdline) {
  // 首先根据cmdline，解析这一行命令。由于我们在解析的函数中修改了传入的cmdline，所以用了一个备份的buffer。
  // 同时解析函数的返回值其实就代表了当前这个被命令行启动的进程是前天还是后台运行的
  // 但是其实好像不备份也行，留坑！
  char buffer[MAXLINE];
  strcpy(buffer, cmdline);
  char *argv[MAXARGS];
  int is_bg = parseline(buffer, argv);
  // 题目已经定义为bg和fg，所以按照题目意思来定义状态
  int state = is_bg ? BG : FG;
  // 如果解析出来是个空的命令行，则直接return
  if (argv[0] == NULL) {
    return;
  }
  // 下面是核心操作部分
  // 创建信号集并初始化，分别是不理会所有信号，不理会一个信号（SIGCHLD），和备份之前的信号
  sigset_t all_mask, one_mask, pre_mask;
  sigfillset(&all_mask);
  sigemptyset(&one_mask);
  sigaddset(&one_mask, SIGCHLD);
  // 如果即将创建的进程不是系统程序，那下面这个函数会返回0
  // 如果是系统程序，那这个函数内部就直接执行了， 不需要我们管了
  // 因此，我们只需要处理非系统程序的情况
  pid_t pid;
  if (!builtin_cmd(argv)) {
    // 在调用fork之前就要阻塞SIGCHLD信号
    sigprocmask(SIG_BLOCK, &one_mask, &pre_mask);
    if ((pid = fork()) == 0) {
      // 子进程
      // 子进程首先解除从父进程那继承来的阻塞信号
      sigprocmask(SIG_SETMASK, &pre_mask, NULL);
      // 修改自己的组id为自己的id
      setpgid(0, 0);
      // 创建真正的进程，其中environ是在libc文件中定义的
      Execve(argv[0], argv, environ);
      // 真正的进程执行完之后，子进程也就完成使命了，使用exit终止进程，更加强劲！
      exit(0);
    }
    // 父进程继续操作
    // 首先获取全局锁，因为要修改全局变量了
    sigprocmask(SIG_BLOCK, &all_mask, NULL);
    addjob(jobs, pid, state, cmdline);
    sigprocmask(SIG_SETMASK, &one_mask, NULL);
    // 如果是前台进程，则父进程要阻塞到这个前台进程结束
    // 如果是后台进程，则父进程打印这个后台进程的信息
    if (state == FG) {
      waitfg(pid);
    } else {
      printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
    }
    // 父进程ok了，可以去接受SIGCHLD信号
    sigprocmask(SIG_SETMASK, &pre_mask, NULL);
  }
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 */
int parseline(const char *cmdline, char **argv) {
  static char array[MAXLINE]; /* holds local copy of command line */
  char *buf = array;          /* ptr that traverses command line */
  char *delim;                /* points to first space delimiter */
  int argc;                   /* number of args */
  int bg;                     /* background job? */

  strcpy(buf, cmdline);
  buf[strlen(buf) - 1] = ' ';   /* replace trailing '\n' with space */
  while (*buf && (*buf == ' ')) /* ignore leading spaces */
    buf++;

  /* Build the argv list */
  argc = 0;
  if (*buf == '\'') {
    buf++;
    delim = strchr(buf, '\'');
  } else {
    delim = strchr(buf, ' ');
  }

  while (delim) {
    argv[argc++] = buf;
    *delim = '\0';
    buf = delim + 1;
    while (*buf && (*buf == ' ')) /* ignore spaces */
      buf++;

    if (*buf == '\'') {
      buf++;
      delim = strchr(buf, '\'');
    } else {
      delim = strchr(buf, ' ');
    }
  }
  argv[argc] = NULL;

  if (argc == 0) /* ignore blank line */
    return 1;

  /* should the job run in the background? */
  if ((bg = (*argv[argc - 1] == '&')) != 0) {
    argv[--argc] = NULL;
  }
  return bg;
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char **argv) {
  // 总共要处理 quit bg fg jobs，并&
  if (!strcmp(argv[0], "quit")) {
    // quit指令，直接终止
    exit(0);
  }
  if (!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg")) {
    // 执行bg或者fg
    do_bgfg(argv);
    return 1;
  }
  if (!strcmp(argv[0], "jobs")) {
    // 列出jobs
    listjobs(jobs);
    return 1;
  }
  return 0;
}

/*
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) {
  // 后面要用到
  struct job_t *job = NULL;

  // 这个函数需要处理多种输入，包括一些非法输入
  // 首先是确认到底是bg操作还是fg操作
  int state = (strcmp(argv[0], "bg") == 0) ? BG : FG;

  // 然后判断是否给出了具体的pid或者jid，如果没有给出正确的参数，那要给出提示
  // 首先看看是否有参数
  if (argv[1] == NULL) {
    printf("%s command requires PID or %%jobid argument\n", argv[0]);
    return;
  }
  // 有参数，那就先看看是不是jid
  if (argv[1][0] == '%') {
    // 使用sscanf尝试获取jid
    int jid;
    if (sscanf(&argv[1][1], "%d", &jid) > 0) {
      job = getjobjid(jobs, jid);
      // 如果getjobjid返回null，则说明没有这个job
      if (job == NULL) {
        printf("%%%d: No such job\n", jid);
        return;
      }
    }
    // 到了这里，那肯定有参数，并且不是jid，但是也有可能是瞎输入的先排除一下
  } else if (!isdigit(argv[1][0])) {
    printf("%s: argument must be a PID or %%jobid\n", argv[0]);
    return;
  } else {
    // 肯定输入的是pid了
    pid_t pid;
    if (sscanf(argv[1], "%d", &pid) > 0) {
      job = getjobpid(jobs, pid);
      if (job == NULL) {
        printf("(%d): No such process\n", pid);
        return;
      }
    }
  }

  // 如果能够走到这里，那么说明已经正确取到对应的job了
  // 唤醒这个job，修改状态
  // 这里没有使用进程组，留坑！
  kill(-job->pid, SIGCONT);
  job->state = state;
  // 根据bg或者fg进行特定的操作
  if (state == BG) {
    printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
  } else {
    waitfg(job->pid);
  }
}

/*
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid) {
  // 注意，进入这个函数的时候，父进程已经阻塞了子进程可能传来的SIGCHLD信号
  // 而我们使用的sigsuspend函数会让父进程进入一个完全没有阻塞信号的状态
  // 因此，只要有一个子进程挂了，父进程就会跳出阻塞去检查一下是否还有前台进程
  // 感觉有点问题，这里实现的好像太严格了，不只考虑了pid，还考虑所有前台进程，留坑
  sigset_t none_mask;
  sigemptyset(&none_mask);
  while (fgpid(jobs) != 0) {
    sigsuspend(&none_mask);
  }
}

/*****************
 * Signal handlers
 *****************/

/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.
 */
void sigchld_handler(int sig) {
  int olderr = errno;
  pid_t pid;
  int state;
  sigset_t all_mask, pre_mask;
  sigfillset(&all_mask);
  // 不断等待子进程，直到收到一个子进程停止的消息
  while ((pid = waitpid(-1, &state, WNOHANG | WUNTRACED)) > 0) {
    // 因为接下来需要操作jobs这个全局变量，先屏蔽所有的信号
    sigprocmask(SIG_SETMASK, &all_mask, &pre_mask);
    // 如果是正常终止，则只需要删除对应的job
    if (WIFEXITED(state)) {
      deletejob(jobs, pid);
      // 如果是被信号杀死，则还需要输出被哪个信号杀死
    } else if (WIFSIGNALED(state)) {
      deletejob(jobs, pid);
      printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid,
             WTERMSIG(state));
      // 如果是被信号暂停了，还需要修改对应job的状态
    } else if (WIFSTOPPED(state)) {
      struct job_t *job = getjobpid(jobs, pid);
      job->state = ST;
      printf("Job [%d] (%d) stoped by signal %d\n", pid2jid(pid), pid,
             WSTOPSIG(state));
    }
    sigprocmask(SIG_SETMASK, &pre_mask, NULL);
  }

  errno = olderr;
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig) {
  // 前台进程只可能有一个，只要还存在前台进程，那就把这个前台进程给杀掉
  // 感觉这里不加信号量也没问题
  int olderr = errno;
  pid_t pid;
  if ((pid = fgpid(jobs)) != 0) {
    kill(-pid, SIGINT);
  }
  errno = olderr;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig) {
  int olderr = errno;
  pid_t pid;
  if ((pid = fgpid(jobs)) != 0) {
    kill(-pid, SIGSTOP);
  }
  errno = olderr;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
  job->pid = 0;
  job->jid = 0;
  job->state = UNDEF;
  job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
  int i;
  for (i = 0; i < MAXJOBS; i++)
    clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) {
  int i, max = 0;

  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].jid > max)
      max = jobs[i].jid;
  return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) {
  int i;

  if (pid < 1)
    return 0;

  for (i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == 0) {
      jobs[i].pid = pid;
      jobs[i].state = state;
      jobs[i].jid = nextjid++;
      if (nextjid > MAXJOBS)
        nextjid = 1;
      strcpy(jobs[i].cmdline, cmdline);
      if (verbose) {
        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid,
               jobs[i].cmdline);
      }
      return 1;
    }
  }
  printf("Tried to create too many jobs\n");
  return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) {
  int i;

  if (pid < 1)
    return 0;

  for (i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == pid) {
      clearjob(&jobs[i]);
      nextjid = maxjid(jobs) + 1;
      return 1;
    }
  }
  return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
  int i;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].state == FG)
      return jobs[i].pid;
  return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
  int i;

  if (pid < 1)
    return NULL;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].pid == pid)
      return &jobs[i];
  return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) {
  int i;

  if (jid < 1)
    return NULL;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].jid == jid)
      return &jobs[i];
  return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) {
  int i;

  if (pid < 1)
    return 0;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].pid == pid) {
      return jobs[i].jid;
    }
  return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) {
  int i;

  for (i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid != 0) {
      printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
      switch (jobs[i].state) {
      case BG:
        printf("Running ");
        break;
      case FG:
        printf("Foreground ");
        break;
      case ST:
        printf("Stopped ");
        break;
      default:
        printf("listjobs: Internal error: job[%d].state=%d ", i, jobs[i].state);
      }
      printf("%s", jobs[i].cmdline);
    }
  }
}
/******************************
 * end job list helper routines
 ******************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) {
  printf("Usage: shell [-hvp]\n");
  printf("   -h   print this message\n");
  printf("   -v   print additional diagnostic information\n");
  printf("   -p   do not emit a command prompt\n");
  exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg) {
  fprintf(stdout, "%s: %s\n", msg, strerror(errno));
  exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg) {
  fprintf(stdout, "%s\n", msg);
  exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) {
  struct sigaction action, old_action;

  action.sa_handler = handler;
  sigemptyset(&action.sa_mask); /* block sigs of type being handled */
  action.sa_flags = SA_RESTART; /* restart syscalls if possible */

  if (sigaction(signum, &action, &old_action) < 0)
    unix_error("Signal error");
  return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) {
  printf("Terminating after receipt of SIGQUIT signal\n");
  exit(1);
}
