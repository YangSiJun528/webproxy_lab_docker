/*
 * csapp.c - CS:APP3e 책에서 사용하는 함수들
 *
 * 2016년 10월 reb 업데이트:
 *   - sio_ltoa가 음수를 처리하지 못하던 버그를 수정했습니다.
 *
 * 2016년 2월 droh 업데이트:
 *   - open_clientfd와 open_listenfd가 실패를 더 부드럽게 처리하도록 수정했습니다.
 *
 * 2014년 8월 droh 업데이트:
 *   - open_clientfd와 open_listenfd의 새 버전은 재진입 가능하고
 *     protocol-independent입니다.
 *
 *   - protocol-independent inet_ntop와 inet_pton 함수를 추가했습니다.
 *     inet_ntoa와 inet_aton 함수는 더 이상 사용하지 않습니다.
 *
 * 2014년 7월 droh 업데이트:
 *   - 재진입 가능한 sio(signal-safe I/O) 루틴을 추가했습니다.
 *
 * 2013년 4월 droh 업데이트:
 *   - rio_readlineb: edge case 버그를 수정했습니다.
 *   - rio_readnb: 중복된 EINTR 검사를 제거했습니다.
 */
/* $begin csapp.c */
#include "csapp.h"

/**************************
 * 오류 처리 함수
 **************************/
/* $begin errorfuns */
/* $begin unixerror */
void unix_error(char *msg) /* Unix 스타일 오류 */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */

void posix_error(int code, char *msg) /* Posix 스타일 오류 */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
}

void gai_error(int code, char *msg) /* Getaddrinfo 스타일 오류 */
{
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
    exit(0);
}

void app_error(char *msg) /* 애플리케이션 오류 */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}
/* $end errorfuns */

void dns_error(char *msg) /* 구식 gethostbyname 오류 */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}


/*********************************************
 * Unix 프로세스 제어 함수의 래퍼
 ********************************************/

/* $begin forkwrapper */
pid_t Fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}
/* $end forkwrapper */

void Execve(const char *filename, char *const argv[], char *const envp[]) 
{
    if (execve(filename, argv, envp) < 0)
	unix_error("Execve error");
}

/* $begin wait */
pid_t Wait(int *status) 
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
	unix_error("Wait error");
    return pid;
}
/* $end wait */

pid_t Waitpid(pid_t pid, int *iptr, int options) 
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0) 
	unix_error("Waitpid error");
    return(retpid);
}

/* $begin kill */
void Kill(pid_t pid, int signum) 
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
	unix_error("Kill error");
}
/* $end kill */

void Pause() 
{
    (void)pause();
    return;
}

unsigned int Sleep(unsigned int secs) 
{
    unsigned int rc;

    if ((rc = sleep(secs)) < 0)
	unix_error("Sleep error");
    return rc;
}

unsigned int Alarm(unsigned int seconds) {
    return alarm(seconds);
}
 
void Setpgid(pid_t pid, pid_t pgid) {
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0)
	unix_error("Setpgid error");
    return;
}

pid_t Getpgrp(void) {
    return getpgrp();
}

/************************************
 * Unix signal 함수의 래퍼
 ***********************************/

/* $begin sigaction */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* 처리 중인 signal 유형을 차단합니다. */
    action.sa_flags = SA_RESTART; /* 가능하면 system call을 재시작합니다. */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}
/* $end sigaction */

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (sigprocmask(how, set, oldset) < 0)
	unix_error("Sigprocmask error");
    return;
}

void Sigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0)
	unix_error("Sigemptyset error");
    return;
}

void Sigfillset(sigset_t *set)
{ 
    if (sigfillset(set) < 0)
	unix_error("Sigfillset error");
    return;
}

void Sigaddset(sigset_t *set, int signum)
{
    if (sigaddset(set, signum) < 0)
	unix_error("Sigaddset error");
    return;
}

void Sigdelset(sigset_t *set, int signum)
{
    if (sigdelset(set, signum) < 0)
	unix_error("Sigdelset error");
    return;
}

