/* getline() ve diğer POSIX fonksiyonları için gereklidir. */
#define _POSIX_C_SOURCE 200809L

#include "preprocess.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

/* ------------------------------------------------------------------ */
/*  1) HASH SET (Tekrarları yakalamak için)                            */
/* ------------------------------------------------------------------ */
/*
 * Parolaları satır satır birbiriyle karşılaştırmak O(n^2) olur ve
 * milyonlarca satırda kullanılamaz. Bunun yerine her parolayı bir
 * hash set'e ekleriz: ekleme/arama ortalama O(1)'dir.
 * Çakışmalar zincirleme (separate chaining) ile çözülür ve doluluk
 * oranı arttıkça tablo otomatik büyütülür.
 */

typedef struct Node {
    char        *key;   /* parola metni (kopya) */
    struct Node *next;  /* aynı kovadaki bir sonraki düğüm */
} Node;

typedef struct {
    Node   **buckets;   /* kova dizisi */
    size_t   capacity;  /* kova sayısı */
    size_t   size;      /* tablodaki eleman sayısı */
} HashSet;

/* djb2 — basit ve hızlı bir string hash fonksiyonu. */
static unsigned long hash_str(const char *s)
{
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*s++))
        h = ((h << 5) + h) + c; /* h * 33 + c */
    return h;
}

static int hs_init(HashSet *set, size_t capacity)
{
    set->buckets = calloc(capacity, sizeof(Node *));
    if (!set->buckets)
        return -1;
    set->capacity = capacity;
    set->size = 0;
    return 0;
}

/* Doluluk oranı yükseldiğinde kova sayısını iki katına çıkarır. */
static int hs_resize(HashSet *set)
{
    size_t new_cap = set->capacity * 2;
    Node **new_buckets = calloc(new_cap, sizeof(Node *));
    if (!new_buckets)
        return -1;

    for (size_t i = 0; i < set->capacity; ++i) {
        Node *n = set->buckets[i];
        while (n) {
            Node *next = n->next;
            size_t idx = hash_str(n->key) % new_cap;
            n->next = new_buckets[idx];
            new_buckets[idx] = n;
            n = next;
        }
    }
    free(set->buckets);
    set->buckets = new_buckets;
    set->capacity = new_cap;
    return 0;
}

/*
 * Parola tabloda yoksa ekler ve 1 döner (yeni / benzersiz).
 * Zaten varsa 0 döner (tekrar / duplicate).
 * Bellek hatasında -1 döner.
 */
static int hs_add(HashSet *set, const char *key)
{
    size_t idx = hash_str(key) % set->capacity;
    for (Node *n = set->buckets[idx]; n; n = n->next)
        if (strcmp(n->key, key) == 0)
            return 0; /* tekrar bulundu */

    Node *node = malloc(sizeof(Node));
    if (!node)
        return -1;
    node->key = strdup(key);
    if (!node->key) {
        free(node);
        return -1;
    }
    node->next = set->buckets[idx];
    set->buckets[idx] = node;
    set->size++;

    /* Doluluk oranı %75'i geçerse büyüt. */
    if (set->size > set->capacity * 3 / 4)
        hs_resize(set);

    return 1; /* yeni eklendi */
}

static void hs_free(HashSet *set)
{
    for (size_t i = 0; i < set->capacity; ++i) {
        Node *n = set->buckets[i];
        while (n) {
            Node *next = n->next;
            free(n->key);
            free(n);
            n = next;
        }
    }
    free(set->buckets);
    set->buckets = NULL;
    set->capacity = set->size = 0;
}

/* ------------------------------------------------------------------ */
/*  2) YARDIMCI FONKSİYONLAR                                           */
/* ------------------------------------------------------------------ */

/* Satır sonundaki '\r' ve '\n' karakterlerini temizler (CRLF/LF uyumu). */
static void strip_newline(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}

/* Dosya adının verilen uzantı ile bitip bitmediğini kontrol eder. */
static int has_suffix(const char *name, const char *suffix)
{
    size_t ln = strlen(name), ls = strlen(suffix);
    return ln >= ls && strcmp(name + ln - ls, suffix) == 0;
}

/* Bir satırı tutan, ihtiyaç oldukça büyüyen tampon. */
typedef struct {
    char  *data;
    size_t cap;
} LineBuf;

/*
 * Taşınabilir satır okuyucu (getline yerine — Linux/Windows uyumu için).
 * Bir satırı (satır sonu dahil) buf içine okur ve uzunluğunu döner.
 * Dosya sonu ve okunacak veri yoksa -1, bellek hatasında -2 döner.
 */
