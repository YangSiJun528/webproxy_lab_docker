/*
 * csapp.h - CS:APP3e 책에서 사용하는 프로토타입과 정의.
 */
/* $begin csapp.h */
#ifndef __CSAPP_H__
#define __CSAPP_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* 기본 파일 권한은 DEF_MODE & ~DEF_UMASK 입니다. */
/* $begin createmasks */
#define DEF_MODE   S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
#define DEF_UMASK  S_IWGRP|S_IWOTH
/* $end createmasks */

/* bind(), connect(), accept() 호출을 간단하게 만듭니다. */
/* $begin sockaddrdef */
typedef struct sockaddr SA;
/* $end sockaddrdef */

/* Robust I/O(Rio) 패키지가 유지하는 상태입니다. */
/* $begin rio_t */
#define RIO_BUFSIZE 8192
typedef struct {
    int rio_fd;                /* 이 내부 버퍼가 연결된 descriptor */
    int rio_cnt;               /* 내부 버퍼에 남아 있는 읽지 않은 바이트 수 */
    char *rio_bufptr;          /* 내부 버퍼에서 다음에 읽을 바이트 */
    char rio_buf[RIO_BUFSIZE]; /* 내부 버퍼 */
} rio_t;
/* $end rio_t */

/* 외부 변수 */
extern int h_errno;    /* DNS 오류를 위해 BIND가 정의합니다. */
extern char **environ; /* libc가 정의합니다. */

/* 기타 상수 */
#define	MAXLINE	 8192  /* 텍스트 한 줄의 최대 길이 */
#define MAXBUF   8192  /* I/O 버퍼의 최대 크기 */
#define LISTENQ  1024  /* listen()의 두 번째 인자 */

/* ==========================================================================
 * 자체 오류 처리 함수
 * ========================================================================== */
/**
 * @brief errno에 기반한 Unix 스타일 오류 메시지를 출력하고 종료합니다.
 *
 * @param[in] msg 오류 상황을 설명하는 접두 메시지입니다.
 * @return void. 메시지를 출력한 뒤 프로세스를 종료하므로 호출자에게 반환하지 않습니다.
 */
void unix_error(char *msg);

/**
 * @brief POSIX 오류 코드에 기반한 오류 메시지를 출력하고 종료합니다.
 *
 * @param[in] code strerror()에 전달할 POSIX 오류 코드입니다.
 * @param[in] msg 오류 상황을 설명하는 접두 메시지입니다.
 * @return void. 메시지를 출력한 뒤 프로세스를 종료하므로 호출자에게 반환하지 않습니다.
 */
void posix_error(int code, char *msg);

/**
 * @brief 구식 gethostbyname 계열 DNS 오류 메시지를 출력하고 종료합니다.
 *
 * @param[in] msg DNS 오류 상황을 설명하는 메시지입니다.
 * @return void. 메시지를 출력한 뒤 프로세스를 종료하므로 호출자에게 반환하지 않습니다.
 */
void dns_error(char *msg);

/**
 * @brief getaddrinfo 계열 오류 코드에 기반한 오류 메시지를 출력하고 종료합니다.
 *
 * @param[in] code gai_strerror()에 전달할 getaddrinfo 계열 오류 코드입니다.
 * @param[in] msg 오류 상황을 설명하는 접두 메시지입니다.
 * @return void. 메시지를 출력한 뒤 프로세스를 종료하므로 호출자에게 반환하지 않습니다.
 */
void gai_error(int code, char *msg);

/**
 * @brief 애플리케이션 수준 오류 메시지를 출력하고 종료합니다.
 *
 * @param[in] msg 출력할 애플리케이션 오류 메시지입니다.
 * @return void. 메시지를 출력한 뒤 프로세스를 종료하므로 호출자에게 반환하지 않습니다.
 */
void app_error(char *msg);

/* ==========================================================================
 * 프로세스 제어 래퍼
 * ========================================================================== */
/**
 * @brief fork()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @return pid_t. 부모 프로세스에는 자식 PID, 자식 프로세스에는 0을 반환합니다.
 */
pid_t Fork(void);

/**
 * @brief execve()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] filename 실행할 프로그램 경로입니다.
 * @param[in] argv 실행할 프로그램에 전달할 NULL 종료 인자 배열입니다.
 * @param[in] envp 실행할 프로그램에 전달할 NULL 종료 환경 변수 배열입니다.
 * @return void. 성공하면 현재 프로세스 이미지가 교체되어 반환하지 않습니다.
 */
