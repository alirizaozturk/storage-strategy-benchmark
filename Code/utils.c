/* clock_gettime, getrusage gibi POSIX/Linux fonksiyonları için. */
#define _POSIX_C_SOURCE 200809L

#include "utils.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>

/* ------------------------------------------------------------------ */
/*  ZAMAN ÖLÇÜMÜ                                                        */
/* ------------------------------------------------------------------ */
/*
 * CLOCK_MONOTONIC sistem saatinden etkilenmeyen, geriye gitmeyen bir
 * sayaçtır; süre ölçümü için en doğru kaynaktır. Nanosaniye çözünürlüğü
 * mikrosaniyeye çevrilerek döndürülür.
 */
double util_now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1.0e6 + (double)ts.tv_nsec / 1.0e3;
}

/* ------------------------------------------------------------------ */
/*  CPU ZAMANI                                                          */
/* ------------------------------------------------------------------ */
/*
 * getrusage; sürecin kullanıcı (ru_utime) ve çekirdek (ru_stime)
 * modunda harcadığı işlemci zamanını verir. Toplamı, gerçek geçen
 * süreye bölündüğünde CPU kullanım yüzdesi elde edilir:
 *     CPU% = (CPU_zamani / gecen_sure) * 100
 */
double util_cpu_us(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    double user = (double)ru.ru_utime.tv_sec * 1.0e6 + (double)ru.ru_utime.tv_usec;
    double sys  = (double)ru.ru_stime.tv_sec * 1.0e6 + (double)ru.ru_stime.tv_usec;
    return user + sys;
}

/* ------------------------------------------------------------------ */
/*  BELLEK ÖLÇÜMÜ                                                       */
/* ------------------------------------------------------------------ */

/* Tepe bellek: getrusage.ru_maxrss Linux'ta kilobayt cinsindedir. */
long util_peak_mem_kb(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_maxrss;
}

/*
 * Anlık RSS: /proc/self/statm dosyasının 2. alanı, yerleşik bellekteki
 * sayfa (page) sayısıdır. Sayfa boyutuyla çarpılıp KB'ye çevrilir.
 */
long util_current_mem_kb(void)
{
    FILE *f = fopen("/proc/self/statm", "r");
    if (!f)
        return -1;

    long pages = 0;
    /* 1. alan toplam program boyutu, 2. alan RSS (sayfa cinsinden). */
    if (fscanf(f, "%*s %ld", &pages) != 1) {
        fclose(f);
        return -1;
    }
    fclose(f);

    long page_kb = sysconf(_SC_PAGESIZE) / 1024;   /* genelde 4 KB */
    return pages * page_kb;
}