int Sigismember(const sigset_t *set, int signum)
{
    int rc;
    if ((rc = sigismember(set, signum)) < 0)
	unix_error("Sigismember error");
    return rc;
}

int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set); /* 항상 -1을 반환합니다. */
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}

/*************************************************************
 * Sio(Signal-safe I/O) 패키지 - signal handler에서 안전하게 사용할 수 있는
 * 단순하고 재진입 가능한 출력 함수들입니다.
 *************************************************************/

/* private sio 함수 */

/* $begin sioprivate */
/* sio_reverse - 문자열을 뒤집습니다(K&R 기반). */
static void sio_reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* sio_ltoa - long 값을 b진수 문자열로 변환합니다(K&R 기반). */
static void sio_ltoa(long v, char s[], int b) 
{
    int c, i = 0;
    int neg = v < 0;

    if (neg)
	v = -v;

    do {  
        s[i++] = ((c = (v % b)) < 10)  ?  c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);

    if (neg)
	s[i++] = '-';

    s[i] = '\0';
    sio_reverse(s);
}

/* sio_strlen - 문자열 길이를 반환합니다(K&R 기반). */
static size_t sio_strlen(char s[])
{
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}
/* $end sioprivate */

/* Public Sio 함수 */
/* $begin siopublic */

ssize_t sio_puts(char s[]) /* 문자열을 출력합니다. */
{
    return write(STDOUT_FILENO, s, sio_strlen(s)); //line:csapp:siostrlen
}

ssize_t sio_putl(long v) /* long 값을 출력합니다. */
{
    char s[128];
    
    sio_ltoa(v, s, 10); /* K&R itoa() 기반입니다. */  //line:csapp:sioltoa
    return sio_puts(s);
}

void sio_error(char s[]) /* 오류 메시지를 출력하고 종료합니다. */
{
    sio_puts(s);
    _exit(1);                                      //line:csapp:sioexit
}
/* $end siopublic */

/*******************************
 * SIO 루틴의 래퍼
 ******************************/
ssize_t Sio_putl(long v)
{
    ssize_t n;
  
    if ((n = sio_putl(v)) < 0)
	sio_error("Sio_putl error");
    return n;
}

ssize_t Sio_puts(char s[])
{
    ssize_t n;
  
    if ((n = sio_puts(s)) < 0)
	sio_error("Sio_puts error");
    return n;
}

void Sio_error(char s[])
{
    sio_error(s);
}

/********************************
 * Unix I/O 루틴의 래퍼
 ********************************/

int Open(const char *pathname, int flags, mode_t mode) 
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
	unix_error("Open error");
    return rc;
}

ssize_t Read(int fd, void *buf, size_t count) 
{
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0) 
	unix_error("Read error");
    return rc;
}

ssize_t Write(int fd, const void *buf, size_t count) 
{
    ssize_t rc;

    if ((rc = write(fd, buf, count)) < 0)
	unix_error("Write error");
    return rc;
}

off_t Lseek(int fildes, off_t offset, int whence) 
{
    off_t rc;

    if ((rc = lseek(fildes, offset, whence)) < 0)
	unix_error("Lseek error");
    return rc;
}

void Close(int fd) 
{
    int rc;

    if ((rc = close(fd)) < 0)
	unix_error("Close error");
}

int Select(int  n, fd_set *readfds, fd_set *writefds,
	   fd_set *exceptfds, struct timeval *timeout) 
{
    int rc;

    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
	unix_error("Select error");
    return rc;
}

int Dup2(int fd1, int fd2) 
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
	unix_error("Dup2 error");
    return rc;
}

void Stat(const char *filename, struct stat *buf) 
{
    if (stat(filename, buf) < 0)
	unix_error("Stat error");
}

void Fstat(int fd, struct stat *buf) 
{
    if (fstat(fd, buf) < 0)
	unix_error("Fstat error");
}

/*********************************
 * 디렉터리 함수의 래퍼
 *********************************/

DIR *Opendir(const char *name) 
{
    DIR *dirp = opendir(name); 

    if (!dirp)
        unix_error("opendir error");
    return dirp;
}