void Execve(const char *filename, char *const argv[], char *const envp[]);

/**
 * @brief 자식 프로세스 하나를 기다리고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[out] status 자식 프로세스의 종료 상태를 저장할 위치입니다.
 * @return pid_t. 종료된 자식 프로세스의 PID를 반환합니다.
 */
pid_t Wait(int *status);

/**
 * @brief 지정한 자식 프로세스를 기다리고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] pid 기다릴 자식 프로세스 PID 또는 waitpid()가 허용하는 특수 값입니다.
 * @param[out] iptr 자식 프로세스의 종료 상태를 저장할 위치입니다.
 * @param[in] options waitpid()에 전달할 옵션 비트입니다.
 * @return pid_t. waitpid()의 반환값을 그대로 반환합니다.
 */
pid_t Waitpid(pid_t pid, int *iptr, int options);

/**
 * @brief 지정한 프로세스에 signal을 보내고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] pid signal을 받을 프로세스 또는 프로세스 그룹 ID입니다.
 * @param[in] signum 보낼 signal 번호입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Kill(pid_t pid, int signum);

/**
 * @brief 지정한 초만큼 sleep하고 남은 시간을 반환합니다.
 *
 * @param[in] secs sleep할 초 단위 시간입니다.
 * @return unsigned int. signal로 중단되면 남은 초를, 완료되면 0을 반환합니다.
 */
unsigned int Sleep(unsigned int secs);

/**
 * @brief signal이 도착할 때까지 프로세스를 일시 정지합니다.
 *
 * @return void. signal handler가 반환된 뒤 호출자에게 돌아옵니다.
 */
void Pause(void);

/**
 * @brief 지정한 초 뒤에 SIGALRM이 발생하도록 alarm을 설정합니다.
 *
 * @param[in] seconds SIGALRM을 보낼 때까지의 초 단위 시간입니다.
 * @return unsigned int. 이전 alarm에 남아 있던 초를 반환합니다.
 */
unsigned int Alarm(unsigned int seconds);

/**
 * @brief 프로세스 그룹 ID를 설정하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] pid 프로세스 그룹을 변경할 프로세스 PID입니다.
 * @param[in] pgid 설정할 프로세스 그룹 ID입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Setpgid(pid_t pid, pid_t pgid);

/**
 * @brief 호출한 프로세스의 프로세스 그룹 ID를 반환합니다.
 *
 * @return pid_t. 현재 프로세스의 프로세스 그룹 ID입니다.
 */
pid_t Getpgrp();

/* ==========================================================================
 * Signal 래퍼
 * ========================================================================== */
typedef void handler_t(int);

/**
 * @brief sigaction()으로 signal handler를 설치하고 이전 handler를 반환합니다.
 *
 * @param[in] signum handler를 설치할 signal 번호입니다.
 * @param[in] handler 설치할 signal handler 함수 포인터입니다.
 * @return handler_t*. 이전 signal handler를 반환합니다.
 */
handler_t *Signal(int signum, handler_t *handler);

/**
 * @brief 프로세스의 signal mask를 변경하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] how signal mask 변경 방식입니다.
 * @param[in] set 적용할 signal 집합입니다.
 * @param[out] oldset 이전 signal mask를 저장할 위치입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

/**
 * @brief signal 집합을 비우고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[out] set 초기화할 signal 집합입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Sigemptyset(sigset_t *set);

/**
 * @brief signal 집합을 모든 signal로 채우고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[out] set 채울 signal 집합입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Sigfillset(sigset_t *set);

/**
 * @brief signal 집합에 signal을 추가하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in,out] set signal을 추가할 집합입니다.
 * @param[in] signum 추가할 signal 번호입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Sigaddset(sigset_t *set, int signum);

/**
 * @brief signal 집합에서 signal을 제거하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in,out] set signal을 제거할 집합입니다.
 * @param[in] signum 제거할 signal 번호입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Sigdelset(sigset_t *set, int signum);

/**
 * @brief signal 집합에 signal이 포함되어 있는지 확인합니다.
 *
 * @param[in] set 검사할 signal 집합입니다.
 * @param[in] signum 포함 여부를 확인할 signal 번호입니다.
 * @return int. 포함되어 있으면 1, 포함되어 있지 않으면 0을 반환합니다.
 */
