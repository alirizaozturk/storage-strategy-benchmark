#include "search.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

/*
 * Program akışı (Aşama 3 — Arama ve Performans Analizi)
 * ----------------------------------------------------
 * 1) Processed/passwords.txt'den rastgele N (varsayılan 100) parola seçilir.
 * 2) Bu parolalar üç farklı yapıda aranır: Raw, Hashed, Compressed.
 * 3) Her yöntem için süre / CPU / bellek ölçülür (utils modülü).
 * 4) Sonuçlar Results/performance_results.txt dosyasına tablo halinde yazılır.
 *
 * Kullanım (Code/ klasöründen):  ./search [arama_sayisi]
 */

/* Tek bir yöntemin ölçüm sonuçlarını tutar. */
typedef struct {
    const char *name;
    double avg_us;        /* ortalama arama süresi */
    double max_us;        /* en uzun (worst-case) arama süresi */
    double total_wall_us; /* tüm aramaların toplam gerçek süresi */
    double cpu_percent;   /* CPU kullanım yüzdesi */
    long   mem_kb;        /* yöntem sonundaki anlık RSS */
    int    found;         /* bulunan parola sayısı */
} MethodResult;

/* Üç arama fonksiyonu da bu imzaya sahip. */
typedef int (*SearchFn)(const char *path, const char *query);

/* ---- Rapor çıktısını hem ekrana hem dosyaya yazan yardımcı ---- */
static FILE *g_report = NULL;
static void rep(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    if (g_report) {
        va_start(ap, fmt);
        vfprintf(g_report, fmt, ap);
        va_end(ap);
    }
}

static long file_size(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return sz;
}

/*
 * Bir yöntemi N sorgu için çalıştırır ve ölçer:
 *  - her sorgu tek tek zamanlanır -> ortalama ve maksimum süre
 *  - tüm döngü için CPU zamanı / gerçek süre -> CPU%
 *  - döngü sonunda anlık RSS -> bellek
 */
static MethodResult run_method(const char *name, SearchFn fn,
                               const char *path,
                               char **queries, size_t n)
{
    MethodResult r = {0};
    r.name = name;

    double sum = 0.0, maxt = 0.0;
    int    found = 0;

    double cpu0  = util_cpu_us();
    double wall0 = util_now_us();

    for (size_t i = 0; i < n; ++i) {
        double t0 = util_now_us();
        int    hit = fn(path, queries[i]);
        double dt = util_now_us() - t0;

        sum += dt;
        if (dt > maxt)
            maxt = dt;
        if (hit == 1)
            found++;
    }

    double wall1 = util_now_us();
    double cpu1  = util_cpu_us();

    r.total_wall_us = wall1 - wall0;
    r.avg_us        = sum / (double)n;
    r.max_us        = maxt;
    r.cpu_percent   = (r.total_wall_us > 0.0)
                      ? (cpu1 - cpu0) / r.total_wall_us * 100.0 : 0.0;
    r.mem_kb        = util_current_mem_kb();
    r.found         = found;
    return r;
}

