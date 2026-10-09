#ifndef UTILS_H
#define UTILS_H

/*
 * utils modülü — Performans ölçüm yardımcıları (Linux)
 * ----------------------------------------------------
 * Zaman (wall clock), CPU zamanı ve bellek (RSS) ölçümleri sağlar.
 * Linux'a özgü API'ler kullanır: clock_gettime, getrusage, /proc.
 */

/* Monotonik duvar saati (wall clock), MİKROSANİYE cinsinden döner.
   İki çağrı arasındaki fark geçen gerçek süreyi verir. */
double util_now_us(void);

/* Sürecin başından beri kullandığı toplam CPU zamanı
   (kullanıcı + sistem), mikrosaniye cinsinden (getrusage). */
double util_cpu_us(void);

/* Sürecin tepe (peak) bellek kullanımı, kilobayt (getrusage ru_maxrss). */
long util_peak_mem_kb(void);

/* Sürecin o anki yerleşik bellek (RSS) kullanımı, kilobayt
   (/proc/self/statm üzerinden). Hata durumunda -1. */
long util_current_mem_kb(void);

#endif /* UTILS_H */
