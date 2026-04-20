#include <stdio.h>

/* 권장 최대 캐시 크기와 객체 크기 */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* 이 긴 줄을 코드에 포함해도 style 점수는 깎이지 않습니다. */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/**
 * @brief 프록시 과제에서 권장하는 User-Agent 헤더를 표준 출력에 출력합니다.
 *
 * @return 정상 종료 시 0을 반환합니다.
 */
int main()
{
  printf("%s", user_agent_hdr);
  return 0;
}