int Sigismember(const sigset_t *set, int signum);

/**
 * @brief 임시 signal mask로 signal을 기다리고 EINTR 외 오류가 있으면 종료합니다.
 *
 * @param[in] set 대기 중 임시로 적용할 signal mask입니다.
 * @return int. 정상적인 signal 전달 후에는 sigsuspend()처럼 -1을 반환합니다.
 */
int Sigsuspend(const sigset_t *set);

/* ==========================================================================
 * Sio(Signal-safe I/O) 루틴
 * ========================================================================== */
/**
 * @brief signal handler에서도 안전하게 문자열을 표준 출력에 씁니다.
 *
 * @param[in] s 출력할 NULL 종료 문자열입니다.
 * @return ssize_t. write()가 기록한 바이트 수를 반환하며, 실패하면 -1을 반환합니다.
 */
ssize_t sio_puts(char s[]);

/**
 * @brief signal handler에서도 안전하게 long 값을 표준 출력에 씁니다.
 *
 * @param[in] v 출력할 long 정수 값입니다.
 * @return ssize_t. 기록한 바이트 수를 반환하며, 실패하면 -1을 반환합니다.
 */
ssize_t sio_putl(long v);

/**
 * @brief signal-safe 방식으로 오류 메시지를 출력하고 즉시 종료합니다.
 *
 * @param[in] s 출력할 오류 메시지입니다.
 * @return void. _exit(1)을 호출하므로 호출자에게 반환하지 않습니다.
 */
void sio_error(char s[]);

/* ==========================================================================
 * Sio 래퍼
 * ========================================================================== */
/**
 * @brief sio_puts()를 호출하고 실패하면 signal-safe 오류 처리를 수행합니다.
 *
 * @param[in] s 출력할 NULL 종료 문자열입니다.
 * @return ssize_t. 성공 시 기록한 바이트 수를 반환합니다.
 */
ssize_t Sio_puts(char s[]);

/**
 * @brief sio_putl()을 호출하고 실패하면 signal-safe 오류 처리를 수행합니다.
 *
 * @param[in] v 출력할 long 정수 값입니다.
 * @return ssize_t. 성공 시 기록한 바이트 수를 반환합니다.
 */
ssize_t Sio_putl(long v);

/**
 * @brief signal-safe 방식으로 오류 메시지를 출력하고 즉시 종료합니다.
 *
 * @param[in] s 출력할 오류 메시지입니다.
 * @return void. _exit(1)을 호출하므로 호출자에게 반환하지 않습니다.
 */
void Sio_error(char s[]);

/* ==========================================================================
 * Unix I/O 래퍼
 * ========================================================================== */
/**
 * @brief open()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] pathname 열 파일 경로입니다.
 * @param[in] flags open()에 전달할 플래그입니다.
 * @param[in] mode 새 파일을 만들 때 사용할 권한입니다.
 * @return int. 열린 파일 descriptor를 반환합니다.
 */
int Open(const char *pathname, int flags, mode_t mode);

/**
 * @brief read()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] fd 읽을 파일 descriptor입니다.
 * @param[out] buf 읽은 데이터를 저장할 버퍼입니다.
 * @param[in] count 읽으려는 최대 바이트 수입니다.
 * @return ssize_t. 읽은 바이트 수를 반환하며, EOF이면 0을 반환합니다.
 */
ssize_t Read(int fd, void *buf, size_t count);

/**
 * @brief write()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] fd 쓸 파일 descriptor입니다.
 * @param[in] buf 쓸 데이터가 들어 있는 버퍼입니다.
 * @param[in] count 쓰려는 바이트 수입니다.
 * @return ssize_t. 기록한 바이트 수를 반환합니다.
 */
ssize_t Write(int fd, const void *buf, size_t count);

/**
 * @brief lseek()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] fildes 위치를 변경할 파일 descriptor입니다.
 * @param[in] offset whence 기준으로 이동할 offset입니다.
 * @param[in] whence offset 기준을 지정하는 값입니다.
 * @return off_t. 변경된 파일 offset을 반환합니다.
 */
off_t Lseek(int fildes, off_t offset, int whence);