struct dirent *Readdir(DIR *dirp)
{
    struct dirent *dep;
    
    errno = 0;
    dep = readdir(dirp);
    if ((dep == NULL) && (errno != 0))
        unix_error("readdir error");
    return dep;
}

int Closedir(DIR *dirp) 
{
    int rc;

    if ((rc = closedir(dirp)) < 0)
        unix_error("closedir error");
    return rc;
}

/***************************************
 * 메모리 매핑 함수의 래퍼
 ***************************************/
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) 
{
    void *ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1))
	unix_error("mmap error");
    return(ptr);
}

void Munmap(void *start, size_t length) 
{
    if (munmap(start, length) < 0)
	unix_error("munmap error");
}

/***************************************************
 * 동적 저장 공간 할당 함수의 래퍼
 ***************************************************/

void *Malloc(size_t size) 
{
    void *p;

    if ((p  = malloc(size)) == NULL)
	unix_error("Malloc error");
    return p;
}

void *Realloc(void *ptr, size_t size) 
{
    void *p;

    if ((p  = realloc(ptr, size)) == NULL)
	unix_error("Realloc error");
    return p;
}

void *Calloc(size_t nmemb, size_t size) 
{
    void *p;

    if ((p = calloc(nmemb, size)) == NULL)
	unix_error("Calloc error");
    return p;
}

void Free(void *ptr) 
{
    free(ptr);
}

/******************************************
 * 표준 I/O 함수의 래퍼
 ******************************************/
void Fclose(FILE *fp) 
{
    if (fclose(fp) != 0)
	unix_error("Fclose error");
}

FILE *Fdopen(int fd, const char *type) 
{
    FILE *fp;

    if ((fp = fdopen(fd, type)) == NULL)
	unix_error("Fdopen error");

    return fp;
}

char *Fgets(char *ptr, int n, FILE *stream) 
{
    char *rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
	app_error("Fgets error");

    return rptr;
}

FILE *Fopen(const char *filename, const char *mode) 
{
    FILE *fp;

    if ((fp = fopen(filename, mode)) == NULL)
	unix_error("Fopen error");

    return fp;
}

void Fputs(const char *ptr, FILE *stream) 
{
    if (fputs(ptr, stream) == EOF)
	unix_error("Fputs error");
}

size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream) 
{
    size_t n;

    if (((n = fread(ptr, size, nmemb, stream)) < nmemb) && ferror(stream)) 
	unix_error("Fread error");
    return n;
}

void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) 
{
    if (fwrite(ptr, size, nmemb, stream) < nmemb)
	unix_error("Fwrite error");
}


/****************************
 * Socket 인터페이스 래퍼
 ****************************/

int Socket(int domain, int type, int protocol) 
{
    int rc;

    if ((rc = socket(domain, type, protocol)) < 0)
	unix_error("Socket error");
    return rc;
}

void Setsockopt(int s, int level, int optname, const void *optval, int optlen) 
{
    int rc;

    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
	unix_error("Setsockopt error");
}

void Bind(int sockfd, struct sockaddr *my_addr, int addrlen) 
{
    int rc;

    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
	unix_error("Bind error");
}

void Listen(int s, int backlog) 
{
    int rc;

    if ((rc = listen(s,  backlog)) < 0)
	unix_error("Listen error");
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen) 
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
	unix_error("Accept error");
    return rc;
}

void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen) 
{
    int rc;

    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
	unix_error("Connect error");
}

/*******************************
 * Protocol-independent 래퍼
 *******************************/
/* $begin getaddrinfo */
void Getaddrinfo(const char *node, const char *service, 
                 const struct addrinfo *hints, struct addrinfo **res)
{
    int rc;

    if ((rc = getaddrinfo(node, service, hints, res)) != 0) 
        gai_error(rc, "Getaddrinfo error");
}
/* $end getaddrinfo */

void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, 
                 size_t hostlen, char *serv, size_t servlen, int flags)
{
    int rc;

    if ((rc = getnameinfo(sa, salen, host, hostlen, serv, 
                          servlen, flags)) != 0) 
        gai_error(rc, "Getnameinfo error");
}

