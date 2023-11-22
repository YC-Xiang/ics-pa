#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

char *itoa(int i, char *str)
{
    int mod, div = abs(i), index = 0;
    char *start, *end, temp;

    do
    {
        mod = div % 10;
        str[index++] = '0' + mod;
        div = div / 10;
    }while(div != 0);

    if (i < 0)
        str[index++] = '-';

    str[index] = '\0';

    for (start = str, end = str + strlen(str) - 1;
        start < end; start++, end--)
    {
        temp    = *start;
        *start    = *end;
        *end    = temp;
    }

    return str;
}

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  int i, index, num;
  int len = strlen(fmt);
  char *string;
  va_list ap;
  va_start(ap, fmt);

  for (i = 0, index = 0; i < len; i++)
  {
    if (fmt[i] != '%')
    {
      out[index++] = fmt[i];
    }
    else
    {
      switch (fmt[i + 1])
      {
      case 'd':
        num = va_arg(ap, int);
        string = itoa(num, out + index);
        index += strlen(string);
        i++;
        break;
      case 's':
        string = va_arg(ap, char*);
        strcpy(out + index, string);
        index += strlen(string);
        i++;
        break;
      default:
        break;
      }
    }
  }
  out[index] = '\0';
  va_end(ap);

  return strlen(out);
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