/**
 * @brief close()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] fd 닫을 파일 descriptor입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Close(int fd);

/**
 * @brief select()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] n 검사할 descriptor 범위의 상한입니다.
 * @param[in,out] readfds 읽기 가능 여부를 검사할 descriptor 집합입니다.
 * @param[in,out] writefds 쓰기 가능 여부를 검사할 descriptor 집합입니다.
 * @param[in,out] exceptfds 예외 상태를 검사할 descriptor 집합입니다.
 * @param[in,out] timeout 대기 시간입니다. 호출 후 남은 시간이 반영될 수 있습니다.
 * @return int. 준비된 descriptor 수를 반환하며, timeout이면 0을 반환합니다.
 */
int Select(int  n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	   struct timeval *timeout);

/**
 * @brief dup2()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] fd1 복제할 원본 파일 descriptor입니다.
 * @param[in] fd2 복제 대상 파일 descriptor 번호입니다.
 * @return int. 성공 시 fd2를 반환합니다.
 */
int Dup2(int fd1, int fd2);

/**
 * @brief stat()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] filename 상태를 조회할 파일 경로입니다.
 * @param[out] buf 파일 상태 정보를 저장할 stat 구조체입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Stat(const char *filename, struct stat *buf);

/**
 * @brief fstat()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] fd 상태를 조회할 파일 descriptor입니다.
 * @param[out] buf 파일 상태 정보를 저장할 stat 구조체입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Fstat(int fd, struct stat *buf) ;

/* ==========================================================================
 * 디렉터리 래퍼
 * ========================================================================== */
/**
 * @brief opendir()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] name 열 디렉터리 경로입니다.
 * @return DIR*. 열린 디렉터리 스트림 포인터를 반환합니다.
 */
DIR *Opendir(const char *name);

/**
 * @brief readdir()를 호출하고 실제 오류가 있으면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in,out] dirp 읽을 디렉터리 스트림 포인터입니다.
 * @return struct dirent*. 다음 디렉터리 엔트리 포인터를 반환하며, 끝이면 NULL을 반환합니다.
 */
struct dirent *Readdir(DIR *dirp);

/**
 * @brief closedir()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] dirp 닫을 디렉터리 스트림 포인터입니다.
 * @return int. 성공 시 0을 반환합니다.
 */
int Closedir(DIR *dirp);

/* ==========================================================================
 * 메모리 매핑 래퍼
 * ========================================================================== */
/**
 * @brief mmap()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] addr 매핑할 희망 시작 주소입니다. NULL이면 커널이 선택합니다.
 * @param[in] len 매핑할 바이트 길이입니다.
 * @param[in] prot 매핑 영역의 보호 플래그입니다.
 * @param[in] flags 매핑 동작 플래그입니다.
 * @param[in] fd 매핑할 파일 descriptor입니다.
 * @param[in] offset 파일에서 매핑을 시작할 offset입니다.
 * @return void*. 매핑된 메모리의 시작 주소를 반환합니다.
 */
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);

/**
 * @brief munmap()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] start 해제할 매핑 영역의 시작 주소입니다.
 * @param[in] length 해제할 매핑 영역의 바이트 길이입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Munmap(void *start, size_t length);

/* ==========================================================================
 * 표준 I/O 래퍼
 * ========================================================================== */
/**
 * @brief fclose()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] fp 닫을 FILE 스트림입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Fclose(FILE *fp);

/**
 * @brief fdopen()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] fd FILE 스트림으로 연결할 파일 descriptor입니다.
 * @param[in] type 스트림 모드 문자열입니다.
 * @return FILE*. 열린 FILE 스트림 포인터를 반환합니다.
 */
FILE *Fdopen(int fd, const char *type);

/**
 * @brief fgets()를 호출하고 스트림 오류가 있으면 오류를 출력한 뒤 종료합니다.
 *
 * @param[out] ptr 읽은 문자열을 저장할 버퍼입니다.
 * @param[in] n 버퍼에 저장할 최대 문자 수입니다.
 * @param[in,out] stream 읽을 FILE 스트림입니다.
 * @return char*. 성공 시 ptr을 반환하며, EOF이고 오류가 없으면 NULL을 반환합니다.
 */
char *Fgets(char *ptr, int n, FILE *stream);

/**
 * @brief fopen()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] filename 열 파일 경로입니다.
 * @param[in] mode 파일 열기 모드 문자열입니다.
 * @return FILE*. 열린 FILE 스트림 포인터를 반환합니다.
 */
