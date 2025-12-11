#include "../include/common.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        const char *msg = "Usage: lab_os result_filename\n";
        write_all(STDERR_FILENO, msg, strlen(msg));
        return EXIT_FAILURE;
    }
    const char *result_filename = argv[1];

    int pc[2];
    int cp[2];

    if (pipe(pc) == -1) err_exit("pipe pc");
    if (pipe(cp) == -1) err_exit("pipe cp");

    signal(SIGPIPE, SIG_IGN);

    pid_t pid = fork();
    if (pid < 0) err_exit("fork");

    if (pid == 0) {
        close(pc[1]);
        close(cp[0]);
        int rc = child_main(pc[0], cp[1], result_filename);
        _exit(rc);
    } else {
        close(pc[0]);
        close(cp[1]);

        write_all(STDOUT_FILENO, "Enter lines of floats separated by spaces (e.g. \"10 2 5\").\n", 62);
        write_all(STDOUT_FILENO, "To finish, send EOF (Ctrl-D).\n", 31);

        for (;;) {
            write_all(STDOUT_FILENO, ">> ", 3);
            char *line = NULL;
            ssize_t rl = read_line(STDIN_FILENO, &line);
            if (rl < 0) {
                write_all(STDERR_FILENO, "Error reading stdin\n", 20);
                if (line) free(line);
                break;
            } else if (rl == 0) {
                close(pc[1]);
                if (line) free(line);
                for (;;) {
                    char *r2 = NULL;
                    ssize_t rr = read_line(cp[0], &r2);
                    if (rr <= 0) {
                        if (r2) free(r2);
                        break;
                    }
                    write_all(STDOUT_FILENO, "[child] ", 8);
                    write_all(STDOUT_FILENO, r2, (size_t)rr);
                    write_all(STDOUT_FILENO, "\n", 1);
                    free(r2);
                }
                break;
            }

            if (line) {
                size_t L = strlen(line);
                char *tmp = malloc(L + 2);
                if (!tmp) { free(line); break; }
                memcpy(tmp, line, L);
                tmp[L] = '\n';
                tmp[L+1] = '\0';
                if (write_all(pc[1], tmp, L + 1) < 0) {
                    if (errno == EPIPE) {
                        write_all(STDERR_FILENO, "Child closed pipe. Exiting.\n", 28);
                        free(tmp); free(line);
                        break;
                    } else {
                        write_all(STDERR_FILENO, "Error writing to child pipe\n", 27);
                        free(tmp); free(line);
                        break;
                    }
                }
                free(tmp);
            }

            char *resp = NULL;
            ssize_t rr = read_line(cp[0], &resp);
            if (rr < 0) {
                write_all(STDERR_FILENO, "Error reading from child\n", 24);
                if (resp) free(resp);
                free(line);
                break;
            } else if (rr == 0) {
                write_all(STDERR_FILENO, "Child terminated (no response).\n", 32);
                if (resp) free(resp);
                free(line);
                break;
            } else {
                if (strcmp(resp, "DIV_ZERO") == 0) {
                    write_all(STDOUT_FILENO, "[child] Division by zero detected. Terminating both processes.\n", 66);
                    close(pc[1]);
                    free(resp);
                    free(line);
                    break;
                } else if (strncmp(resp, "OK", 2) == 0) {
                    const char *p = resp + 2;
                    if (*p == ' ') p++;
                    write_all(STDOUT_FILENO, "[child] ", 8);
                    write_all(STDOUT_FILENO, p, strlen(p));
                    write_all(STDOUT_FILENO, "\n", 1);
                } else if (strncmp(resp, "ERR", 3) == 0) {
                    write_all(STDOUT_FILENO, "[child] Error: ", 15);
                    write_all(STDOUT_FILENO, resp, strlen(resp));
                    write_all(STDOUT_FILENO, "\n", 1);
                } else {
                    write_all(STDOUT_FILENO, "[child] ", 8);
                    write_all(STDOUT_FILENO, resp, strlen(resp));
                    write_all(STDOUT_FILENO, "\n", 1);
                }
                free(resp);
            }

            free(line);
        }

        close(cp[0]);

        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int ec = WEXITSTATUS(status);
            if (ec == 0) {
                write_all(STDOUT_FILENO, "Child exited normally.\n", 23);
            } else {
                char buf[64];
                size_t n = i64_to_str(ec, buf, sizeof buf);
                buf[n++] = '\n';
                write_all(STDOUT_FILENO, "Child exited with code ", 23);
                write_all(STDOUT_FILENO, buf, n);
            }
        } else if (WIFSIGNALED(status)) {
            char sb[32];
            size_t ln = i64_to_str(WTERMSIG(status), sb, sizeof sb);
            sb[ln++] = '\n';
            write_all(STDOUT_FILENO, "Child killed by signal ", 23);
            write_all(STDOUT_FILENO, sb, ln);
        }

        return EXIT_SUCCESS;
    }
}
