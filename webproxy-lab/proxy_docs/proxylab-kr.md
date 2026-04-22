# 15-213, Fall 20XX
## Proxy Lab: Caching Web Proxy 작성하기

- **배포일(Assigned):** Thu, Nov 19
- **마감일(Due):** Tue, Dec 8, 11:59 PM
- **제출 가능한 마지막 시각(Last Possible Time to Turn In):** Fri, Dec 11, 11:59 PM

---

## 1. 소개(Introduction)

웹 프록시(Web proxy)는 웹 브라우저(Web browser)와 최종 서버(end server) 사이에서 중간자 역할을 하는 프로그램이다. 브라우저는 웹 페이지(Web page)를 얻기 위해 최종 서버에 직접 접속하는 대신 프록시에 접속하고, 프록시는 그 요청(request)을 최종 서버로 전달한다. 최종 서버가 프록시에 응답(reply)하면, 프록시는 그 응답을 브라우저로 보낸다.

프록시(proxy)는 여러 목적으로 유용하다. 때로는 방화벽(firewall)에서 프록시를 사용하여, 방화벽 뒤에 있는 브라우저가 방화벽 너머의 서버에 오직 프록시를 통해서만 접속하도록 만들 수 있다. 프록시는 익명화 도구(anonymizer) 역할도 할 수 있다. 요청에서 식별 가능한 정보를 모두 제거하면, 프록시는 웹 서버(Web server)에 대해 브라우저를 익명으로 만들 수 있다. 또한 프록시는 서버의 객체(object)를 로컬 복사본(local copy)으로 저장한 뒤, 이후 요청에 대해 원격 서버와 다시 통신하는 대신 캐시(cache)에서 읽어 응답함으로써 웹 객체(web object)를 캐시하는 데에도 사용할 수 있다.

이 lab에서는 웹 객체를 캐시하는 간단한 HTTP 프록시(HTTP proxy)를 작성한다.

- 첫 번째 파트에서는 다음을 수행한다:
  - 들어오는 연결(connection)을 수락한다
  - 요청을 읽고 파싱(parse)한다
  - 요청을 웹 서버로 전달한다
  - 서버의 응답(response)을 읽는다
  - 해당 응답을 대응되는 클라이언트(client)로 전달한다

이 첫 번째 파트에서는 다음을 배우게 된다:

- 기본적인 HTTP 동작
- 네트워크 연결(network connection)을 통해 통신하는 프로그램을 작성하기 위해 소켓(socket)을 사용하는 방법

두 번째 파트에서는 프록시를 업그레이드하여 여러 동시 연결(concurrent connection)을 처리하도록 만든다. 이 과정에서 중요한 시스템 개념인 동시성(concurrency)을 다루게 된다.

세 번째이자 마지막 파트에서는 최근 접근한 웹 콘텐츠(web content)를 담는 간단한 주 메모리 캐시(main memory cache)를 사용하여 프록시에 캐싱(caching)을 추가한다.

---

## 2. 운영 방식(Logistics)

이 프로젝트는 개인 프로젝트이다.

---

## 3. Handout 지침

> **SITE-SPECIFIC:** 강사가 `proxylab-handout.tar` 파일을 학생들에게 어떻게 배포할지 설명하는 문단을 여기에 삽입한다.

작업할 Linux 머신의 보호된 디렉터리(protected directory)에 handout 파일을 복사한 다음, 다음 명령을 실행한다:

```sh
linux> tar xvf proxylab-handout.tar
```

그러면 `proxylab-handout`이라는 handout 디렉터리가 생성된다. `README` 파일은 여러 파일에 대해 설명한다.

---

## 4. Part I: 순차적 웹 프록시(sequential web proxy) 구현하기

첫 번째 단계는 HTTP/1.0 `GET` 요청을 처리하는 기본 순차적 프록시(sequential proxy)를 구현하는 것이다. `POST`와 같은 다른 요청 타입(request type)은 전적으로 선택 사항이다.

프록시가 시작되면, 명령줄(command line)에 지정된 포트(port) 번호에서 들어오는 연결을 기다려야 한다. 연결이 성립되면 프록시는 다음을 수행해야 한다:

1. 클라이언트로부터 요청 전체를 읽는다
2. 요청을 파싱한다
3. 클라이언트가 유효한 HTTP 요청을 보냈는지 판단한다
4. 적절한 웹 서버에 자체 연결을 수립한다
5. 클라이언트가 지정한 객체를 요청한다
6. 서버의 응답을 읽는다
7. 그 응답을 클라이언트로 전달한다

### 4.1 HTTP/1.0 GET 요청

최종 사용자(end user)가 웹 브라우저의 주소 표시줄(address bar)에 다음과 같은 URL을 입력하면:

```text
http://www.cmu.edu/hub/index.html
```

브라우저는 다음과 비슷한 줄로 시작하는 HTTP 요청을 프록시에 보낸다:

```http
GET http://www.cmu.edu/hub/index.html HTTP/1.1
```

이 경우 프록시는 요청을 최소한 다음 필드(field)로 파싱해야 한다:

* **hostname:** `www.cmu.edu`
* **path/query:** `/hub/index.html`

이렇게 하면 프록시는 `www.cmu.edu`로 연결을 열어야 한다는 것과, 다음 형태의 줄로 시작하는 자체 HTTP 요청을 보내야 한다는 것을 판단할 수 있다:

```http
GET /hub/index.html HTTP/1.0
```

참고:

* HTTP 요청의 모든 줄은 캐리지 리턴(carriage return, `\r`) 뒤에 줄바꿈(newline, `\n`)이 오는 형태로 끝난다.
* 모든 HTTP 요청은 빈 줄로 종료된다:

```text
"\r\n"
```

위 예시에서 웹 브라우저의 요청 줄(request line)은 `HTTP/1.1`로 끝나지만, 프록시의 요청 줄은 `HTTP/1.0`으로 끝난다는 점에 주목해야 한다.

현대 웹 브라우저는 HTTP/1.1 요청을 생성하지만, 여러분의 프록시는 이를 처리하고 HTTP/1.0 요청으로 전달해야 한다.

HTTP 요청은, HTTP/1.0 `GET` 요청의 부분집합만 보더라도 매우 복잡할 수 있다는 점을 고려하는 것이 중요하다. 교재는 HTTP 트랜잭션(HTTP transaction)의 특정 세부 사항을 설명하지만, 완전한 HTTP/1.0 명세(specification)는 **RFC 1945**를 참조해야 한다.

이상적으로 HTTP 요청 파서(parser)는 RFC 1945의 관련 섹션에 따라 완전히 견고(robust)해야 한다. 단, 한 가지 예외가 있다. 명세는 여러 줄로 된 요청 필드(multiline request field)를 허용하지만, 여러분의 프록시는 이를 올바르게 처리할 필요는 없다.

물론 여러분의 프록시는 잘못된 형식의 요청(malformed request) 때문에 조기에 중단(abort)되어서는 안 된다.

### 4.2 요청 헤더(Request headers)

이 lab에서 중요한 요청 헤더(request header)는 다음과 같다:

* `Host`
* `User-Agent`
* `Connection`
* `Proxy-Connection`

#### Host header

항상 `Host` 헤더를 보낸다.

이 동작은 엄밀히 말해 HTTP/1.0 명세에서 허용된 것은 아니지만, 특정 웹 서버, 특히 가상 호스팅(virtual hosting)을 사용하는 서버에서 정상적인 응답을 얻어내기 위해 필요하다.

예:

```http
Host: www.cmu.edu
```

브라우저가 이미 `Host` 헤더를 보낸 경우, 프록시는 브라우저와 동일한 `Host` 헤더를 사용해야 한다.

#### User-Agent header

다음 `User-Agent` 헤더를 항상 보내도록 선택할 수 있다:

```http
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3
```

writeup에서는 서식 때문에 헤더가 두 줄로 보이지만, 프록시는 이를 **한 줄(single line)** 로 보내야 한다.

`User-Agent` 헤더는 클라이언트를 식별하며, 웹 서버는 제공할 콘텐츠(content)를 조정하기 위해 이 식별 정보를 사용하는 경우가 많다.

이 특정 `User-Agent` 문자열(string)을 보내면 간단한 telnet 스타일 테스트에서 얻는 응답의 내용과 다양성이 개선될 수 있다.

#### Connection header

항상 다음을 보낸다:

```http
Connection: close
```

#### Proxy-Connection header

항상 다음을 보낸다:

```http
Proxy-Connection: close
```

`Connection` 및 `Proxy-Connection` 헤더는 첫 번째 요청/응답 교환(request/response exchange)이 완료된 뒤 연결을 유지할지 여부를 지정한다.

프록시가 각 요청마다 새 연결을 여는 것은 허용되며, 권장된다.

이 헤더들의 값을 `close`로 지정하면, 프록시가 첫 번째 요청/응답 교환 이후 연결을 닫으려 한다는 사실을 웹 서버에 알릴 수 있다.

편의를 위해 `User-Agent` 헤더 값은 `proxy.c`에 문자열 상수(string constant)로 제공되어 있다.

마지막으로, 브라우저가 HTTP 요청의 일부로 추가 요청 헤더를 보낸다면, 프록시는 이를 변경하지 않고 전달해야 한다.

### 4.3 포트 번호(Port numbers)

이 lab에는 중요한 포트 번호 부류가 두 가지 있다:

* HTTP 요청 포트(HTTP request port)
* 프록시의 리스닝 포트(listening port)

#### HTTP request port

HTTP 요청 포트는 HTTP 요청의 URL에 포함될 수 있는 선택적 필드(optional field)이다.

예를 들면:

```text
http://www.cmu.edu:8080/hub/index.html
```

이 경우 프록시는 기본 HTTP 포트(default HTTP port) `80` 대신 `8080` 포트에서 호스트(host) `www.cmu.edu`에 연결해야 한다.

URL에 포트 번호가 포함되어 있든 없든 프록시는 올바르게 동작해야 한다.

#### Listening port

리스닝 포트는 프록시가 들어오는 연결을 기다리는 포트이다.

프록시는 리스닝 포트 번호를 지정하는 명령줄 인자(command line argument)를 받아야 한다.

예:

```sh
linux> ./proxy 15213
```

다음 조건을 만족하는 임의의 비특권 리스닝 포트(non-privileged listening port)를 선택할 수 있다:

* `1024`보다 크고
* `65536`보다 작다

또한 다른 프로세스(process)가 사용 중이어서는 안 된다.

각 프록시는 고유한 리스닝 포트를 사용해야 하고, 많은 사람이 각 머신에서 동시에 작업할 것이므로, 개인용 포트 번호를 고르는 데 도움을 주기 위해 `port-for-user.pl` 스크립트(script)가 제공된다.

예:

```sh
linux> ./port-for-user.pl droh
droh: 45806
```

`port-for-user.pl`이 반환하는 포트 `p`는 항상 짝수이다.

따라서 Tiny 서버에 사용할 추가 포트 번호가 필요하다면, 포트 `p`와 `p + 1`을 안전하게 사용할 수 있다.

임의의 랜덤 포트를 직접 고르지 말라. 그렇게 하면 다른 사용자를 방해할 위험이 있다.

---

## 5. Part II: 여러 동시 요청(concurrent request) 다루기

동작하는 순차적 프록시를 갖게 되면, 여러 요청을 동시에 처리하도록 이를 수정해야 한다.

동시 서버(concurrent server)를 구현하는 가장 간단한 방법은 새 연결 요청마다 새 스레드(thread)를 생성하는 것이다. 교재 12.5.5절에서 설명하는 사전 스레드 서버(prethreaded server)와 같은 다른 설계도 가능하다.

참고:

* 메모리 누수(memory leak)를 피하기 위해 스레드는 **detached mode**로 실행되어야 한다.
* CS:APP3e 교재에서 설명하는 `open_clientfd`와 `open_listenfd` 함수는 현대적이고 프로토콜 독립적인(protocol-independent) `getaddrinfo` 함수를 기반으로 하므로 스레드 안전(thread safe)하다.

---

## 6. Part III: 웹 객체 캐싱(Caching web objects)

lab의 마지막 파트에서는 최근 사용된 웹 객체를 메모리에 저장하는 캐시를 프록시에 추가한다.

HTTP는 실제로 웹 서버가 자신이 제공하는 객체를 어떻게 캐시해야 하는지 지시하고, 클라이언트가 자신을 대신해 캐시를 어떻게 사용해야 하는지 지정할 수 있는 꽤 복잡한 모델(model)을 정의한다. 그러나 여러분의 프록시는 단순화된 접근 방식을 채택한다.

