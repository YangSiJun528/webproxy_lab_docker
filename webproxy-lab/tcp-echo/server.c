#include "csapp.h"

void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd); // connfd에 대한 버퍼링된 읽기 상태 초기화

    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int) n);
        Rio_writen(connfd, buf, n); // 읽은 내용을 그대로 클라이언트에 다시 보냄
    }
}

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; // IPv4/IPv6 어떤 주소도 담을 수 있는 클라이언트 주소 버퍼
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]); // 지정한 포트에서 연결을 기다리는 리스닝 소켓 생성

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);

        // 클라이언트 연결 요청을 수락하고, 해당 클라이언트와 통신할 새 연결 소켓(connfd)을 반환
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);

        // 클라이언트 주소를 사람이 읽을 수 있는 호스트명/포트 문자열로 변환
        Getnameinfo((SA *) &clientaddr, clientlen,
                    client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);

        printf("Connected to (%s, %s)\n", client_hostname, client_port);

        echo(connfd); // 이 클라이언트가 보낸 데이터를 읽어 그대로 다시 전송
        Close(connfd); // 클라이언트 처리 종료 후 연결 닫기
    }
}
