# Tiny

## 실행

devcontainer 내부 터미널에서, 저장소 루트에서 실행:

```sh
cd webproxy-lab/tiny
make clean
make
./tiny 8000
```

이미 `webproxy-lab/tiny` 안에 있으면 `cd webproxy-lab/tiny` 는 생략합니다.

## 브라우저

호스트 브라우저에서 포트 `8000`으로 접속:
`http://localhost:8000/`

adder 확인:
`http://localhost:8000/cgi-bin/adder?n1=1&n2=2`

## 연결 확인

### telnet

devcontainer 내부 터미널에서 실행:

```sh
telnet 127.0.0.1 8000
```

연결 후 입력:

```text
GET / HTTP/1.0

```

### curl

devcontainer 내부 터미널에서 실행:

```sh
curl -i http://127.0.0.1:8000/
```

## adder 확인

devcontainer 내부 터미널에서 실행:

```sh
curl -i "http://127.0.0.1:8000/cgi-bin/adder?n1=1&n2=2"
```