프록시가 서버로부터 웹 객체를 받으면, 그 객체를 클라이언트로 전송하는 동시에 메모리에 캐시해야 한다. 다른 클라이언트가 같은 서버의 같은 객체를 요청하면, 프록시는 서버에 다시 연결할 필요 없이 캐시된 객체를 그대로 다시 보낼 수 있다.

프록시는 모든 객체를 영원히 캐시할 수 없으므로 다음 두 가지를 모두 강제해야 한다:

* 최대 캐시 크기(maximum cache size)
* 최대 캐시 객체 크기(maximum cache object size)

### 6.1 최대 캐시 크기(Maximum cache size)

프록시 캐시 전체는 다음 최대 크기를 가져야 한다:

```c
MAX_CACHE_SIZE = 1 MiB
```

캐시 크기를 계산할 때 프록시는 실제 웹 객체를 저장하는 데 사용된 바이트(byte)만 세야 한다. 메타데이터(metadata)를 포함한 그 밖의 바이트는 무시해야 한다.

### 6.2 최대 객체 크기(Maximum object size)

프록시는 다음 최대 크기를 넘지 않는 웹 객체만 캐시해야 한다:

```c
MAX_OBJECT_SIZE = 100 KiB
```

편의를 위해 두 크기 제한(size limit)은 모두 `proxy.c`에 매크로(macro)로 제공되어 있다.

올바른 캐시를 구현하는 가장 쉬운 방법은 각 활성 연결(active connection)에 대해 버퍼(buffer)를 할당하고, 서버로부터 데이터를 받을 때마다 누적하는 것이다.

* 버퍼 크기가 최대 객체 크기를 넘는 순간, 해당 버퍼는 버릴 수 있다.
* 최대 객체 크기를 넘기 전에 웹 서버의 응답 전체를 읽었다면, 그 객체는 캐시할 수 있다.

이 방식을 사용하면 프록시가 웹 객체에 대해 사용하는 데이터의 최대량은 다음과 같다:

```c
MAX_CACHE_SIZE + T * MAX_OBJECT_SIZE
```

여기서 `T`는 활성 연결의 최대 개수이다.

### 6.3 축출 정책(Eviction policy)

프록시의 캐시는 최소 최근 사용(least-recently-used, LRU) 축출 정책(eviction policy)에 가까운 정책을 사용해야 한다.

엄격하게 LRU일 필요는 없지만, 합리적으로 그에 가까워야 한다.

객체를 읽는 것과 쓰는 것 모두 객체를 사용하는 것으로 간주한다.

### 6.4 동기화(Synchronization)

캐시에 대한 접근(access)은 스레드 안전(thread-safe)해야 하며, 캐시 접근에서 경쟁 상태(race condition)를 없애는 것이 이 lab 파트에서 더 흥미로운 측면일 가능성이 높다.

여러 스레드가 캐시에서 동시에 읽을 수 있어야 한다는 특별한 요구사항이 있다.

* 여러 reader: **허용**
* 한 번에 하나의 writer만: **허용**
* 모든 것에 대해 하나의 큰 배타 락(exclusive lock)을 사용하는 방식: **허용되지 않음**

가능한 접근 방식:

* 캐시 파티셔닝(partitioning)
* Pthreads readers-writers locks 사용
* 세마포어(semaphore)를 사용해 직접 readers-writers solution 구현

엄격한 LRU 축출 정책을 구현하지 않아도 된다는 사실은 여러 reader를 지원하는 데 어느 정도 유연성을 제공한다.

---

## 7. 평가(Evaluation)

이 과제는 총 **70점** 만점으로 채점된다:

* **BasicCorrectness:** 기본 프록시 동작에 대해 40점(autograded)
* **Concurrency:** 동시 요청 처리에 대해 15점(autograded)
* **Cache:** 동작하는 캐시에 대해 15점(autograded)

### 7.1 자동 채점(Autograding)

handout 자료에는 `driver.sh`라는 autograder가 포함되어 있으며, 강사는 이를 사용해 다음 항목의 점수를 부여한다:

* `BasicCorrectness`
* `Concurrency`
* `Cache`

`proxylab-handout` 디렉터리에서:

```sh
linux> ./driver.sh
```

driver는 Linux 머신에서 실행해야 한다.

