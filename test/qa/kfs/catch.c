/*
 * catch Bad Signals and be informative
 */
#define _GNU_SOURCE
/*
#define DO_DL
*/
#undef CATCH_TEST

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

#include <signal.h>
#include <ucontext.h>
#include <sys/reg.h>

#ifdef DO_DL
#include <dlfcn.h>
#endif

#include <string.h>
#include <errno.h>

static void
catcher(int sig, siginfo_t *info, void *arg)
{
#ifdef DO_DL
  Dl_info dli;
#endif
  greg_t	pc, sp;
  ucontext_t *uc = arg;

  printf("\nSIG %d ERRNO %d CODE %d\n", sig, info->si_errno, info->si_code);

  if (sig == SIGSEGV) {
    printf("Segmentation Fault: @(%p) ", info->si_addr);
    switch(info->si_code){
    case SEGV_MAPERR: printf("Address not mapped to object\n"); break;
    case SEGV_ACCERR: printf("Invalid permissions\n"); break;
    default: printf("Unknown code\n"); break;
    }
  } else if (sig == SIGBUS) {
    printf("Bus Error: @(%p) ", info->si_addr);
    switch(info->si_code){
    case BUS_ADRALN: printf("Invalid address alignment\n"); break;
    case BUS_ADRERR: printf("Invalid physical address\n"); break;
    case BUS_OBJERR: printf("Object specific hardware error\n"); break;
    default: printf("Unknown code\n"); break;
    }
  }

#if defined(sparc)
  pc = uc->uc_mcontext.gregs[PC];
  sp = uc->uc_mcontext.gregs[SP];
#elif defined(__i386__)
  pc = uc->uc_mcontext.gregs[REG_EIP];
  sp = uc->uc_mcontext.gregs[REG_ESP];
#elif defined(__x86_64__)
  pc = uc->uc_mcontext.gregs[REG_RIP];
  sp = uc->uc_mcontext.gregs[REG_RSP];
#else
#error "I don't understand your architecture"
#endif

  printf("\nPC 0x%08lx SP 0x%08lx ", (long)pc, (long)sp);

#ifdef DO_DL
  if (dladdr((void *)pc, &dli))
    printf("(%s+0x%lx)\n", dli.dli_sname, (long)((void *)pc - dli.dli_saddr));
  else
#endif
    printf("(no symbol)\n");
  exit(sig);
}

void
catchme(void)
{
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = catcher;
  if (sigaction(SIGSEGV, &sa, 0) || sigaction(SIGBUS, &sa, 0))
    printf("sigaction %s\n", strerror(errno));
}

#ifdef CATCH_TEST
int
main(int argc, char **argv)
{
  catchme();
  printf("in\n");
  printf("die %s\n", (char *)2);
  exit(0);
}
#endif
