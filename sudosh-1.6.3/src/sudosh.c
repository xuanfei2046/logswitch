/*

sudosh - sudo shell that supports input and output logging to syslog

Copyright 2004 and $Date: 2004/10/25 05:18:29 $
		Douglas Richard Hanks Jr.

Licensed under the Open Software License version 2.0

This program is free software; you can redistribute it
and/or modify it under the terms of the Open Software
License, version 2.0 by Lauwrence E. Rosen.

This program is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the Open Software License for details.

*/

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>

#include "config.h"

#ifdef TIME_WITH_SYS_TIME
#include <time.h>
#include <sys/time.h>
#else
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#endif

#ifdef HAVE_PTY_H
#include <pty.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#ifdef HAVE_SYS_TERMIO_H
#include <sys/termio.h>
#endif

#ifdef HAVE_USERSEC_H
#include <usersec.h>
#endif

#ifndef SIGCHLD
#define SIGCHLD	SIGCLD
#endif

#ifndef SUDOSHLOGINDIR
#define SUDOSHLOGINDIR	"/bin"
#endif
#define LOGINSCRIPT SUDOSHLOGINDIR "/sudosh-login"


#define WRITE(a, b, c) do_write(a, b, c, __FILE__, __LINE__)

static struct termios termorig;
static struct winsize winorig;

struct pst {
    char *master;
    char *slave;
    int mfd;
    int sfd;
} pspair;

struct s_file {
    int fd;
    int bytes;
    struct stat stat;
    struct stat cstat;
    struct stat tstat;
    char name[BUFSIZ];
    char str[BUFSIZ];
};

struct s_file script;
struct s_file input;
struct s_file timing;

struct s_env {
    char str[BUFSIZ];
    char *ptr;
};

struct s_user {
    char to[BUFSIZ];
    char from[BUFSIZ];
    char *vshell;
    struct s_env home;
    struct s_env to_home;
    struct s_env term;
    struct s_env path;
    struct s_env mail;
    struct s_env shell;
    struct s_env logname;
    struct passwd *pw;
};

struct s_user user;

static char *progname;
char start_msg[BUFSIZ];

int init = 0;
int loginshell = 0;

static void bye(int);
static void newwinsize(int);
static void prepchild(struct pst *);
static void rawmode(int);
static int findms(struct pst *);
void mysyslog(int, const char *, ...);
void mklogdir(void);
char *rand2str(size_t len);

