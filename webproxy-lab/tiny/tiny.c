/* $begin tinymain */
/*
 * tiny.c - GET 메서드를 사용해 정적/동적 콘텐츠를 제공하는
 *     단순한 반복형 HTTP/1.0 웹 서버입니다.
 *
 * 2019년 11월 droh 업데이트
 *   - serve_static()과 clienterror()의 sprintf() aliasing 문제를 수정했습니다.
 */
#include <stdbool.h>
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


// HTTP 메시지 포맷은 이 글 참고: https://developer.mozilla.org/ko/docs/Web/HTTP/Guides/Messages
/**
 * @brief 지정한 포트에서 Tiny 웹 서버를 시작하고 연결을 반복 처리합니다.
 *
 * @param argc 명령행 인자의 개수입니다.
 * @param argv 첫 번째 인자로 포트를 받는 명령행 인자 배열입니다.
 * @return 정상적으로는 반환하지 않습니다.
 */
int main(int argc, char **argv) {
    const char *program_name = argv[0];
    const char *listen_port = argv[1];

    /* 명령행 인자를 검사합니다. */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", program_name);
        exit(1);
    }

    int listenfd = Open_listenfd((char *) listen_port);

    while (1) {
        struct sockaddr_storage clientaddr;
        socklen_t clientlen = sizeof(clientaddr);
        int connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);

        char hostname[MAXLINE];
        char port[MAXLINE];

        // 연결된 상대 소캣의 정보를 가져옴
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,
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
void doit(int fd) {
    rio_t rio;
    char buf[MAXLINE];

    Rio_readinitb(&rio, fd);
    if (Rio_readlineb(&rio, buf, MAXLINE) <= 0) {
        // \n 까지 읽기 (시작 줄 읽기)
        return;
    }

    printf("Request headers:\n");
    printf("%s", buf);

    char method[MAXLINE];
    char uri[MAXLINE];
    char version[MAXLINE];

    // 시작 줄 분석
    if (sscanf(buf, "%s %s %s", method, uri, version) != 3) {
        clienterror(fd, buf, "400", "Bad Request",
                    "Tiny could not parse the request line");
        return;
    }

    bool is_get_method = (strcasecmp(method, "GET") == 0);
    if (is_get_method == false) {
        clienterror(fd, method, "501", "Not implemented",
                    "Tiny does not implement this method");
        return;
    }

    // 헤더 읽기
    read_requesthdrs(&rio);

    char filename[MAXLINE];
    char cgiargs[MAXLINE];
    int is_static = parse_uri(uri, filename, cgiargs);

    struct stat sbuf;
    bool file_exists = (stat(filename, &sbuf) == 0);
    if (file_exists == false) {
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    }

    if (is_static) {
        bool is_readable_regular_file = (S_ISREG(sbuf.st_mode) && (S_IRUSR & sbuf.st_mode));
        if (is_readable_regular_file == false) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }

        serve_static(fd, filename, sbuf.st_size);
        return;
    }

    bool is_executable_regular_file = (S_ISREG(sbuf.st_mode) && (S_IXUSR & sbuf.st_mode));
    if (is_executable_regular_file == false) {
        clienterror(fd, filename, "403", "Forbidden",
                    "Tiny couldn't run the CGI program");
        return;
    }

    serve_dynamic(fd, filename, cgiargs);
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
                 char *shortmsg, char *longmsg) {
    char body[MAXBUF];

    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    char buf[MAXLINE];

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Content-length: %d\r\n\r\n", (int) strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

/**
 * @brief 요청 헤더를 빈 줄이 나올 때까지 읽습니다. (지금은 읽기만 함)
 *
 * @param rp 클라이언트 연결에 초기화된 Rio 읽기 버퍼입니다.
 */
void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];

    while (1) {
        Rio_readlineb(rp, buf, MAXLINE);

        bool is_header_end = (strcmp(buf, "\r\n") == 0);
        if (is_header_end == true) {
            return;
        }

        printf("%s", buf);
    }
}

/**
 * @brief URI를 해석해 정적 요청과 동적 요청을 구분합니다.
 *
 * @param uri 파싱할 요청 URI입니다.
 * @param filename 해석된 파일 경로를 저장할 버퍼입니다.
 * @param cgiargs CGI 인자를 저장할 버퍼입니다.
 * @return 정적 콘텐츠 요청이면 1, 동적 콘텐츠 요청이면 0을 반환합니다.
 */
int parse_uri(char *uri, char *filename, char *cgiargs) {
    bool is_static_content = (strstr(uri, "cgi-bin") == NULL);
    if (is_static_content == true) {
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);

        if (uri[strlen(uri) - 1] == '/') {
            strcat(filename, "home.html");
        }

        return 1;
    }

    // 동적 콘텐츠 처리: CGI 인자가 있는 경우
    char *query_start = index(uri, '?');
    if (query_start != NULL) {
        strcpy(cgiargs, query_start + 1);
        *query_start = '\0';
    } else {
        strcpy(cgiargs, "");
    }

    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
}

/**
 * @brief 정적 파일의 헤더와 본문을 클라이언트에 전송합니다.
 *
 * @param fd 클라이언트 연결 socket descriptor입니다.
 * @param filename 전송할 정적 파일 경로입니다.
 * @param filesize 전송할 파일 크기입니다.
 */
void serve_static(int fd, char *filename, int filesize) {
    char filetype[MAXLINE];
    char buf[MAXBUF];

    get_filetype(filename, filetype);

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    printf("Response headers:\n");
    printf("%s", buf);

    int srcfd = Open(filename, O_RDONLY, 0);
    char *srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);

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
void get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html")) {
        strcpy(filetype, "text/html");
    } else if (strstr(filename, ".gif")) {
        strcpy(filetype, "image/gif");
    } else if (strstr(filename, ".png")) {
        strcpy(filetype, "image/png");
    } else if (strstr(filename, ".jpg")) {
        strcpy(filetype, "image/jpeg");
    } else if (strstr(filename, ".mp4")) {
        strcpy(filetype, "video/mp4");
    } else {
        strcpy(filetype, "text/plain");
    }
}

/**
 * @brief CGI 프로그램을 실행해 동적 응답을 생성합니다.
 *
 * @param fd 클라이언트 연결 socket descriptor입니다.
 * @param filename 실행할 CGI 프로그램 경로입니다.
 * @param cgiargs CGI 프로그램에 전달할 인자 문자열입니다.
 */
void serve_dynamic(int fd, char *filename, char *cgiargs) {
    char buf[MAXLINE];

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) {
        // 자식 프로세스인 경우만 execve() 를 호출해서 CGI 프로그램을 실행. 이때 fd는 복사됨.
        char *emptylist[] = {NULL};

        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO); // 자식(여기 영역을 실행중인 코드)의 stdout을 fd에 연결. 자식의 stdout 결과가 fd 소켓을 통해 전달 됨.
        Execve(filename, emptylist, environ);
    }

    Wait(NULL);
}
