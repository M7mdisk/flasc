#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

char *get_current_time_str() {
  time_t current_time;
  time(&current_time);

  char *formatted_date = (char *)malloc(sizeof(char) * 100);
  strftime(formatted_date, 100, "%A, %d %B %Y %H:%M:%S GMT",
           gmtime(&current_time));
  return formatted_date;
}

bool is_positive_int(char *s) {
  for (int i = 0; s[i] != '\0'; i++) {
    if (!isdigit(s[i])) return false;
  }
  return true;
}

// Deal with memorry allocation stuff
void append_to_str(char **str, const char *append) {
  size_t currentLen = *str ? strlen(*str) : 0;
  size_t appendLen = strlen(append);

  char *newStr = (char *)realloc(
      *str, currentLen + appendLen + 1);  // +1 for null terminator

  if (newStr) {
    strcpy(newStr + currentLen, append);
    *str = newStr;
  }
}