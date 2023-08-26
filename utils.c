#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

char *get_current_time_str() {
  time_t current_time;
  time(&current_time);

  char *formatted_date = (char *)malloc(sizeof(char) * 100);
  strftime(formatted_date, 100, "%A, %d %B %Y %H:%M:%S GMT",
           gmtime(&current_time));
  return formatted_date;
}

char *get_last_modified_date(const char *file_path) {
  struct stat file_stat;
  if (stat(file_path, &file_stat) == -1) {
    perror("Error getting file information");
    return NULL;
  }

  struct tm *time_info = gmtime(&(file_stat.st_mtime));

  char *formatted_date = (char *)malloc(sizeof(char) * 100);
  if (formatted_date == NULL) {
    perror("Memory allocation error");
    return NULL;
  }

  strftime(formatted_date, 100, "%A, %d %b %Y %H:%M:%S GMT", time_info);

  return formatted_date;
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