# Proxy Lab 실행 메모

## 요청 포맷 먼저 보기

아직 `proxy.c`를 구현하지 않았어도 `netcat`으로 프록시에 들어오는 요청 형식을 볼 수 있다.

터미널 1:

```sh
nc -l 4500
```

터미널 2:

```sh
curl -v --proxy http://127.0.0.1:4500 http://www.example.com:8080/index.html
```

터미널 1에는 대략 이런 요청이 보인다:

```http
GET http://www.example.com:8080/index.html HTTP/1.1
Host: www.example.com:8080
User-Agent: curl/...
Accept: */*
Proxy-Connection: Keep-Alive
```

프록시는 첫 줄에서 원본 서버 정보를 뽑아야 한다:

```text
host = www.example.com
port = 8080
path = /index.html
```

## 프록시가 원본 서버에 보내는 요청

클라이언트가 프록시에 보낸 요청:

```http
GET http://www.example.com:8080/index.html HTTP/1.1
```

프록시가 원본 서버 `www.example.com:8080`에 보내야 하는 요청:

```http
GET /index.html HTTP/1.0
Host: www.example.com:8080
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3
Connection: close
Proxy-Connection: close
```

서버 주소는 연결할 때 쓰고, request line에는 `path`만 넣는다.

## 로컬 테스트

Tiny 원본 서버 실행:

```sh
(cd webproxy-lab/tiny && make clean && make && ./tiny 8000)
```

프록시 실행:

```sh
(cd webproxy-lab && make clean && make && ./proxy 4500)
```

프록시를 통해 Tiny에 요청:

```sh
curl -v --proxy http://127.0.0.1:4500 http://127.0.0.1:8000/home.html
```

프록시가 받는 요청:

```http
GET http://127.0.0.1:8000/home.html HTTP/1.1
Host: 127.0.0.1:8000
```

프록시가 Tiny에 보내야 하는 요청:

```http
GET /home.html HTTP/1.0
Host: 127.0.0.1:8000
Connection: close
Proxy-Connection: close
```

## Driver 테스트

전체 테스트를 반복해서 실행할 때:

```sh
(cd webproxy-lab && make clean && make && make -C tiny clean && make -C tiny && ./driver.sh)
```
