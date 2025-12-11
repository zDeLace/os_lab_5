#include "../include/common.h"

ssize_t write_all(int fd, const void *buf, size_t count) {
    size_t written = 0;
    const char *p = (const char *)buf;
    while (written < count) {
        ssize_t w = write(fd, p + written, count - written);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        written += (size_t)w;
    }
    return (ssize_t)written;
}

ssize_t read_line(int fd, char **out_buf) {
    if (!out_buf) return -1;
    size_t cap = 256;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) return -1;

    while (1) {
        char ch;
        ssize_t r = read(fd, &ch, 1);
        if (r < 0) {
            if (errno == EINTR) continue;
            free(buf);
            return -1;
        } else if (r == 0) {
            /* EOF */
            if (len == 0) {
                free(buf);
                return 0;
            } else {
                buf[len] = '\0';
                *out_buf = buf;
                return (ssize_t)len;
            }
        } else {
            if (ch == '\r') continue;
            if (ch == '\n') {
                buf[len] = '\0';
                *out_buf = buf;
                return (ssize_t)len;
            } else {
                if (len + 1 >= cap) {
                    size_t newcap = cap * 2;
                    char *tmp = realloc(buf, newcap);
                    if (!tmp) { free(buf); return -1; }
                    buf = tmp;
                    cap = newcap;
                }
                buf[len++] = ch;
            }
        }
    }
}

/* trim_trailing_ws: удаляет завершающие пробельные символы (пробел, таб, CR, LF) */
void trim_trailing_ws(char *s) {
    if (!s) return;
    size_t i = strlen(s);
    while (i > 0) {
        char c = s[i-1];
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
            s[i-1] = '\0';
            i--;
        } else break;
    }
}

/* конвертация знакового 64-битного целого в строку (без терминатора), возвращает длину записанных символов */
size_t i64_to_str(int64_t v, char *buf, size_t bufsz) {
    if (bufsz == 0) return 0;
    uint64_t uv;
    int neg = 0;
    if (v < 0) { neg = 1; uv = (uint64_t)(-v); } else uv = (uint64_t)v;

    char tmp[32];
    size_t pos = 0;
    if (uv == 0) tmp[pos++] = '0';
    else {
        while (uv > 0) {
            tmp[pos++] = (char)('0' + (uv % 10));
            uv /= 10;
        }
    }

    size_t needed = pos + (neg ? 1 : 0);
    if (needed > bufsz) {
        size_t out = 0;
        if (neg && out < bufsz) buf[out++] = '-';
        size_t take = bufsz - out;
        for (size_t i = 0; i < take; ++i) {
            buf[out + i] = tmp[take - 1 - i];
        }
        return bufsz;
    }

    size_t idx = 0;
    if (neg) buf[idx++] = '-';
    for (size_t i = 0; i < pos; ++i) buf[idx + i] = tmp[pos - 1 - i];
    return needed;
}

size_t u64_to_str(uint64_t v, char *buf, size_t bufsz) {
    if (bufsz == 0) return 0;
    char tmp[32];
    size_t pos = 0;
    if (v == 0) tmp[pos++] = '0';
    else {
        while (v > 0 && pos < sizeof(tmp)) {
            tmp[pos++] = (char)('0' + (v % 10));
            v /= 10;
        }
    }

    if (pos > bufsz) pos = bufsz;

    for (size_t i = 0; i < pos; ++i) {
        buf[i] = tmp[pos - 1 - i];
    }

    return pos;
}

size_t double_to_str(double d, char *buf, size_t bufsz) {
    if (bufsz == 0) return 0;
    if (!isfinite(d)) {
        if (isnan(d)) {
            const char *s = "nan";
            size_t L = strlen(s);
            if (L > bufsz) L = bufsz;
            memcpy(buf, s, L);
            return L;
        } else {
            if (signbit(d)) {
                const char *s = "-inf";
                size_t L = strlen(s);
                if (L > bufsz) L = bufsz;
                memcpy(buf, s, L);
                return L;
            } else {
                const char *s = "inf";
                size_t L = strlen(s);
                if (L > bufsz) L = bufsz;
                memcpy(buf, s, L);
                return L;
            }
        }
    }

    char tmp[128];
    size_t pos = 0;
    if (d < 0) {
        if (pos < sizeof tmp) tmp[pos++] = '-';
        d = -d;
    }
    double intpart;
    double frac = modf(d, &intpart);
    uint64_t ip = (uint64_t) intpart;
    char intbuf[64] = {0};
    size_t intlen = u64_to_str(ip, intbuf, sizeof intbuf);
    for (size_t k = 0; k < intlen && pos < sizeof tmp; ++k)
        tmp[pos++] = intbuf[k];

    const int PREC = 6;
    if (frac > 0.0) {
        if (pos < sizeof tmp) tmp[pos++] = '.';
        double scaled = frac * pow(10.0, PREC);
        uint64_t fracint = (uint64_t) llround(scaled);
        uint64_t pow10 = 1;
        for (int i = 0; i < PREC; ++i) pow10 *= 10;
        if (fracint >= pow10) {
            fracint -= pow10;
            ip = ip + 1;
            intlen = u64_to_str(ip, intbuf, sizeof intbuf);
            size_t start = 0;
            if (tmp[0] == '-') start = 1;
            pos = start;
            for (size_t k = 0; k < intlen && pos < sizeof tmp; ++k) tmp[pos++] = intbuf[k];
            if (pos < sizeof tmp) tmp[pos++] = '.';
        }
        char fracbuf[32];
        for (int i = PREC - 1; i >= 0; --i) {
            fracbuf[i] = (char)('0' + (fracint % 10));
            fracint /= 10;
        }
        for (int i = 0; i < PREC && pos < sizeof tmp; ++i) tmp[pos++] = fracbuf[i];
        while (pos > 0 && tmp[pos-1] == '0') pos--;
        if (pos > 0 && tmp[pos-1] == '.') pos--;
    }

    size_t tocopy = pos;
    if (tocopy > bufsz) tocopy = bufsz;
    memcpy(buf, tmp, tocopy);
    return tocopy;
}

void err_exit(const char *msg) {
    char out[512];
    size_t p = 0;
    if (msg && *msg) {
        size_t L = strlen(msg);
        if (L > sizeof out - 20) L = sizeof out - 20;
        memcpy(out + p, msg, L);
        p += L;
    } else {
        const char *d = "error";
        size_t L = strlen(d);
        memcpy(out + p, d, L);
        p += L;
    }
    out[p++] = ':';
    out[p++] = ' ';
    char num[32];
    size_t nlen = i64_to_str((int64_t)errno, num, sizeof num);
    if (nlen > 0) {
        if (p + nlen > sizeof out - 2) nlen = sizeof out - 2 - p;
        memcpy(out + p, num, nlen);
        p += nlen;
    }
    out[p++] = '\n';
    write_all(STDERR_FILENO, out, p);
    _exit(EXIT_FAILURE);
}