int main(int argc, char *argv[], char *environ[])
{
    int n = 1;
    int valid = -1;
    unsigned char iobuf[BUFSIZ];
    char *p = NULL;
    char *rand = rand2str(16);
    time_t now;
    struct stat s;
    struct sigaction sa;
    struct timeval tv;
    double oldtime, newtime;
    pid_t pid = getpid();
    struct stat ttybuf;

    user.vshell = NULL;
    user.shell.ptr = NULL;
    user.home.ptr = NULL;
    user.term.ptr = NULL;

    progname = argv[0];

    if ((p = (char *) strrchr(progname, '/')) != NULL)
	progname = p + 1;

	if(*progname == '-')
		loginshell = 1;

    /* Who are you? */
    user.pw = getpwuid((uid_t) geteuid());

    if (user.pw == NULL) {
	fprintf(stderr, "I do not know who you are.  Stopping.\n");
	perror("getpwuid");
	exit(errno);
    }

    strncpy(user.to, user.pw->pw_name, BUFSIZ - 1);

    user.term.ptr = getenv("TERM");

    if (user.term.ptr == NULL)
	user.term.ptr = "dumb";

    if (strlen(user.term.ptr) < 1)
	user.term.ptr = "dumb";

    snprintf(user.home.str, BUFSIZ - 1, "HOME=%s", user.pw->pw_dir);
    strncpy(user.to_home.str, user.pw->pw_dir, BUFSIZ - 1);
    snprintf(user.term.str, BUFSIZ - 1, "TERM=%s", user.term.ptr);

    if (ttyname(0) != NULL) {
	if (stat(ttyname(0), &ttybuf) == 0) {
	    if ((getpwuid(ttybuf.st_uid)->pw_name) == NULL) {
		fprintf(stderr, "I have no idea who you are.\n");
		exit(255);
	    }
	    strncpy(user.from, getpwuid(ttybuf.st_uid)->pw_name,
		    BUFSIZ - 1);
	} else {
	    fprintf(stderr, "Couldn't stat %s\n", ttyname(0));
	    exit(255);
	}
    } else {
	fprintf(stderr, "Couldn't get your controlling terminal.\n");
	exit(255);
    }

    if (argc >= 2) {
	if (!strcmp(argv[1], "-V") ||
	    !strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")) {
	    fprintf(stdout, "%s version %s\n", PACKAGE_NAME, VERSION);
	    exit(0);
	} else if (!strcmp(argv[1], "-i") || !strcmp(argv[1], "--init")) {
	    init = 1;
	    mklogdir();
	    exit(0);
	} else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
	    fprintf(stdout,
		    "Usage: sudosh\n"
		    "sudo shell that supports input and output logging to syslog\n"
		    "\n"
		    "-h, --help	display this help and exit\n"
		    "-i, --init	initialize LOGDIR (mkdir and chmod)\n"
		    "-v, --version	output version information and exit\n"
		    "\n" "Report bugs to <%s>\n", PACKAGE_BUGREPORT);
	    exit(0);
	} else {
	    fprintf(stderr, "sudosh: unrecognized option `%s'\n",
		    *(argv + 1));
	    fprintf(stderr, "Try `sudosh --help' for more information.\n");
	    exit(1);
	}
    }

    mklogdir();

#ifdef HAVE_GETUSERSHELL
    if ((user.shell.ptr = getenv("SHELL")) == NULL)
	user.shell.ptr = user.pw->pw_shell;

    /* check against /etc/shells to make sure it's a real shell */
    setusershell();
    while (user.vshell = (char *) getusershell()) {
	if (strcmp(user.shell.ptr, user.vshell) == 0)
	    valid = 1;
    }
    endusershell();

    if (valid != 1) {
	if (user.shell.ptr == NULL) {
	    fprintf(stderr, "Could not determine a valid shell.\n");
	    mysyslog(LOG_ERR, "Could not determine a valid shell");
	    exit(200);
	} else {
	    fprintf(stderr, "%s is not in /etc/shells\n", user.shell.ptr);
	    mysyslog(LOG_ERR, "%s,%s: %s is not in /etc/shells",
		     user.from, ttyname(0), user.shell.ptr);
	    exit(210);
	}
    }

    if (stat((const char *) user.shell.ptr, &s) == -1) {
	fprintf(stderr, "Shell %s doesn't exist.\n", user.shell.ptr);
	mysyslog(LOG_ERR, "%s,%s: shell %s doesn't exist.",
		 user.from, ttyname(0), user.shell.ptr);
	exit(255);
    }
#else
    user.shell.ptr = user.pw->pw_shell;
