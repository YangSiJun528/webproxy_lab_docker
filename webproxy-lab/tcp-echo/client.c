#include "csapp.h"

//아니 이거 구현 이미 되어있는거였네;; 삽질 이슈 - 이거 볼 필요 없음
int open_listenfd(char *port) {
    // Designated Initializer in C 사용 - 나머지 기본값은 binary 0
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC, /* IPv4/IPv6 둘 다 허용 */
        .ai_socktype = SOCK_STREAM, /* TCP 연결 */
        .ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV,
        /* AI_PASSIVE : 서버가 자기 자신에 bind할 주소 후보를 반환 */
        /* AI_ADDRCONFIG  : 실제 설정된 IP 계열만 반환 */
        /* AI_NUMERICSERV : port는 숫자 문자열로만 해석 */
    };

    struct addrinfo *candidates = NULL;
    Getaddrinfo(NULL, port, &hints, &candidates); // 이 포트에 바인드 가능한 로컬 주소 후보 목록 조회

    int listenfd = -1;
    int reuse_addr_bool = 1;
    for (struct addrinfo *p = candidates; p != NULL; p = p->ai_next) {
        listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listenfd < 0) {
            continue; // 이 후보로는 소켓 생성 실패 → 다음 후보
        }

        // "Address already in use" 방지
        // TCP 연결이 끊어져도 어느정도 TIME_WAIT 상태가 유지되는데, 이 상태에서 연결을 시도해도 실패하지 않게 하는 옵션이 SO_REUSEADDR임
        // setsockopt()는 범용 API라서 옵션 값을 직접 받지 않고, 값이 저장된 메모리의 주소와 크기를 함께 받는다. - SO_REUSEADDR는 bool이지만 구조체가 필요한 경우도 있으므로
        Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr_bool, sizeof(reuse_addr_bool));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) {
            break; // 바인딩 성공 → 탐색 종료
        }

        Close(listenfd); // 바인딩 실패 → 정리하고 다음 후보
        listenfd = -1;
    }

    Freeaddrinfo(candidates); // 커넥션 or 모든 후보 순회 후 주소 목록 해제

    if (listenfd < 0)
        return -1; // 바인딩 가능한 주소가 하나도 없었으면 실패

    if (listen(listenfd, LISTENQ) < 0) {
        // listen에서 에러 발생 시 실패
        Close(listenfd);
        return -1;
    }

    return listenfd;
}

// 이거만 보면 됨.
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    char *host = argv[1]; // 접속할 서버 호스트명 또는 IP 주소
    char *port = argv[2]; // 접속할 서버 포트 번호 문자열
    int clientfd;
    char buf[MAXLINE];
    rio_t rio;

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) { // 표준 입력에서 한 줄씩 읽음, EOF면 종료
        Rio_writen(clientfd, buf, strlen(buf));  // 입력한 줄을 서버에 그대로 전송 - strlen이라 '\0' 은 포함 안됨
        Rio_readlineb(&rio, buf, MAXLINE);   // 서버가 돌려준 한 줄 응답을 읽음
        Fputs(buf, stdout);                         // 응답을 화면에 출력
    }

    Close(clientfd);
    exit(0);
}