### 7.2 견고성(Robustness)

늘 그렇듯이, 오류(error)는 물론 잘못되었거나 악의적인 입력(malformed or malicious input)에도 견고한 프로그램을 제출해야 한다.

서버는 일반적으로 장기 실행 프로세스(long-running process)이며, 웹 프록시도 예외가 아니다. 장기 실행 프로세스가 여러 종류의 오류에 어떻게 반응해야 하는지 신중히 생각하라.

많은 종류의 오류에 대해 프록시가 즉시 종료(exit)되는 것은 분명히 부적절하다.

견고성은 또한 다음을 의미한다:

* segmentation fault가 없어야 함
* 메모리 누수가 없어야 함
* 파일 디스크립터(file descriptor) 누수가 없어야 함

---

## 8. 테스트와 디버깅(Testing and debugging)

간단한 autograder 외에는 구현을 테스트할 샘플 입력(sample input)이나 테스트 프로그램(test program)이 제공되지 않는다.

코드를 디버깅하고 올바른 구현인지 판단하기 위해 직접 테스트를 만들고, 어쩌면 직접 테스트 하네스(testing harness)도 만들어야 한다.

이는 실제 환경(real world)에서 가치 있는 기술이다. 실제 환경에서는 정확한 실행 조건(operating condition)을 거의 알 수 없고, 참조 솔루션(reference solution)이 없는 경우도 많다.

다행히 프록시를 디버깅하고 테스트하는 데 사용할 수 있는 도구가 많다. 모든 코드 경로(code path)를 실행해 보고, 다음을 포함한 대표적인 입력 집합을 테스트하라:

* 기본 케이스(base case)
* 일반 케이스(typical case)
* 경계 케이스(edge case)

### 8.1 Tiny web server

handout 디렉터리에는 CS:APP Tiny 웹 서버(Tiny web server)의 소스 코드(source code)가 포함되어 있다.

`thttpd`만큼 강력하지는 않지만, CS:APP Tiny 웹 서버는 필요에 맞게 수정하기 쉽다. 또한 프록시 코드의 합리적인 출발점이기도 하다.

이는 driver 코드가 페이지를 가져오는 데 사용하는 서버이기도 하다.

### 8.2 telnet

교재(11.5.3)에서 설명한 것처럼, `telnet`을 사용해 프록시에 연결을 열고 HTTP 요청을 보낼 수 있다.

### 8.3 curl

`curl`을 사용하면 자신의 프록시를 포함해 어떤 서버로든 HTTP 요청을 생성할 수 있다. 이는 매우 유용한 디버깅 도구(debugging tool)이다.

예를 들어 프록시와 Tiny가 모두 로컬 머신(local machine)에서 실행 중이고, Tiny는 `15213` 포트에서 listen 중이며, 프록시는 `15214` 포트에서 listen 중이라면, 다음 명령으로 프록시를 통해 Tiny의 페이지를 요청할 수 있다:

```sh
linux> curl -v --proxy http://localhost:15214 http://localhost:15213/home.html
```

예시 상호작용:

```text
* About to connect() to proxy localhost port 15214 (#0)
* Trying 127.0.0.1... connected
* Connected to localhost (127.0.0.1) port 15214 (#0)
> GET http://localhost:15213/home.html HTTP/1.1
> User-Agent: curl/7.19.7 (x86_64-redhat-linux-gnu)...
> Host: localhost:15213
> Accept: */*
> Proxy-Connection: Keep-Alive
>
* HTTP 1.0, assume close after body
< HTTP/1.0 200 OK
< Server: Tiny Web Server
< Content-length: 120
< Content-type: text/html
<
<html>
<head><title>test</title></head>
<body>
<img align="middle" src="godzilla.gif">
Dave O’Hallaron
</body>
</html>
* Closing connection #0
```

### 8.4 netcat

`nc`라고도 알려진 `netcat`은 다목적 네트워크 유틸리티(network utility)이다.

`telnet`처럼 `netcat`을 사용해 서버에 연결을 열 수 있다.

예:

```sh
sh> nc catshark.ics.cs.cmu.edu 12345
GET http://www.cmu.edu/hub/index.html HTTP/1.0
HTTP/1.1 200 OK
...
```

