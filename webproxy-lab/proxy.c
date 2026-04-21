#include <stdbool.h>
#include <stdio.h>
#include "csapp.h"

/* 권장 최대 캐시 크기와 객체 크기 */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* 이 긴 줄을 코드에 포함해도 style 점수는 깎이지 않습니다. */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

/**
 * @brief 지정한 포트에서 프록시 웹 서버를 시작하고 연결을 반복 처리.
 *
 * @param argc 명령행 인자의 개수, 항상 2개여야 함
 * @param argv 포트 번호를 받는다
 * @return 프로그램 종료됨
 */
int main(int argc, char **argv) {
    const char *program_name = argv[0];
    const char *listen_port = argv[1];
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
    return 0;
}

void print_full_html(rio_t rio, char buf[8192]) {
    // HTTP 요청의 끝("\r\n")일 때까지 출력
    printf(">>> REQ START==================================\n");
    while (Rio_readlineb(&rio, buf, MAXLINE) > 0) {
        printf("%s", buf);
        if (strcmp(buf, "\r\n") == 0) {
            break;
        }
    }
    printf(">>> REQ END==================================\n");
    fflush(stdout);
}

/**
 * @brief 클라이언트로부터 들어온 요청을 HTTP 헤더 끝(\r\n)까지 읽어 stdout에 출력.
 *
 * @param fd 클라이언트 커넥션 socket descriptor
 */
void doit(int fd) {
    rio_t rio;
    char buf[MAXLINE];

    Rio_readinitb(&rio, fd);
    if (Rio_readlineb(&rio, buf, MAXLINE) <= 0) {
        // \n 까지 읽기 (시작 줄 읽기)
        return;
    }

    char method[MAXLINE];
    char uri[MAXLINE];
    char version[MAXLINE];

    // 시작 줄 분석
    if (sscanf(buf, "%s %s %s", method, uri, version) != 3) {
        clienterror(fd, buf, "400", "Bad Request",
                    "Tiny could not parse the request line");
        return;
    }

    // 현재는 GET 만 지원함
    bool is_get_method = (strcasecmp(method, "GET") == 0);
    if (is_get_method == false) {
        clienterror(fd, method, "501", "Not implemented",
                    "Tiny does not implement this method");
        return;
    }

    //TODO: libyuarel 사용해서 첫 줄 읽어서 정상적인 프록시 요청인지 파악하기.



    //TODO: 헤더 읽기
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