void Freeaddrinfo(struct addrinfo *res)
{
    freeaddrinfo(res);
}

void Inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (!inet_ntop(af, src, dst, size))
        unix_error("Inet_ntop error");
}

void Inet_pton(int af, const char *src, void *dst) 
{
    int rc;

    rc = inet_pton(af, src, dst);
    if (rc == 0)
	app_error("inet_pton error: invalid dotted-decimal address");
    else if (rc < 0)
        unix_error("Inet_pton error");
}

/*******************************************
 * DNS 인터페이스 래퍼
 *
 * 참고: 이 함수들은 thread-safe하지 않아 구식입니다.
 * 대신 getaddrinfo와 getnameinfo를 사용하세요.
 ***********************************/

/* $begin gethostbyname */
struct hostent *Gethostbyname(const char *name) 
{
    struct hostent *p;

    if ((p = gethostbyname(name)) == NULL)
	dns_error("Gethostbyname error");
    return p;
}
/* $end gethostbyname */

struct hostent *Gethostbyaddr(const char *addr, int len, int type) 
{
    struct hostent *p;

    if ((p = gethostbyaddr(addr, len, type)) == NULL)
	dns_error("Gethostbyaddr error");
    return p;
}

/************************************************
 * Pthreads thread 제어 함수의 래퍼
 ************************************************/

void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, 
		    void * (*routine)(void *), void *argp) 
{
    int rc;

    if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
	posix_error(rc, "Pthread_create error");
}

void Pthread_cancel(pthread_t tid) {
    int rc;

    if ((rc = pthread_cancel(tid)) != 0)
	posix_error(rc, "Pthread_cancel error");
}

void Pthread_join(pthread_t tid, void **thread_return) {
    int rc;

    if ((rc = pthread_join(tid, thread_return)) != 0)
	posix_error(rc, "Pthread_join error");
}

/* $begin detach */
void Pthread_detach(pthread_t tid) {
    int rc;

    if ((rc = pthread_detach(tid)) != 0)
	posix_error(rc, "Pthread_detach error");
}
/* $end detach */

void Pthread_exit(void *retval) {
    pthread_exit(retval);
}

pthread_t Pthread_self(void) {
    return pthread_self();
}
 
void Pthread_once(pthread_once_t *once_control, void (*init_function)()) {
    pthread_once(once_control, init_function);
}

/*******************************
 * POSIX semaphore 래퍼
 *******************************/

void Sem_init(sem_t *sem, int pshared, unsigned int value) 
{
    if (sem_init(sem, pshared, value) < 0)
	unix_error("Sem_init error");
}

void P(sem_t *sem) 
{
    if (sem_wait(sem) < 0)
	unix_error("P error");
}

void V(sem_t *sem) 
{
    if (sem_post(sem) < 0)
	unix_error("V error");
}

/****************************************
 * Rio 패키지 - Robust I/O 함수
 ****************************************/

/**
 * @brief 버퍼를 사용하지 않고 최대 n바이트를 견고하게 읽습니다.
 *
 * @param fd 읽을 파일 descriptor입니다.
 * @param usrbuf 읽은 데이터를 저장할 사용자 버퍼입니다.
 * @param n 읽으려는 최대 바이트 수입니다.
 * @return 읽은 바이트 수를 반환하고, 오류가 발생하면 -1을 반환합니다.
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* signal handler 반환으로 중단되었습니다. */
		nread = 0;      /* read()를 다시 호출합니다. */
	    else
		return -1;      /* errno는 read()가 설정합니다. */
	} 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* 0 이상의 값을 반환합니다. */
}
/* $end rio_readn */

/**
 * @brief 버퍼를 사용하지 않고 n바이트를 견고하게 씁니다.
 *
 * @param fd 쓸 파일 descriptor입니다.
 * @param usrbuf 쓸 데이터가 들어 있는 사용자 버퍼입니다.
 * @param n 쓰려는 바이트 수입니다.
 * @return 성공 시 n을 반환하고, 오류가 발생하면 -1을 반환합니다.
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nwritten = write(fd, bufp, nleft)) <= 0) {
	    if (errno == EINTR)  /* signal handler 반환으로 중단되었습니다. */
		nwritten = 0;    /* write()를 다시 호출합니다. */
	    else
		return -1;       /* errno는 write()가 설정합니다. */
	}
	nleft -= nwritten;
	bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */


/*
 * rio_read - Unix read() 함수의 래퍼입니다. 내부 버퍼에서 사용자 버퍼로
 *    min(n, rio_cnt) 바이트를 옮깁니다. 여기서 n은 사용자가 요청한
 *    바이트 수이고 rio_cnt는 내부 버퍼에 남아 있는 읽지 않은 바이트
 *    수입니다. 진입 시 내부 버퍼가 비어 있으면 read()를 호출해 내부
 *    버퍼를 다시 채웁니다.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* 버퍼가 비어 있으면 다시 채웁니다. */
	rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, 
			   sizeof(rp->rio_buf));
	if (rp->rio_cnt < 0) {
	    if (errno != EINTR) /* signal handler 반환으로 중단되었습니다. */
		return -1;
	}
	else if (rp->rio_cnt == 0)  /* EOF */
	    return 0;
	else 
	    rp->rio_bufptr = rp->rio_buf; /* 버퍼 포인터를 초기화합니다. */
    }

    /* 내부 버퍼에서 사용자 버퍼로 min(n, rp->rio_cnt) 바이트를 복사합니다. */
    cnt = n;          
    if (rp->rio_cnt < n)   
	cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}
/* $end rio_read */

/**
 * @brief Rio 읽기 버퍼를 descriptor와 연결하고 초기화합니다.
 *
 * @param rp 초기화할 Rio 읽기 버퍼입니다.
 * @param fd Rio 버퍼와 연결할 파일 descriptor입니다.
 */
/* $begin rio_readinitb */
void rio_readinitb(rio_t *rp, int fd) 
{
    rp->rio_fd = fd;  
    rp->rio_cnt = 0;  
    rp->rio_bufptr = rp->rio_buf;
}
/* $end rio_readinitb */

/**
 * @brief Rio 내부 버퍼를 사용해 최대 n바이트를 견고하게 읽습니다.
 *
 * @param rp 초기화된 Rio 읽기 버퍼입니다.
 * @param usrbuf 읽은 데이터를 저장할 사용자 버퍼입니다.
 * @param n 읽으려는 최대 바이트 수입니다.
 * @return 읽은 바이트 수를 반환하고, 오류가 발생하면 -1을 반환합니다.
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;
    
    while (nleft > 0) {
	if ((nread = rio_read(rp, bufp, nleft)) < 0) 
            return -1;          /* errno는 read()가 설정합니다. */
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* 0 이상의 값을 반환합니다. */
}
/* $end rio_readnb */

/**
 * @brief Rio 내부 버퍼를 사용해 텍스트 한 줄을 견고하게 읽습니다.
 *
 * @param rp 초기화된 Rio 읽기 버퍼입니다.
 * @param usrbuf 읽은 줄을 저장할 사용자 버퍼입니다.
 * @param maxlen 버퍼에 저장할 수 있는 최대 길이입니다.
 * @return 읽은 바이트 수를 반환합니다. EOF에서 데이터가 없으면 0,
 * 오류가 발생하면 -1을 반환합니다.
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) { 
        if ((rc = rio_read(rp, &c, 1)) == 1) {
	    *bufp++ = c;
	    if (c == '\n') {
                n++;
     		break;
            }
	} else if (rc == 0) {
	    if (n == 1)
		return 0; /* EOF, 읽은 데이터가 없습니다. */
	    else
		break;    /* EOF, 일부 데이터를 읽었습니다. */
	} else
	    return -1;	  /* 오류 */
    }
    *bufp = 0;
    return n-1;
}
/* $end rio_readlineb */

/**********************************
 * Robust I/O 루틴의 래퍼
 **********************************/
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes) 
{
    ssize_t n;
  
    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
	unix_error("Rio_readn error");
    return n;
}

