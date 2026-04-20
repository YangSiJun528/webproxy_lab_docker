/*
 * adder.c - 두 숫자를 더하는 최소 CGI 프로그램입니다.
 */
/* $begin adder */
#include "csapp.h"

/**
 * @brief QUERY_STRING에서 두 숫자 인자를 읽어 더한 뒤 HTML 응답을 생성합니다.
 *
 * @return 정상 처리 시 0으로 종료합니다.
 */
int main(void)
{
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  /* 두 인자를 추출합니다. */
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    p = strchr(buf, '&');
    *p = '\0';
    strcpy(arg1, buf);
    strcpy(arg2, p + 1);
    n1 = atoi(strchr(arg1, '=') + 1);
    n2 = atoi(strchr(arg2, '=') + 1);
  }

  /* 응답 본문을 만듭니다. */
  sprintf(content, "QUERY_STRING=%s\r\n<p>", buf);
  sprintf(content + strlen(content), "Welcome to add.com: ");
  sprintf(content + strlen(content), "THE Internet addition portal.\r\n<p>");
  sprintf(content + strlen(content), "The answer is: %d + %d = %d\r\n<p>",
          n1, n2, n1 + n2);
  sprintf(content + strlen(content), "Thanks for visiting!\r\n");

  /* HTTP 응답을 생성합니다. */
  printf("Content-type: text/html\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}
/* $end adder */