int main(int argc, char *argv[])
{
    int sample_n = (argc > 1) ? atoi(argv[1]) : 100;
    if (sample_n <= 0)
        sample_n = 100;

    const char *processed = "../Processed/passwords.txt";
    const char *raw_path  = "../Storage/Raw/passwords.txt";
    const char *hash_path = "../Storage/Hashed/passwords.md5.txt";
    const char *gz_path   = "../Storage/Compressed/passwords.txt.gz";
    const char *report    = "../Results/performance_results.txt";

    /* 1) Arama setini oluştur (rastgele N parola). */
    unsigned seed = (unsigned)time(NULL);
    srand(seed);

    size_t count = 0, total = 0;
    char **set = search_pick_random(processed, sample_n, &count, &total);
    if (!set || count == 0) {
        fprintf(stderr, "[HATA] Arama seti olusturulamadi.\n");
        return 1;
    }
    printf("Arama seti hazir: %zu parola (toplam %zu kayittan, seed=%u)\n\n",
           count, total, seed);

    /* 2) Üç yöntemi çalıştır ve ölç. */
    MethodResult res[3];
    printf("Raw aramasi calisiyor...\n");
    res[0] = run_method("Raw",        search_raw,        raw_path,  set, count);
    printf("Hashed aramasi calisiyor...\n");
    res[1] = run_method("Hashed",     search_hashed,     hash_path, set, count);
    printf("Compressed aramasi calisiyor...\n");
    res[2] = run_method("Compressed", search_compressed, gz_path,   set, count);

    /* 3) Decompression süresi (3 tam açma denemesinin ortalaması). */
    printf("Decompression suresi olculuyor...\n");
    double dsum = 0.0;
    long   dbytes = 0;
    const int DRUNS = 3;
    for (int i = 0; i < DRUNS; ++i) {
        double t0 = util_now_us();
        dbytes = search_decompress_all(gz_path);
        dsum += util_now_us() - t0;
    }
    double decompress_us = dsum / DRUNS;

    /* 4) Disk kullanımı ve sıkıştırma oranı. */
    long sz_raw  = file_size(raw_path);
    long sz_hash = file_size(hash_path);
    long sz_gz   = file_size(gz_path);
    long sz_orig = file_size(processed);
    double ratio = (sz_orig > 0 && sz_gz >= 0)
                   ? 100.0 * (1.0 - (double)sz_gz / (double)sz_orig) : 0.0;
    double factor = (sz_gz > 0) ? (double)sz_orig / (double)sz_gz : 0.0;

    long peak_kb = util_peak_mem_kb();

    /* 5) Raporu yaz (hem ekrana hem dosyaya). */
    g_report = fopen(report, "w");
    if (!g_report)
        fprintf(stderr, "[UYARI] Rapor dosyasi acilamadi: %s\n", report);

    rep("\n");
    rep("==================================================================\n");
    rep("   DOSYA ORGANIZASYONU - ASAMA 3: ARAMA & PERFORMANS ANALIZI\n");
    rep("==================================================================\n");
    rep("Arama seti        : %zu rastgele parola\n", count);
    rep("Toplam kayit      : %zu parola (Processed/passwords.txt)\n", total);
    rep("Rastgelelik seed  : %u\n", seed);
    rep("\n");

    rep("------------------------- DISK KULLANIMI -------------------------\n");
    rep("%-14s %-36s %12s\n", "Yontem", "Dosya", "Boyut (MB)");
    rep("%-14s %-36s %12.2f\n", "Raw",        raw_path,  sz_raw  / 1048576.0);
    rep("%-14s %-36s %12.2f\n", "Hashed",     hash_path, sz_hash / 1048576.0);
    rep("%-14s %-36s %12.2f\n", "Compressed", gz_path,   sz_gz   / 1048576.0);
    rep("%-14s %-36s %12.2f\n", "Orijinal",   processed, sz_orig / 1048576.0);
    rep("Sikistirma orani  : %%%.1f kazanc  (%.2fx daha kucuk)\n", ratio, factor);
    rep("\n");

    rep("---------------------- ARAMA PERFORMANSI -------------------------\n");
    rep("(Sureler milisaniye cinsindendir; her yontem %zu arama)\n\n", count);
    rep("%-12s %12s %12s %8s %11s %9s\n",
        "Yontem", "Ort.(ms)", "Maks.(ms)", "CPU%", "Bellek(MB)", "Bulunan");
    for (int i = 0; i < 3; ++i) {
        rep("%-12s %12.4f %12.4f %8.1f %11.2f %6d/%zu\n",
            res[i].name,
            res[i].avg_us / 1000.0,
            res[i].max_us / 1000.0,
            res[i].cpu_percent,
            res[i].mem_kb / 1024.0,
            res[i].found, count);
    }
    rep("\n");
    rep("Decompression (tam acma) suresi : %.4f ms  (%ld bayt acildi)\n",
        decompress_us / 1000.0, dbytes);
    rep("Surec tepe bellek (peak RSS)    : %.2f MB\n", peak_kb / 1024.0);
    rep("==================================================================\n");

    if (g_report) {
        fclose(g_report);
        printf("\nRapor kaydedildi -> %s\n", report);
    }

    search_free_set(set, count);
    return 0;
}
