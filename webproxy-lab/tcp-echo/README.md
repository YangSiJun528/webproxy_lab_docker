# TCP echo

TCP echo 서버/클라이언트를 직접 구현하기 위한 빈 스캐폴드입니다.

현재 `server.c`와 `client.c`는 TODO만 포함하고 실제 socket 로직은 비어 있습니다.

## Build

```sh
make
```

## Run

터미널 1:

```sh
make server-run PORT=9000
```

터미널 2:

```sh
make client-run HOST=127.0.0.1 PORT=9000
```
