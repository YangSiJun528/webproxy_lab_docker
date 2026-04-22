#include <stdbool.h>
#include <stdio.h>
#include "yuarel.h"
#include "csapp.h"

/* 권장 최대 캐시 크기와 객체 크기 */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* 이 긴 줄을 코드에 포함해도 style 점수는 깎이지 않습니다. */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);
void *handle_client_thread(void *vargp);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void create_proxy_requesthdrs(rio_t *rp, const struct yuarel *url, char *out_proxy_hdrs);
static bool is_header_name(const char *line, const char *name);
static void append_header(char *headers, const char *line);
static void create_default_host_header(const struct yuarel *url, char *host_line);
static void create_request_line(const struct yuarel *url, char *request_line);

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
        int *connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);

        char hostname[MAXLINE];
        char port[MAXLINE];

        // 연결된 상대 소캣의 정보를 가져옴
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);

        pthread_t tid;
        Pthread_create(&tid, NULL, handle_client_thread, connfdp);
    }
    return 0;
}

void *handle_client_thread(void *vargp) {
    int connfd = *((int *) vargp);
    Free(vargp);

    Pthread_detach(Pthread_self());
    doit(connfd);
    Close(connfd);

    return NULL;
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
    char raw_uri[MAXLINE];
    char version[MAXLINE];

    // 시작 줄 분석
    if (sscanf(buf, "%s %s %s", method, raw_uri, version) != 3) {
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

    struct yuarel url = {0};

    bool is_succes_url_parse = yuarel_parse(&url, raw_uri) != -1;
    if (is_succes_url_parse == false || url.host == NULL)
    {
        clienterror(fd, method, "400", "Bad Request",
                    "TODO - proxy 요청 아님");
        return;
    }

    // printf("Struct values:\n");
    // printf("\tscheme:\t\t%s\n", url.scheme);
    // printf("\thost:\t\t%s\n", url.host);
    // printf("\tport:\t\t%d\n", url.port);
    // printf("\tpath:\t\t%s\n", url.path);
    // printf("\tquery:\t\t%s\n", url.query);
    // printf("\tfragment:\t%s\n", url.fragment);

    char proxy_hdrs[MAXBUF];
    create_proxy_requesthdrs(&rio, &url, proxy_hdrs);

    //TODO: 이거 이름 target_ 말고 다른거 없나
    int target_port_int = url.port == 0 ? 80 : url.port;
    int clientfd;
    char target_buf[MAXBUF];
    rio_t target_rio;

    char origin_port[MAXLINE];
    sprintf(origin_port, "%d", target_port_int);
    clientfd = Open_clientfd(url.host, origin_port);

    char request_line[MAXLINE];
    create_request_line(&url, request_line);
    Rio_writen(clientfd, request_line, strlen(request_line));
    Rio_writen(clientfd, proxy_hdrs, strlen(proxy_hdrs));

    Rio_readinitb(&target_rio, clientfd);
    ssize_t n;
    while ((n = Rio_readnb(&target_rio, target_buf, MAXBUF)) > 0) {
        Rio_writen(fd, target_buf, n);
    }

    Close(clientfd);
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
 * @brief 클라이언트 요청 헤더를 읽고 원본 서버로 보낼 프록시 요청 헤더를 만듭니다.
 *
 * @param rp 클라이언트 연결에 초기화된 Rio 읽기 버퍼입니다.
 * @param url request line에서 파싱한 URL 정보입니다.
 * @param out_proxy_hdrs 생성된 헤더 문자열을 저장할 출력 버퍼입니다.
 */
void create_proxy_requesthdrs(rio_t *rp, const struct yuarel *url, char *out_proxy_hdrs) {
    char buf[MAXLINE];
    char host_line[MAXLINE];

    out_proxy_hdrs[0] = '\0';
    create_default_host_header(url, host_line);

    while (Rio_readlineb(rp, buf, MAXLINE) > 0) {
        if (strcmp(buf, "\r\n") == 0) {
            break;
        }

        if (is_header_name(buf, "Host")) {
            strcpy(host_line, buf);
            continue;
        }

        //이미 있는 경우 스킵
        if (is_header_name(buf, "User-Agent") ||
            is_header_name(buf, "Connection") ||
            is_header_name(buf, "Proxy-Connection")) {
            continue;
        }

        append_header(out_proxy_hdrs, buf);
    }

    // 마지막에 필수 헤더 추가 후 헤더 종료
    append_header(out_proxy_hdrs, host_line);
    append_header(out_proxy_hdrs, user_agent_hdr);
    append_header(out_proxy_hdrs, "Connection: close\r\n");
    append_header(out_proxy_hdrs, "Proxy-Connection: close\r\n");
    append_header(out_proxy_hdrs, "\r\n");
}

static bool is_header_name(const char *line, const char *name) {
    size_t name_len = strlen(name);

    return strncasecmp(line, name, name_len) == 0 && line[name_len] == ':';
}

static void append_header(char *headers, const char *line) {
    if (strlen(headers) + strlen(line) < MAXBUF) {
        strcat(headers, line);
    }
}

static void create_default_host_header(const struct yuarel *url, char *host_line) {
    if (url->host == NULL) {
        strcpy(host_line, "Host: \r\n");
        return;
    }

    if (url->port > 0 && url->port != 80) {
        snprintf(host_line, MAXLINE, "Host: %s:%d\r\n", url->host, url->port);
    } else {
        snprintf(host_line, MAXLINE, "Host: %s\r\n", url->host);
    }
}

static void create_request_line(const struct yuarel *url, char *request_line) {
    const char *path = url->path == NULL ? "" : url->path;

    if (url->query != NULL) {
        snprintf(request_line, MAXLINE, "GET /%s?%s HTTP/1.0\r\n", path, url->query);
    } else {
        snprintf(request_line, MAXLINE, "GET /%s HTTP/1.0\r\n", path);
    }
}