FILE *Fopen(const char *filename, const char *mode);

/**
 * @brief fputs()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] ptr 출력할 NULL 종료 문자열입니다.
 * @param[in,out] stream 쓸 FILE 스트림입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Fputs(const char *ptr, FILE *stream);

/**
 * @brief fread()를 호출하고 스트림 오류가 있으면 오류를 출력한 뒤 종료합니다.
 *
 * @param[out] ptr 읽은 데이터를 저장할 버퍼입니다.
 * @param[in] size 항목 하나의 바이트 크기입니다.
 * @param[in] nmemb 읽으려는 항목 수입니다.
 * @param[in,out] stream 읽을 FILE 스트림입니다.
 * @return size_t. 성공적으로 읽은 항목 수를 반환합니다.
 */
size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

/**
 * @brief fwrite()를 호출하고 요청한 항목을 모두 쓰지 못하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] ptr 쓸 데이터가 들어 있는 버퍼입니다.
 * @param[in] size 항목 하나의 바이트 크기입니다.
 * @param[in] nmemb 쓰려는 항목 수입니다.
 * @param[in,out] stream 쓸 FILE 스트림입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/* ==========================================================================
 * 동적 저장 공간 할당 래퍼
 * ========================================================================== */
/**
 * @brief malloc()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] size 할당할 바이트 수입니다.
 * @return void*. 할당된 메모리 블록의 시작 주소를 반환합니다.
 */
void *Malloc(size_t size);

/**
 * @brief realloc()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in,out] ptr 크기를 변경할 기존 메모리 블록 포인터입니다.
 * @param[in] size 새로 요청할 바이트 수입니다.
 * @return void*. 크기가 변경된 메모리 블록의 시작 주소를 반환합니다.
 */
void *Realloc(void *ptr, size_t size);

/**
 * @brief calloc()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] nmemb 할당할 항목 수입니다.
 * @param[in] size 항목 하나의 바이트 크기입니다.
 * @return void*. 0으로 초기화된 메모리 블록의 시작 주소를 반환합니다.
 */
void *Calloc(size_t nmemb, size_t size);

/**
 * @brief free()로 동적 메모리를 해제합니다.
 *
 * @param[in] ptr 해제할 메모리 블록 포인터입니다.
 * @return void. 반환값이 없습니다.
 */
void Free(void *ptr);

/* ==========================================================================
 * Socket 인터페이스 래퍼
 * ========================================================================== */
/**
 * @brief socket()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] domain 통신 domain입니다.
 * @param[in] type socket 타입입니다.
 * @param[in] protocol 사용할 protocol 번호입니다.
 * @return int. 생성된 socket descriptor를 반환합니다.
 */
int Socket(int domain, int type, int protocol);

/**
 * @brief setsockopt()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] s 옵션을 설정할 socket descriptor입니다.
 * @param[in] level 옵션이 속한 protocol level입니다.
 * @param[in] optname 설정할 옵션 이름입니다.
 * @param[in] optval 옵션 값이 들어 있는 버퍼입니다.
 * @param[in] optlen optval 버퍼의 바이트 길이입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Setsockopt(int s, int level, int optname, const void *optval, int optlen);

/**
 * @brief bind()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] sockfd bind할 socket descriptor입니다.
 * @param[in] my_addr socket에 연결할 local 주소 구조체입니다.
 * @param[in] addrlen my_addr 구조체의 바이트 길이입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Bind(int sockfd, struct sockaddr *my_addr, int addrlen);

/**
 * @brief listen()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] s listening 상태로 만들 socket descriptor입니다.
 * @param[in] backlog 대기열에 둘 수 있는 연결 요청 수입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Listen(int s, int backlog);

/**
 * @brief accept()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] s listening socket descriptor입니다.
 * @param[out] addr 연결된 peer 주소를 저장할 구조체입니다.
 * @param[in,out] addrlen 입력 시 addr 버퍼 길이, 출력 시 실제 주소 길이입니다.
 * @return int. 연결된 client socket descriptor를 반환합니다.
 */
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief connect()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] sockfd 연결할 socket descriptor입니다.
 * @param[in] serv_addr 연결 대상 서버 주소 구조체입니다.
 * @param[in] addrlen serv_addr 구조체의 바이트 길이입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen);

/* ==========================================================================
 * Protocol-independent 래퍼
 * ========================================================================== */