void Rio_writen(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n)
	unix_error("Rio_writen error");
}

void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
} 

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
	unix_error("Rio_readnb error");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
	unix_error("Rio_readlineb error");
    return rc;
} 

/********************************
 * Client/server 헬퍼 함수
 ********************************/
/**
 * @brief <hostname, port>의 서버에 연결합니다.
 *
 * @param hostname 연결할 서버의 hostname입니다.
 * @param port 연결할 서버의 port입니다.
 * @return 성공 시 읽기/쓰기 가능한 socket descriptor를 반환합니다.
 * getaddrinfo 오류는 -2, 그 외 오류는 errno를 설정한 뒤 -1을 반환합니다.
 *
 * 이 함수는 재진입 가능하고 protocol-independent입니다.
 */
/* $begin open_clientfd */
int open_clientfd(char *hostname, char *port) {
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;

    /* 가능한 서버 주소 목록을 가져옵니다. */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* 연결을 엽니다. */
    hints.ai_flags = AI_NUMERICSERV;  /* 숫자 port 인자를 사용합니다. */
    hints.ai_flags |= AI_ADDRCONFIG;  /* 연결에 권장됩니다. */
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        return -2;
    }
  
    /* 성공적으로 연결할 수 있는 주소를 찾을 때까지 목록을 순회합니다. */
    for (p = listp; p; p = p->ai_next) {
        /* socket descriptor를 생성합니다. */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue; /* Socket 생성 실패, 다음 주소를 시도합니다. */

        /* 서버에 연결합니다. */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) 
            break; /* 성공 */
        if (close(clientfd) < 0) { /* 연결 실패, 다른 주소를 시도합니다. */  //line:netp:openclientfd:closefd
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1;
        } 
    } 

    /* 정리합니다. */
    freeaddrinfo(listp);
    if (!p) /* 모든 연결이 실패했습니다. */
        return -1;
    else    /* 마지막 연결이 성공했습니다. */
        return clientfd;
}
/* $end open_clientfd */

/**
 * @brief port에서 listening socket을 열고 반환합니다.
 *
 * @param port 연결 요청을 받을 port입니다.
 * @return 성공 시 listening socket descriptor를 반환합니다.
 * getaddrinfo 오류는 -2, 그 외 오류는 errno를 설정한 뒤 -1을 반환합니다.
 *
 * 이 함수는 재진입 가능하고 protocol-independent입니다.
 */
/* $begin open_listenfd */
int open_listenfd(char *port) 
{
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval=1;

    /* 가능한 서버 주소 목록을 가져옵니다. */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             /* 연결을 받습니다. */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* 모든 IP 주소에서 받습니다. */
    hints.ai_flags |= AI_NUMERICSERV;            /* port 번호를 사용합니다. */
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
        return -2;
    }

    /* bind할 수 있는 주소를 찾을 때까지 목록을 순회합니다. */
    for (p = listp; p; p = p->ai_next) {
        /* socket descriptor를 생성합니다. */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue;  /* Socket 생성 실패, 다음 주소를 시도합니다. */

        /* bind의 "Address already in use" 오류를 줄입니다. */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,    //line:netp:csapp:setsockopt
                   (const void *)&optval , sizeof(int));

        /* descriptor를 주소에 bind합니다. */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* 성공 */
        if (close(listenfd) < 0) { /* Bind 실패, 다음 주소를 시도합니다. */
            fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }


    /* 정리합니다. */
    freeaddrinfo(listp);
    if (!p) /* 동작한 주소가 없습니다. */
        return -1;

    /* 연결 요청을 받을 준비가 된 listening socket으로 만듭니다. */
    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
	return -1;
    }
    return listenfd;
}
/* $end open_listenfd */

/****************************************************
 * 재진입 가능한 protocol-independent 헬퍼의 래퍼
 ****************************************************/
int Open_clientfd(char *hostname, char *port) 
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0) 
	unix_error("Open_clientfd error");
    return rc;
}

int Open_listenfd(char *port) 
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
	unix_error("Open_listenfd error");
    return rc;
}

/* $end csapp.c */


