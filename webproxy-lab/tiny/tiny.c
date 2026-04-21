/* $begin tinymain */
/*
 * tiny.c - GET 메서드를 사용해 정적/동적 콘텐츠를 제공하는
 *     단순한 반복형 HTTP/1.0 웹 서버입니다.
 *
 * 2019년 11월 droh 업데이트
 *   - serve_static()과 clienterror()의 sprintf() aliasing 문제를 수정했습니다.
 */
#include "csapp.h"

/**
 * @brief 연결된 클라이언트의 HTTP 요청 하나를 처리합니다.
 *
 * @param fd 클라이언트 연결 socket descriptor입니다.
 */
void doit(int fd);

/**
 * @brief 요청 헤더를 읽고 끝까지 소비합니다.
 *
 * @param rp 클라이언트 연결에 초기화된 Rio 읽기 버퍼입니다.
 */
void read_requesthdrs(rio_t *rp);

/**
 * @brief URI를 정적 파일 경로 또는 CGI 실행 정보로 파싱합니다.
 *
 * @param uri 파싱할 요청 URI입니다.
 * @param filename 해석된 파일 경로를 저장할 버퍼입니다.
 * @param cgiargs CGI 인자를 저장할 버퍼입니다.
 * @return 정적 콘텐츠 요청이면 1, 동적 콘텐츠 요청이면 0을 반환합니다.
 */
int parse_uri(char *uri, char *filename, char *cgiargs);

/**
 * @brief 정적 파일을 HTTP 응답으로 전송합니다.
 *
 * @param fd 클라이언트 연결 socket descriptor입니다.
 * @param filename 전송할 정적 파일 경로입니다.
 * @param filesize 전송할 파일 크기입니다.
 */
void serve_static(int fd, char *filename, int filesize);

/**
 * @brief 파일 이름으로부터 HTTP Content-Type 값을 결정합니다.
 *
 * @param filename MIME type을 판별할 파일 이름입니다.
 * @param filetype 결정된 MIME type을 저장할 버퍼입니다.
 */
void get_filetype(char *filename, char *filetype);

/**
 * @brief CGI 프로그램을 실행해 동적 콘텐츠를 응답합니다.
 *
 * @param fd 클라이언트 연결 socket descriptor입니다.
 * @param filename 실행할 CGI 프로그램 경로입니다.
 * @param cgiargs CGI 프로그램에 전달할 인자 문자열입니다.
 */
void serve_dynamic(int fd, char *filename, char *cgiargs);

/**
 * @brief 클라이언트에 HTTP 오류 응답을 전송합니다.
 *
 * @param fd 클라이언트 연결 socket descriptor입니다.
 * @param cause 오류의 원인이 된 대상입니다.
 * @param errnum HTTP 상태 코드 문자열입니다.
 * @param shortmsg 짧은 오류 메시지입니다.
 * @param longmsg 자세한 오류 메시지입니다.
 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);

/**
 * @brief 지정한 포트에서 Tiny 웹 서버를 시작하고 연결을 반복 처리합니다.
 *
 * @param argc 명령행 인자의 개수입니다.
 * @param argv 첫 번째 인자로 포트를 받는 명령행 인자 배열입니다.
 * @return 정상적으로는 반환하지 않습니다.
 */
int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* 명령행 인자를 검사합니다. */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
}

/**
 * @brief 클라이언트의 요청 라인을 읽고 정적 또는 동적 콘텐츠를 처리합니다.
 *
 * @param fd 클라이언트 연결 socket descriptor입니다.
 */
void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    /* 요청 라인과 헤더를 읽습니다. */
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not implemented",
                    "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);

    /* GET 요청의 URI를 파싱합니다. */
    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    }

    if (is_static) { /* 정적 콘텐츠를 제공합니다. */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
    } else { /* 동적 콘텐츠를 제공합니다. */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }
}

/**
 * @brief HTTP 오류 응답 본문과 헤더를 만들어 클라이언트에 전송합니다.
 *
 * @param fd 클라이언트 연결 socket descriptor입니다.
 * @param cause 오류의 원인이 된 대상입니다.
 * @param errnum HTTP 상태 코드 문자열입니다.
 * @param shortmsg 짧은 오류 메시지입니다.
 * @param longmsg 자세한 오류 메시지입니다.
 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* HTTP 응답 본문을 만듭니다. */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* HTTP 응답을 출력합니다. */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

/**
 * @brief 요청 헤더를 빈 줄이 나올 때까지 읽습니다.
 *
 * @param rp 클라이언트 연결에 초기화된 Rio 읽기 버퍼입니다.
 */
void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

/**
 * @brief URI를 해석해 정적 요청과 동적 요청을 구분합니다.
 *
 * @param uri 파싱할 요청 URI입니다.
 * @param filename 해석된 파일 경로를 저장할 버퍼입니다.
 * @param cgiargs CGI 인자를 저장할 버퍼입니다.
 * @return 정적 콘텐츠 요청이면 1, 동적 콘텐츠 요청이면 0을 반환합니다.
 */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) { /* 정적 콘텐츠입니다. */
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri) - 1] == '/')
            strcat(filename, "home.html");
        return 1;
    } else { /* 동적 콘텐츠입니다. */
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        } else
            strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

/**
 * @brief 정적 파일의 헤더와 본문을 클라이언트에 전송합니다.
 *
 * @param fd 클라이언트 연결 socket descriptor입니다.
 * @param filename 전송할 정적 파일 경로입니다.
 * @param filesize 전송할 파일 크기입니다.
 */
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* 클라이언트에 응답 헤더를 보냅니다. */
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));
    printf("Response headers:\n");
    printf("%s", buf);

    /* 응답 본문을 보냅니다. */
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}

/**
 * @brief 파일 이름 확장자를 기반으로 MIME type을 결정합니다.
 *
 * @param filename MIME type을 판별할 파일 이름입니다.
 * @param filetype 결정된 MIME type을 저장할 버퍼입니다.
 */
void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}

/**
 * @brief CGI 프로그램을 실행해 동적 응답을 생성합니다.
 *
 * @param fd 클라이언트 연결 socket descriptor입니다.
 * @param filename 실행할 CGI 프로그램 경로입니다.
 * @param cgiargs CGI 프로그램에 전달할 인자 문자열입니다.
 */
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* HTTP 응답의 첫 부분을 반환합니다. */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) { /* 자식 프로세스 */
        /* 실제 서버라면 여기서 모든 CGI 변수를 설정합니다. */
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO); /* 표준 출력을 클라이언트로 리다이렉트합니다. */
        Execve(filename, emptylist, environ); /* CGI 프로그램을 실행합니다. */
    }
    Wait(NULL); /* 부모는 기다렸다가 자식 프로세스를 수거합니다. */
}