`netcat`은 웹 서버에 연결할 수 있을 뿐 아니라, 그 자체가 서버로 동작할 수도 있다.

예:

```sh
sh> nc -l 12345
```

`netcat` 서버를 설정한 뒤에는, 프록시를 통해 그 서버의 가짜 객체(phony object)에 요청을 생성할 수 있으며, 프록시가 `netcat`으로 보낸 정확한 요청을 검사(inspect)할 수 있다.

### 8.5 웹 브라우저(Web browsers)

결국에는 Mozilla Firefox의 최신 버전(most recent version)을 사용해 프록시를 테스트해야 한다.

**About Firefox**를 방문하면 브라우저가 최신 버전으로 자동 업데이트된다.

프록시와 함께 동작하도록 Firefox를 설정하려면 다음으로 이동한다:

```text
Preferences > Advanced > Network > Settings
```

실제 웹 브라우저를 통해 프록시가 동작하는 것을 보면 매우 흥미로울 것이다. 프록시의 기능은 제한적이지만, 프록시를 통해 대부분의 웹사이트를 탐색할 수 있음을 알게 될 것이다.

중요한 주의사항:

* 현대 웹 브라우저에는 자체 캐시가 있다
* 프록시의 캐시를 테스트하려면 먼저 브라우저 캐시(browser cache)를 비활성화해야 한다

---

## 9. Handin 지침

제공된 `Makefile`에는 최종 handin 파일을 빌드하는 기능이 포함되어 있다.

실행:

```sh
linux> make handin
```

출력은 다음 파일이다:

```text
../proxylab-handin.tar
```

이 파일을 제출하면 된다.

> **SITE-SPECIFIC:** 각 학생이 `proxylab-handin.tar` 솔루션 파일을 어떻게 제출해야 하는지 알려 주는 문단을 여기에 삽입한다.

추가 참고 자료:

* 교재 10-12장은 다음에 대한 유용한 정보를 담고 있다:

    * 시스템 수준 I/O(system-level I/O)
    * 네트워크 프로그래밍(network programming)
    * HTTP 프로토콜(HTTP protocols)
    * 동시 프로그래밍(concurrent programming)
* RFC 1945(`http://www.ietf.org/rfc/rfc1945.txt`)는 HTTP/1.0 프로토콜의 완전한 명세이다.

---

## 10. 힌트(Hints)

* 교재 10.11절에서 논의한 것처럼, 소켓 입력과 출력에 표준 I/O 함수(standard I/O function)를 사용하는 것은 문제가 된다. 대신 `csapp.c`에서 제공하는 Robust I/O (RIO) 패키지를 사용하는 것이 권장된다.
* `csapp.c`에서 제공되는 오류 처리 함수(error-handling function)는 프록시에 적합하지 않다. 서버가 연결을 받기 시작한 뒤에는 종료되어서는 안 되기 때문이다. 이 함수들을 수정하거나 직접 작성해야 한다.
* handout 디렉터리의 파일은 원하는 방식으로 자유롭게 수정할 수 있다. 예를 들어 좋은 모듈성(modularity)을 위해 캐시 함수를 `cache.c`와 `cache.h`라는 파일의 라이브러리(library)로 구현할 수도 있다. 새 파일을 추가하면 제공된 `Makefile`도 업데이트해야 한다.
* CS:APP3e 교재 964쪽 Aside에서 논의한 것처럼, 프록시는 `SIGPIPE` 시그널(signal)을 무시해야 하며, `EPIPE` 오류를 반환하는 write 작업(write operation)을 정상적으로 처리해야 한다.
* 때로는 조기에 닫힌 소켓에서 바이트를 받기 위해 `read`를 호출하면, `read`가 `-1`을 반환하고 `errno`가 `ECONNRESET`으로 설정된다. 프록시는 이 오류 때문에 종료되어서도 안 된다.
* 웹의 모든 콘텐츠가 ASCII 텍스트인 것은 아니라는 점을 기억하라. 웹 콘텐츠의 상당 부분은 이미지와 비디오 같은 바이너리 데이터(binary data)이다. 네트워크 I/O에 사용할 함수를 선택하고 사용할 때 바이너리 데이터를 고려해야 한다.
* 원래 요청이 HTTP/1.1이더라도 모든 요청은 HTTP/1.0으로 전달하라.

**Good luck!**