/**
 * @brief getaddrinfo()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] node 조회할 hostname 또는 numeric address입니다.
 * @param[in] service 조회할 service 이름 또는 port 문자열입니다.
 * @param[in] hints 조회 조건을 담은 addrinfo 구조체입니다.
 * @param[out] res 결과 addrinfo 연결 리스트를 저장할 위치입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Getaddrinfo(const char *node, const char *service,
                 const struct addrinfo *hints, struct addrinfo **res);

/**
 * @brief getnameinfo()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] sa 변환할 socket 주소 구조체입니다.
 * @param[in] salen sa 구조체의 바이트 길이입니다.
 * @param[out] host hostname 또는 numeric host 문자열을 저장할 버퍼입니다.
 * @param[in] hostlen host 버퍼의 바이트 길이입니다.
 * @param[out] serv service 또는 numeric port 문자열을 저장할 버퍼입니다.
 * @param[in] servlen serv 버퍼의 바이트 길이입니다.
 * @param[in] flags getnameinfo()에 전달할 플래그입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host,
                 size_t hostlen, char *serv, size_t servlen, int flags);

/**
 * @brief getaddrinfo()가 할당한 주소 목록을 해제합니다.
 *
 * @param[in] res 해제할 addrinfo 연결 리스트입니다.
 * @return void. 반환값이 없습니다.
 */
void Freeaddrinfo(struct addrinfo *res);

/**
 * @brief 네트워크 주소를 presentation 문자열로 변환하고 실패하면 종료합니다.
 *
 * @param[in] af 주소 체계입니다.
 * @param[in] src 변환할 네트워크 주소입니다.
 * @param[out] dst 변환된 문자열을 저장할 버퍼입니다.
 * @param[in] size dst 버퍼의 바이트 길이입니다.
 * @return void. 성공 시 dst에 결과 문자열을 저장합니다.
 */
void Inet_ntop(int af, const void *src, char *dst, socklen_t size);

/**
 * @brief presentation 문자열을 네트워크 주소로 변환하고 실패하면 종료합니다.
 *
 * @param[in] af 주소 체계입니다.
 * @param[in] src 변환할 presentation 주소 문자열입니다.
 * @param[out] dst 변환된 네트워크 주소를 저장할 버퍼입니다.
 * @return void. 성공 시 dst에 결과 주소를 저장합니다.
 */
void Inet_pton(int af, const char *src, void *dst);

/* ==========================================================================
 * DNS 래퍼
 * ========================================================================== */
/**
 * @brief gethostbyname()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] name 조회할 host 이름입니다.
 * @return struct hostent*. 조회된 host entry 포인터를 반환합니다.
 */
struct hostent *Gethostbyname(const char *name);

/**
 * @brief gethostbyaddr()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] addr 조회할 네트워크 주소 바이트 배열입니다.
 * @param[in] len addr의 바이트 길이입니다.
 * @param[in] type 주소 타입입니다.
 * @return struct hostent*. 조회된 host entry 포인터를 반환합니다.
 */
struct hostent *Gethostbyaddr(const char *addr, int len, int type);

/* ==========================================================================
 * Pthreads thread 제어 래퍼
 * ========================================================================== */
/**
 * @brief pthread_create()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[out] tidp 생성된 thread ID를 저장할 위치입니다.
 * @param[in] attrp thread 속성입니다. 기본값을 쓰려면 NULL을 전달합니다.
 * @param[in] routine 새 thread가 실행할 함수 포인터입니다.
 * @param[in] argp routine에 전달할 인자 포인터입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp,
		    void * (*routine)(void *), void *argp);

/**
 * @brief pthread_join()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] tid 기다릴 thread ID입니다.
 * @param[out] thread_return thread의 반환값을 저장할 위치입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Pthread_join(pthread_t tid, void **thread_return);

/**
 * @brief pthread_cancel()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] tid 취소 요청을 보낼 thread ID입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Pthread_cancel(pthread_t tid);

/**
 * @brief pthread_detach()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] tid detach할 thread ID입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Pthread_detach(pthread_t tid);

/**
 * @brief pthread_exit()으로 현재 thread를 종료합니다.
 *
 * @param[in] retval join하는 thread에 전달할 반환값 포인터입니다.
 * @return void. 현재 thread를 종료하므로 호출자에게 반환하지 않습니다.
 */
