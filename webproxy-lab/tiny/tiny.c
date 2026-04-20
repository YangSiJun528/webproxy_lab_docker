/* $begin tinymain */
/*
 * tiny.c - GET method로 정적/동적 콘텐츠를 제공하는 단순한 반복형
 *     HTTP/1.0 Web server입니다.
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
 * @brief 요청 헤더를 읽고 소비합니다.
 *
 * @param rp 클라이언트 연결에 초기화된 Rio 읽기 버퍼입니다.
 */
void read_requesthdrs(rio_t *rp);
/**
 * @brief URI를 파일 이름과 CGI 인자로 분리합니다.
 *
 * @param uri 파싱할 요청 URI입니다.
 * @param filename URI에 대응하는 파일 경로를 저장할 버퍼입니다.
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
 * @brief 파일 이름에 맞는 HTTP Content-Type 값을 결정합니다.
 *
 * @param filename MIME type을 판단할 파일 이름입니다.
 * @param filetype 결정된 MIME type을 저장할 버퍼입니다.
 */
void get_filetype(char *filename, char *filetype);
/**
 * @brief CGI 프로그램을 실행해 동적 콘텐츠 응답을 생성합니다.
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
 * @param cause 오류 원인이 된 요청 대상입니다.
 * @param errnum HTTP 상태 코드 문자열입니다.
 * @param shortmsg 짧은 오류 메시지입니다.
 * @param longmsg 자세한 오류 메시지입니다.
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

/**
 * @brief 지정한 port에서 반복형 Tiny Web server를 시작하고 들어오는 연결을 처리합니다.
 *
 * @param argc 명령행 인자의 개수입니다.
 * @param argv 첫 번째 인자로 listen할 port를 받는 명령행 인자 배열입니다.
 * @return 정상적으로는 반환하지 않습니다.
 */
int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* 명령행 인자를 검사합니다. */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}
