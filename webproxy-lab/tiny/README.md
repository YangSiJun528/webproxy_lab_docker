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

## CSAPP 숙제 메모

- 11.6(c) Tiny 요청 로그 확인: 브라우저가 보낸 요청 라인과 헤더를 보고 HTTP 버전을 확인한다.
  - HTTP 1.1 버전을 사용중임
  - ```
    GET / HTTP/1.1
    Host: localhost:8000
    Connection: keep-alive
    ...
    ```
- 11.7 MPG 파일 제공: Tiny가 MPG 동영상 파일을 정적 콘텐츠로 제공하도록 MIME type 처리를 추가한다.
  - 

- 11.9 정적 파일 전송 방식 변경: `mmap` 대신 `malloc`, `Rio_readn`, `Rio_writen`으로 파일을 읽어 전송한다.
  메모:

- 11.10(a) adder HTML 폼 작성: 두 숫자를 입력받아 GET 방식으로 CGI adder에 전달하는 HTML form을 만든다.
  메모:

- 11.10(b) adder 폼 동작 확인: 브라우저에서 폼을 제출하고 adder가 생성한 동적 콘텐츠를 확인한다.
  메모:
