#include "../include/common.h"

/* child_main: логика дочернего процесса, полностью на системных вызовах.
 * Читает строки от pc_read_fd (read_line), парсит числа, вычисляет последовательное деление:
 *   result = first / second / third / ...
 * Если встречается деление на ноль (abs(d) < 1e-15), отправляет "DIV_ZERO\n" в cp_write_fd и завершает работу с ненулевым кодом.
 * При успешном расчёте добавляет одну строку "<result>\n" в файл result_filename (open/ write/ close) и отправляет "OK <result>\n" в cp_write_fd.
 * Использует strtod для парсинга double.
 */
int child_main(int pc_read_fd, int cp_write_fd, const char *result_filename) {
    if (pc_read_fd < 0 || cp_write_fd < 0 || !result_filename) return EXIT_FAILURE;

    int fd_res = open(result_filename, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd_res < 0) {
        char buf[128];
        size_t p = 0;
        const char *pref = "ERR OPEN_FILE ";
        memcpy(buf + p, pref, strlen(pref)); p += strlen(pref);
        char num[32];
        size_t nlen = i64_to_str((int64_t)errno, num, sizeof num);
        memcpy(buf + p, num, nlen); p += nlen;
        buf[p++] = '\n';
        write_all(cp_write_fd, buf, p);
        return EXIT_FAILURE;
    }

    for (;;) {
        char *line = NULL;
        ssize_t rl = read_line(pc_read_fd, &line);
        if (rl < 0) {
            char msg[] = "ERR READ_PIPE\n";
            write_all(cp_write_fd, msg, sizeof msg - 1);
            if (line) free(line);
            close(fd_res);
            return EXIT_FAILURE;
        } else if (rl == 0) {
            if (line) free(line);
            close(fd_res);
            return EXIT_SUCCESS;
        }

        trim_trailing_ws(line);
        if (line[0] == '\0') {
            write_all(cp_write_fd, "OK EMPTY\n", 9);
            free(line);
            continue;
        }

        char *saveptr = NULL;
        char *tok = strtok_r(line, " \t", &saveptr);
        if (!tok) {
            write_all(cp_write_fd, "OK EMPTY\n", 9);
            free(line);
            continue;
        }

        errno = 0;
        char *endptr = NULL;
        double result = strtod(tok, &endptr);
        if (endptr == tok || errno == ERANGE) {
            write_all(cp_write_fd, "ERR BAD_NUMBER\n", 15);
            free(line);
            continue;
        }

        int div_by_zero = 0;
        while ((tok = strtok_r(NULL, " \t", &saveptr)) != NULL) {
            errno = 0;
            double d = strtod(tok, &endptr);
            if (endptr == tok || errno == ERANGE) {
                write_all(cp_write_fd, "ERR BAD_NUMBER\n", 15);
                div_by_zero = -1;
                break;
            }
            if (fabs(d) < 1e-15) {
                write_all(cp_write_fd, "DIV_ZERO\n", 9);
                div_by_zero = 1;
                break;
            }
            result /= d;
        }

        if (div_by_zero == 1) {
            free(line);
            close(fd_res);
            return EXIT_FAILURE;
        } else if (div_by_zero == -1) {
            free(line);
            continue;
        } else {
            char numbuf[128];
            size_t nlen = double_to_str(result, numbuf, sizeof numbuf);
            char filebuf[160];
            size_t p = 0;
            if (nlen > sizeof filebuf - 2) nlen = sizeof filebuf - 2;
            memcpy(filebuf + p, numbuf, nlen); p += nlen;
            filebuf[p++] = '\n';
            if (write_all(fd_res, filebuf, p) < 0) {
                write_all(cp_write_fd, "ERR WRITE_FILE\n", 15);
                free(line);
                close(fd_res);
                return EXIT_FAILURE;
            }
            char pipebuf[192];
            size_t q = 0;
            const char *pref = "OK ";
            memcpy(pipebuf + q, pref, 3); q += 3;
            if (nlen > sizeof pipebuf - q - 2) nlen = sizeof pipebuf - q - 2;
            memcpy(pipebuf + q, numbuf, nlen); q += nlen;
            pipebuf[q++] = '\n';

            if (write_all(cp_write_fd, pipebuf, q) < 0) {
                free(line);
                close(fd_res);
                return EXIT_FAILURE;
            }
        }

        free(line);
    }

    close(fd_res);
    return EXIT_SUCCESS;
}