void Pthread_exit(void *retval);

/**
 * @brief 현재 thread의 ID를 반환합니다.
 *
 * @return pthread_t. 호출한 thread의 ID입니다.
 */
pthread_t Pthread_self(void);

/**
 * @brief 초기화 루틴이 한 번만 실행되도록 pthread_once()를 호출합니다.
 *
 * @param[in,out] once_control 한 번 실행 여부를 관리하는 제어 객체입니다.
 * @param[in] init_function 한 번만 실행할 초기화 함수입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Pthread_once(pthread_once_t *once_control, void (*init_function)());

/* ==========================================================================
 * POSIX semaphore 래퍼
 * ========================================================================== */
/**
 * @brief sem_init()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[out] sem 초기화할 semaphore 객체입니다.
 * @param[in] pshared process 간 공유 여부를 나타내는 값입니다.
 * @param[in] value semaphore의 초기 값입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Sem_init(sem_t *sem, int pshared, unsigned int value);

/**
 * @brief semaphore에 대해 P 연산(sem_wait)을 수행합니다.
 *
 * @param[in,out] sem wait할 semaphore 객체입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void P(sem_t *sem);

/**
 * @brief semaphore에 대해 V 연산(sem_post)을 수행합니다.
 *
 * @param[in,out] sem post할 semaphore 객체입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void V(sem_t *sem);

/* ==========================================================================
 * Rio(Robust I/O) 패키지
 * ========================================================================== */

// Robust I/O (rio)는 read/write가 부분적으로만 수행되거나 EINTR로 중단될 수 있는 상황을 안전하게 처리하기 위한 함수들이다.
//
// 특히 TCP 소켓에서는 한 번의 read()가 요청한 바이트 수를 모두 반환한다고 보장되지 않으므로,
// rio_readn()은 가능한 한 n바이트를 채울 때까지 반복해서 읽는다. 단, EOF를 만나면 그 시점까지 읽은 바이트 수를 반환한다.

/**
 * @brief 버퍼를 사용하지 않고 최대 n바이트를 견고하게 읽습니다.
 *
 * @param[in] fd 읽을 파일 descriptor입니다.
 * @param[out] usrbuf 읽은 데이터를 저장할 사용자 버퍼입니다.
 * @param[in] n 읽으려는 최대 바이트 수입니다.
 * @return ssize_t. 실제로 읽은 바이트 수를 반환하며, 오류가 발생하면 -1을 반환합니다.
 */
ssize_t rio_readn(int fd, void *usrbuf, size_t n);

/**
 * @brief 버퍼를 사용하지 않고 정확히 n바이트를 견고하게 씁니다.
 *
 * @param[in] fd 쓸 파일 descriptor입니다.
 * @param[in] usrbuf 쓸 데이터가 들어 있는 사용자 버퍼입니다.
 * @param[in] n 쓰려는 바이트 수입니다.
 * @return ssize_t. 성공 시 n을 반환하며, 오류가 발생하면 -1을 반환합니다.
 */
ssize_t rio_writen(int fd, void *usrbuf, size_t n);

/**
 * @brief Rio 읽기 버퍼를 descriptor와 연결하고 초기화합니다.
 *
 * @param[out] rp 초기화할 Rio 읽기 버퍼입니다.
 * @param[in] fd Rio 버퍼와 연결할 파일 descriptor입니다.
 * @return void. 반환값이 없습니다.
 */
void rio_readinitb(rio_t *rp, int fd);

/**
 * @brief Rio 내부 버퍼를 사용해 최대 n바이트를 견고하게 읽습니다.
 *
 * @param[in,out] rp 초기화된 Rio 읽기 버퍼입니다.
 * @param[out] usrbuf 읽은 데이터를 저장할 사용자 버퍼입니다.
 * @param[in] n 읽으려는 최대 바이트 수입니다.
 * @return ssize_t. 실제로 읽은 바이트 수를 반환하며, 오류가 발생하면 -1을 반환합니다.
 */
ssize_t	rio_readnb(rio_t *rp, void *usrbuf, size_t n);

