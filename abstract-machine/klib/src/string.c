#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t i = 0;

  while (s[i] != '\0')
  {
    i++;
  }

  return i;
}

char *strcpy(char *dst, const char *src) {
  size_t i;

  for (i = 0; src[i] != '\0'; i++)
  {
    dst[i] = src[i];
  }

  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  panic("Not implemented");
}

char *strcat(char *dst, const char *src) {
  size_t i;

  for (i = 0; dst[i] != '\0'; i++)
  {
    continue;
  }

  for (int j = 0; src[j] != '\0'; j++)
  {
    dst[i] = src[j];
    i++;
  }

  dst[i] = '\0';

  return dst;
}

int strcmp(const char *s1, const char *s2) {

  const char *tmp1 = s1;
  const char *tmp2 = s2;

  while (*tmp1 != '\0' && *tmp2 != '\0')
  {
    if (*tmp1 < *tmp2)
      return -1;
    else if (*tmp1 > *tmp2)
      return 1;
    else
    {
      tmp1++;
      tmp2++;
    }
  }

  if (*tmp1 < *tmp2)
    return -1;
  else if (*tmp1 > *tmp2)
    return 1;

  return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  panic("Not implemented");
}

void *memset(void *s, int c, size_t n) {
  unsigned char *tmp = s;

  for (int i = 0; i < n; i++)
  {
    *tmp = c;
    tmp++;
  }

  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  panic("Not implemented");
}

void *memcpy(void *out, const void *in, size_t n) {
  panic("Not implemented");
}

int memcmp(const void *s1, const void *s2, size_t n) {

  const unsigned char *tmp1 = s1;
  const unsigned char *tmp2 = s2;

  for (int i = 0; i < n; i++)
  {
    if (*tmp1 < *tmp2)
      return -1;
    else if (*tmp1 > *tmp2)
      return 1;
    else
    {
      tmp1++;
      tmp2++;
    }
  }

  return 0;
}

#endif
