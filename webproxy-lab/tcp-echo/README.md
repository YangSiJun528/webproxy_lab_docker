# TCP echo

TCP echo 서버/클라이언트 테스트용 하위 프로젝트입니다.

서버는 클라이언트가 보낸 한 줄을 읽고 그대로 다시 보냅니다. 클라이언트는 표준 입력에서 한 줄씩 읽어 서버에 보내고, 서버가 돌려준 echo 응답을 표준 출력에 출력합니다.

## Build

```sh
cd webproxy-lab/tcp-echo
make clean
make
```

루트 Makefile을 통해 빌드할 수도 있습니다.

```sh
cd webproxy-lab
make tcp-echo-clean
make tcp-echo
```

## Run in two terminals

터미널 1:

```sh
cd webproxy-lab/tcp-echo
make server-run PORT=9000
```

서버는 연결을 기다리다가 클라이언트가 접속하면 다음과 비슷한 로그를 출력합니다.

```text
Connected to (localhost, 53210)
server received 6 bytes
```

터미널 2:

```sh
cd webproxy-lab/tcp-echo
make client-run HOST=127.0.0.1 PORT=9000
```

클라이언트가 실행되면 입력을 기다립니다. 문자열을 입력하고 Enter를 누르면 같은 문자열이 다시 출력되어야 합니다.

```text
hello
hello
```

입력을 끝내려면 클라이언트 터미널에서 `Ctrl-D`를 누릅니다. 서버는 계속 다음 연결을 기다리므로 중지하려면 서버 터미널에서 `Ctrl-C`를 누릅니다.

## One-shot client input

클라이언트 입력을 직접 타이핑하지 않고 한 번에 보낼 수도 있습니다.

```sh
printf "hello\nworld\n" | make client-run HOST=127.0.0.1 PORT=9000
```

예상 출력:

```text
hello
world
```

## Run from project root

루트 Makefile 타깃을 쓰면 `tcp-echo` 디렉터리로 직접 이동하지 않아도 됩니다.

터미널 1:

```sh
cd webproxy-lab
make tcp-echo-server-run TCP_ECHO_PORT=9000
```

터미널 2:

```sh
cd webproxy-lab
make tcp-echo-client-run TCP_ECHO_PORT=9000
```

## Troubleshooting

`Address already in use`가 나오면 이미 같은 port를 쓰는 프로세스가 있는 상태입니다. 다른 port를 쓰거나 기존 서버를 종료하세요.

`undefined reference to Open_clientfd`, `undefined reference to Rio_writen`처럼 `csapp` 함수 링크 오류가 나오면 `csapp.o`가 함께 링크되지 않은 것입니다. 이 프로젝트에서는 `make clean && make`를 사용하면 `csapp.c`까지 같이 빌드됩니다.

클라이언트가 아무것도 출력하지 않는 것처럼 보이면 입력 대기 중일 수 있습니다. 문자열을 입력하고 Enter를 누르거나, `printf "hello\n" | make client-run ...` 형태로 테스트하세요.