/**
 * @brief Rio 내부 버퍼를 사용해 텍스트 한 줄을 견고하게 읽습니다.
 *
 * @param[in,out] rp 초기화된 Rio 읽기 버퍼입니다.
 * @param[out] usrbuf 읽은 줄을 저장할 사용자 버퍼입니다.
 * @param[in] maxlen usrbuf에 저장할 수 있는 최대 길이입니다.
 * @return ssize_t. 읽은 바이트 수를 반환합니다. EOF에서 데이터가 없으면 0,
 * 오류가 발생하면 -1을 반환합니다.
 */
ssize_t	rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

/* ==========================================================================
 * Rio 패키지 래퍼
 * ========================================================================== */
/**
 * @brief rio_readn()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] fd 읽을 파일 descriptor입니다.
 * @param[out] usrbuf 읽은 데이터를 저장할 사용자 버퍼입니다.
 * @param[in] n 읽으려는 최대 바이트 수입니다.
 * @return ssize_t. 실제로 읽은 바이트 수를 반환합니다.
 */
ssize_t Rio_readn(int fd, void *usrbuf, size_t n);

/**
 * @brief rio_writen()을 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] fd 쓸 파일 descriptor입니다.
 * @param[in] usrbuf 쓸 데이터가 들어 있는 사용자 버퍼입니다.
 * @param[in] n 쓰려는 바이트 수입니다.
 * @return void. 성공 시 반환값이 없습니다.
 */
void Rio_writen(int fd, void *usrbuf, size_t n);

/**
 * @brief rio_readinitb()로 Rio 읽기 버퍼를 초기화합니다.
 *
 * @param[out] rp 초기화할 Rio 읽기 버퍼입니다.
 * @param[in] fd Rio 버퍼와 연결할 파일 descriptor입니다.
 * @return void. 반환값이 없습니다.
 */
void Rio_readinitb(rio_t *rp, int fd);

/**
 * @brief rio_readnb()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in,out] rp 초기화된 Rio 읽기 버퍼입니다.
 * @param[out] usrbuf 읽은 데이터를 저장할 사용자 버퍼입니다.
 * @param[in] n 읽으려는 최대 바이트 수입니다.
 * @return ssize_t. 실제로 읽은 바이트 수를 반환합니다.
 */
ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n);

/**
 * @brief rio_readlineb()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in,out] rp 초기화된 Rio 읽기 버퍼입니다.
 * @param[out] usrbuf 읽은 줄을 저장할 사용자 버퍼입니다.
 * @param[in] maxlen usrbuf에 저장할 수 있는 최대 길이입니다.
 * @return ssize_t. 읽은 바이트 수를 반환합니다. EOF에서 데이터가 없으면 0을 반환합니다.
 */
ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

/* ==========================================================================
 * 재진입 가능한 protocol-independent client/server 헬퍼
 * ========================================================================== */
/**
 * @brief 지정한 hostname과 port의 서버에 연결합니다.
 *
 * @param[in] hostname 연결할 서버의 hostname입니다.
 * @param[in] port 연결할 서버의 port 문자열입니다.
 * @return int. 성공 시 읽기/쓰기 가능한 socket descriptor를 반환합니다.
 * getaddrinfo 오류는 -2, 그 외 오류는 errno를 설정한 뒤 -1을 반환합니다.
 */
int open_clientfd(char *hostname, char *port);

/**
 * @brief 지정한 port에서 연결 요청을 받을 listening socket을 엽니다.
 *
 * @param[in] port 연결 요청을 받을 port 문자열입니다.
 * @return int. 성공 시 listening socket descriptor를 반환합니다.
 * getaddrinfo 오류는 -2, 그 외 오류는 errno를 설정한 뒤 -1을 반환합니다.
 */
int open_listenfd(char *port);

/* ==========================================================================
 * 재진입 가능한 protocol-independent client/server 헬퍼의 래퍼
 * ========================================================================== */
/**
 * @brief open_clientfd()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] hostname 연결할 서버의 hostname입니다.
 * @param[in] port 연결할 서버의 port 문자열입니다.
 * @return int. 성공 시 읽기/쓰기 가능한 socket descriptor를 반환합니다.
 */
int Open_clientfd(char *hostname, char *port);

/**
 * @brief open_listenfd()를 호출하고 실패하면 오류를 출력한 뒤 종료합니다.
 *
 * @param[in] port 연결 요청을 받을 port 문자열입니다.
 * @return int. 성공 시 listening socket descriptor를 반환합니다.
 */
int Open_listenfd(char *port);


#endif /* __CSAPP_H__ */
/* $end csapp.h */