#endif				/* HAVE_GETUSERSHELL */

	if(loginshell)
		user.shell.ptr = DEFSHELL;

    now = time((time_t *) NULL);

    script.bytes = 0;
    timing.bytes = 0;
    input.bytes = 0;

    snprintf(script.name, (size_t) BUFSIZ - 1, "%s/%s%s%s%sscript%s%i%s%s", LOGDIR, user.from, FILEDELIMIT, user.to, FILEDELIMIT, FILEDELIMIT, now, FILEDELIMIT, rand);
    snprintf(timing.name, (size_t) BUFSIZ - 1, "%s/%s%s%s%stime%s%i%s%s", LOGDIR, user.from, FILEDELIMIT, user.to, FILEDELIMIT, FILEDELIMIT, now, FILEDELIMIT, rand);
    snprintf(input.name, (size_t) BUFSIZ - 1, "%s/%s%s%s%sinput%s%i%s%s", LOGDIR, user.from, FILEDELIMIT, user.to, FILEDELIMIT, FILEDELIMIT, now, FILEDELIMIT, rand);

    snprintf(start_msg, BUFSIZ - 1, "starting session for %s as %s,%s (%s)", user.from, user.to, ttyname(0), user.shell.ptr);

    if ((script.fd = open(script.name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1) {
	perror(script.name);
	bye(1);
    }

    if (fstat(script.fd, &script.stat) == -1) {
	perror("fstat script.fd");
	exit(errno);
    }

    if ((timing.fd = open(timing.name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1) {
	perror(timing.name);
	bye(1);
    }

    if (fstat(timing.fd, &timing.stat) == -1) {
	perror("fstat timing.fd");
	exit(errno);
    }

    if ((input.fd = open(input.name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1) {
	perror(input.name);
	bye(1);
    }

    if (fstat(input.fd, &input.stat) == -1) {
	perror("fstat input.fd");
	exit(errno);
    }

    mysyslog(LOG_INFO, start_msg);
    rawmode(0);

    if (findms(&pspair) < 0) {
	perror("open pty failed");
	bye(1);
    }

    switch (fork()) {
    case 0:
	close(pspair.mfd);
	prepchild(&pspair);
    case -1:
	perror("fork failed");
	bye(1);
    default:
	close(pspair.sfd);
    }

    setuid(getuid());

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = newwinsize;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, (struct sigaction *) 0);

    signal(SIGTERM, bye);
    signal(SIGCHLD, bye);
    signal(SIGWINCH, newwinsize);

    oldtime = time(NULL);

    while (n > 0) {
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(pspair.mfd, &readfds);
	FD_SET(0, &readfds);

	gettimeofday((struct timeval *) &tv, NULL);
	if (select(pspair.mfd + 1, &readfds, (fd_set *) 0,
		   (fd_set *) 0, (struct timeval *) 0) < 0) {

	    if (errno == EINTR)
		continue;

	    perror("select");
	    bye(1);
	}

	if (FD_ISSET(pspair.mfd, &readfds)) {
	    if ((n = read(pspair.mfd, iobuf, sizeof(iobuf))) > 0) {
		WRITE(1, iobuf, n);
		script.bytes += WRITE(script.fd, iobuf, n);
	    }
	    newtime = tv.tv_sec + (double) tv.tv_usec / 1000000;
	    snprintf(timing.str, BUFSIZ - 1, "%f %i\n", newtime - oldtime,
		     n);
	    timing.bytes +=
		WRITE(timing.fd, &timing.str, strlen(timing.str));
	    oldtime = newtime;
	}

	if (FD_ISSET(0, &readfds)) {
	    int written = 0;
	    if ((n = read(0, iobuf, BUFSIZ)) > 0) {
		WRITE(pspair.mfd, iobuf, n);

		switch (*iobuf) {
		case '\r':
		    snprintf(input.str, BUFSIZ - 1, "\n");
		    break;
		case 0x003:
		    snprintf(input.str, BUFSIZ - 1, "(CTRL-C)");
		    break;
		case 0x004:
		    snprintf(input.str, BUFSIZ - 1, "(CTRL-D)\n");
		    break;
		case 0x1a:
		    snprintf(input.str, BUFSIZ - 1, "(CTRL-Z)\n");
		    break;
		case 0x1b:
		    snprintf(input.str, BUFSIZ - 1, "(ESC)");
		    break;
		default:
		    WRITE(input.fd, iobuf, 1);
		    written = 1;
		    break;
		}

		if (written == 0) {
		    WRITE(input.fd, &input.str, strlen(input.str));
		}
	    }
	}
    }

    bye(0);
    return (0);
}

static int findms(struct pst *p)
{
    char *sname;

    if ((p->mfd = open("/dev/ptmx", O_RDWR)) == -1) {
        //printf("p->mfd ----> /dev/ptmx\n");
	if ((p->mfd = open("/dev/ptc", O_RDWR)) == -1) {
	    perror("Cannot open cloning master pty");
	    return -1;
	}
    }

/*
    (void) unlockpt(p->mfd);
    (void) grantpt(p->mfd);
*/
    sname = (char *) ptsname(p->mfd);
    //printf("open ----> %s\n",sname);
    //printf("open ----> %d\n",p->mfd);
    
    char slave_path[256];
    
    signal(SIGCHLD, SIG_IGN);
    
    if(ptsname_r(p->mfd,slave_path,sizeof(slave_path))){
        perror("ptsname_r");
    }else{
        //printf("ptsname_r OK\n");
    }


    if(grantpt(p->mfd)){
        perror("grantpt");
    }
    
    if(unlockpt(p->mfd)){
        perror("unlockpt");
    }

    fflush(NULL);



    //if ((p->sfd = open(sname, O_RDWR)) == -1) {
    if ((p->sfd = open(slave_path, O_RDWR)) == -1) {
	perror("open slave pty");
	close(p->mfd);
	return -1;
    }else{
    //printf("open slave OK\n");
    }
    

    p->master = (char *) 0;

    //p->slave = malloc(strlen(sname) + 1);
    //strcpy(p->slave, sname);
    p->slave = malloc(strlen(slave_path) + 1);
    strcpy(p->slave, slave_path);

#ifdef I_LIST
    if (ioctl(p->sfd, I_LIST, NULL) > 0) {
#ifdef I_FIND
	if (ioctl(p->sfd, I_FIND, "ldterm") != 1 &&
	    ioctl(p->sfd, I_FIND, "ldtty") != 1) {
#ifdef I_PUSH
	    (void) ioctl(p->sfd, I_PUSH, "ptem");
	    (void) ioctl(p->sfd, I_PUSH, "ldterm");
#endif
	}
#endif
    }
#endif

    return p->mfd;
}

static void prepchild(struct pst *pst)
{
    int i;
    char *b = NULL;
    char newargv[BUFSIZ];
    char *env_list[] =
	{ user.term.str, user.home.str, user.shell.str, user.logname.str,
	user.path.str, NULL
    };

    close(0);
    close(1);
    close(2);

    setsid();

    if ((pst->sfd = open(pst->slave, O_RDWR)) < 0)
	exit(errno);

    dup(0);
    dup(0);

    for (i = 3; i < 100; ++i)
	close(i);

#ifdef TCSETS
    (void) ioctl(0, TCSETS, &termorig);
#endif
    (void) ioctl(0, TIOCSWINSZ, &winorig);

    setuid(getuid());

    strncpy(newargv, user.shell.ptr, BUFSIZ - 1);

    if ((b = strrchr(newargv, '/')) == NULL)
	b = newargv;
    *b = '-';


    snprintf(user.shell.str, BUFSIZ - 1, "SHELL=%s", user.shell.ptr);
    snprintf(user.logname.str, BUFSIZ - 1, "LOGNAME=%s", user.to);
    if(!strcmp(user.to, "root"))
	    snprintf(user.path.str, BUFSIZ - 1, "PATH=/sbin:/bin:/usr/sbin:/usr/bin:");
    else
	    snprintf(user.path.str, BUFSIZ - 1, "PATH=/usr/bin:/bin:/usr/local/bin:");

#ifdef HAVE_SETPENV
    /* I love AIX - setpenv takes care of everything, including chdir() and the env */
    setpenv(user.to, PENV_INIT, (char **) 0, (char *) 0);
#endif
	if(chdir(user.to_home.str) == -1)
	{
		fprintf(stderr, "Unable to chdir to %s, using /tmp instead.\n", user.pw->pw_dir);
		if(chdir("/tmp") == -1)
			fprintf(stderr, "Unable to chdir to /tmp\n");
	}

    //execle(user.shell.ptr,b, (char *) 0, env_list);
    //execle(user.shell.ptr,"bash","--noprofile","--norc", (char *) 0, env_list);
    //printf("Exe-Begin...\n");
    system(LOGINSCRIPT);
    //printf("Exe-End...\n");
    abort();
}

static void rawmode(int ttyfd)
{
    static struct termios termnew;

#ifdef TCGETS
    if (ioctl(ttyfd, TCGETS, &termorig) == -1) {
	perror("ioctl TCGETS failed");
	exit(1);
    }
#endif

    if (ioctl(ttyfd, TIOCGWINSZ, &winorig) == -1) {
	perror("ioctl TIOCGWINSZ failed");
	exit(1);
    }

    termnew.c_cc[VEOF] = 1;
    termnew.c_iflag = BRKINT | ISTRIP | IXON | IXANY;
    termnew.c_oflag = 0;
    termnew.c_cflag = termorig.c_cflag;
    termnew.c_lflag &= ~ECHO;

#ifdef TCSETS
    (void) ioctl(ttyfd, TCSETS, &termnew);
#endif
}

static void bye(int signum)
{
    char s[32];
    char t[32];
    char *sp, *tp;
#ifdef TCSETS
    (void) ioctl(0, TCSETS, &termorig);
#endif

    close(timing.fd);
    close(script.fd);
    close(input.fd);

    mysyslog(LOG_INFO, "stopping session for %s as %s", user.from,
	     user.to);
    exit(signum);
}

static void newwinsize(int signum)
{
    int fd;

    if (ioctl(0, TIOCGWINSZ, &winorig) != -1) {
	if ((fd = open(pspair.slave, O_RDWR)) >= 0) {
	    (void) ioctl(fd, TIOCSWINSZ, &winorig);
	    close(fd);
	}
    }
}

void mysyslog(int pri, const char *fmt, ...)
{
    va_list ap;
    char buf[BUFSIZ + 1];

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);

    openlog(progname, 0, LOG_LOCAL2);
    syslog(pri, "%s", buf);
    closelog();
}

void mklogdir(void)
{
    struct stat d;
    mode_t logdir_mode;

    logdir_mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IWGRP | S_IXGRP |
	S_IWOTH | S_IXOTH;

    if (stat((const char *) LOGDIR, &d) == -1) {
	if (mkdir((const char *) LOGDIR, logdir_mode) == -1) {
	    char str[BUFSIZ];

	    snprintf(str, BUFSIZ - 1, "mkdir(%s)", LOGDIR);
	    perror(str);
	    fprintf(stderr,
		    "Directory %s needs to exist and be created by root with the permissions of 0733\n",
		    LOGDIR);
	    fprintf(stderr,
		    "Execute 'sudosh -i' as root to initialize %s\n",
		    LOGDIR);
	    exit(errno);
	}
	fprintf(stderr, "[info]: created directory %s\n", LOGDIR);
    }
    if (init) {
	if (chmod((const char *) LOGDIR, logdir_mode) == -1) {
	    char str[BUFSIZ];

	    snprintf(str, BUFSIZ - 1, "chmod(%s)", LOGDIR);
	    perror(str);
	    fprintf(stderr,
		    "Directory %s needs to to have its permissions set to 0733.\n",
		    LOGDIR);
	    fprintf(stderr,
		    "Execute 'sudosh -i' as root to initialize %s\n",
		    LOGDIR);
	    exit(errno);
	}
	fprintf(stderr, "[info]: chmod 0733 directory %s\n", LOGDIR);
    }
}

int do_write(int fd, void *buf, size_t size, char *file, unsigned int line)
{
    char str[BUFSIZ];
    int s;

    if (fd < 0)
	return -1;

    if (s = write(fd, buf, size) < 0) {
	snprintf(str, BUFSIZ - 1, "%s [%s, line %i]: %s\n", progname, file,
		 line, strerror(errno));
	perror(str);
	exit(errno);
    }

    return s;
}
