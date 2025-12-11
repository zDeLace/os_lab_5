#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

/* child_main:
 *  pc_read_fd  - дескриптор для чтения (parent -> child)
 *  cp_write_fd - дескриптор для записи (child -> parent)
 *  result_filename - имя файла для записи результатов
 *  Возвращает 0 при нормальном завершении, !=0 при ошибке (например, div by zero).
 */
int child_main(int pc_read_fd, int cp_write_fd, const char *result_filename);

/* Вспомогательные низкоуровневые функции (реализованы в utils.c) */
void err_exit(const char *msg); /* печатает msg и errno код в STDERR и _exit(EXIT_FAILURE) */

ssize_t write_all(int fd, const void *buf, size_t count); /* надёжная запись всех байт */
ssize_t read_line(int fd, char **out_buf); /* читает одну строку до '\\n' (без '\\n'), выделяет буфер через malloc; возвращает длину или 0 (EOF) или -1 (ошибка) */
void trim_trailing_ws(char *s);

/* Форматирование чисел (без использования stdio) */
size_t i64_to_str(int64_t v, char *buf, size_t bufsz); /* возврат длины записанных байт (без \0) */
size_t u64_to_str(uint64_t v, char *buf, size_t bufsz);
size_t double_to_str(double d, char *buf, size_t bufsz); /* записывает представление double с макс 6 знаков после точки */

#endif /* COMMON_H */