static long read_line(FILE *in, LineBuf *buf)
{
    size_t len = 0;
    int    c   = EOF;

    if (buf->cap == 0) {
        buf->cap  = 128;
        buf->data = malloc(buf->cap);
        if (!buf->data)
            return -2;
    }

    while ((c = fgetc(in)) != EOF) {
        if (len + 1 >= buf->cap) {              /* tamponu büyüt */
            size_t ncap = buf->cap * 2;
            char  *nd   = realloc(buf->data, ncap);
            if (!nd)
                return -2;
            buf->data = nd;
            buf->cap  = ncap;
        }
        buf->data[len++] = (char)c;
        if (c == '\n')
            break;
    }

    if (len == 0 && c == EOF)
        return -1;          /* dosya sonu */

    buf->data[len] = '\0';
    return (long)len;
}

/* ------------------------------------------------------------------ */
/*  3) TEK BİR DOSYAYI İŞLE                                            */
/* ------------------------------------------------------------------ */
/*
 * Bir ham dosyayı satır satır okur. Her satırı temizler, boşsa atlar,
 * tekrar değilse out dosyasına yazıp hash set'e ekler.
 * .csv dosyalarında yalnızca ilk sütun (virgülden önceki kısım) alınır;
 * çünkü PIN listesi "parola,frekans" biçimindedir.
 */
static int process_file(const char *path, int is_csv,
                        HashSet *set, FILE *out, PreprocessStats *stats)
{
    FILE *in = fopen(path, "rb");
    if (!in) {
        fprintf(stderr, "  [UYARI] Acilamadi: %s\n", path);
        return -1;
    }

    LineBuf buf = {0};
    long    n;

    while ((n = read_line(in, &buf)) >= 0) {
        char *line = buf.data;
        stats->total_lines++;

        /* .csv ise virgülden sonrasını (frekans) at. */
        if (is_csv) {
            char *comma = strchr(line, ',');
            if (comma)
                *comma = '\0';
        }

        strip_newline(line);

        if (line[0] == '\0') {        /* boş satır */
            stats->empty_skipped++;
            continue;
        }

        int r = hs_add(set, line);
        if (r < 0) {
            fprintf(stderr, "  [HATA] Bellek yetersiz.\n");
            free(buf.data);
            fclose(in);
            return -1;
        }
        if (r == 0) {                 /* tekrar */
            stats->duplicates_skipped++;
            continue;
        }

        fprintf(out, "%s\n", line);   /* benzersiz parola */
        stats->unique_written++;
    }

    free(buf.data);
    fclose(in);
    return (n == -2) ? -1 : 0;
}

/* ------------------------------------------------------------------ */
/*  4) ANA İŞLEM: TÜM KLASÖRÜ İŞLE                                     */
/* ------------------------------------------------------------------ */

int preprocess_run(const char *raw_dir, const char *out_path,
                   PreprocessStats *stats)
{
    static PreprocessStats local;
    if (!stats)
        stats = &local;

    /* Sayaçları her durumda sıfırla (çağıranın verdiği yapı dahil). */
    stats->files_processed = stats->total_lines = stats->empty_skipped = 0;
    stats->duplicates_skipped = stats->unique_written = 0;

    HashSet set;
    if (hs_init(&set, 1024) != 0) {
        fprintf(stderr, "[HATA] Hash set olusturulamadi.\n");
        return -1;
    }

    FILE *out = fopen(out_path, "wb");
    if (!out) {
        fprintf(stderr, "[HATA] Cikti dosyasi acilamadi: %s\n", out_path);
        hs_free(&set);
        return -1;
    }

    DIR *dir = opendir(raw_dir);
    if (!dir) {
        fprintf(stderr, "[HATA] RawData klasoru acilamadi: %s\n", raw_dir);
        fclose(out);
        hs_free(&set);
        return -1;
    }

    struct dirent *entry;
    char path[4096];

    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;

        /* Yalnızca .txt ve .csv dosyalarını işle. */
        int is_csv = has_suffix(name, ".csv");
        if (!has_suffix(name, ".txt") && !is_csv)
            continue;

        snprintf(path, sizeof(path), "%s/%s", raw_dir, name);
        printf("  -> Isleniyor: %s\n", name);

        if (process_file(path, is_csv, &set, out, stats) == 0)
            stats->files_processed++;
    }

    closedir(dir);
    fclose(out);
    hs_free(&set);
    return 0;
}
