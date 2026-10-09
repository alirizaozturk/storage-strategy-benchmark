/* strdup() ve diğer POSIX fonksiyonları için (C11'de gereklidir). */
#define _POSIX_C_SOURCE 200809L

#include "search.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>          /* compressed arama için */
#include <openssl/evp.h>   /* hashed arama için MD5 */

#define LINE_MAX_LEN 1024

/* ------------------------------------------------------------------ */
/*  YARDIMCILAR                                                         */
/* ------------------------------------------------------------------ */

/* Satır sonundaki '\r' / '\n' karakterlerini temizler. */
static void strip_newline(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}

/* Sorgunun MD5'ini 32 karakterlik onaltılık metne çevirir (out >= 33). */
static int md5_hex(const char *data, char *out)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  dlen = 0;

    if (EVP_Digest(data, strlen(data), digest, &dlen, EVP_md5(), NULL) != 1)
        return -1;

    static const char hx[] = "0123456789abcdef";
    for (unsigned int i = 0; i < dlen; ++i) {
        out[i * 2]     = hx[digest[i] >> 4];
        out[i * 2 + 1] = hx[digest[i] & 0x0F];
    }
    out[dlen * 2] = '\0';
    return 0;
}

/* ------------------------------------------------------------------ */
/*  ARAMA SETİNİN OLUŞTURULMASI (reservoir sampling)                   */
/* ------------------------------------------------------------------ */
/*
 * Tek geçişte, dosyanın tamamını belleğe almadan k adet rastgele satır
 * seçer. İlk k satır doğrudan alınır; sonraki her i. satır k/(i+1)
 * olasılıkla seçilenlerden birinin yerine konur. Sonuç: her satırın
 * seçilme olasılığı eşittir.
 */
char **search_pick_random(const char *path, int k,
                          size_t *out_count, size_t *out_total)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[HATA] Acilamadi: %s\n", path);
        return NULL;
    }

    char **res = calloc((size_t)k, sizeof(char *));
    if (!res) {
        fclose(f);
        return NULL;
    }

    char   line[LINE_MAX_LEN];
    size_t seen = 0;   /* görülen boş olmayan satır sayısı */

    while (fgets(line, sizeof(line), f)) {
        strip_newline(line);
        if (line[0] == '\0')
            continue;

        if (seen < (size_t)k) {
            res[seen] = strdup(line);
        } else {
            size_t j = (size_t)rand() % (seen + 1);
            if (j < (size_t)k) {
                free(res[j]);
                res[j] = strdup(line);
            }
        }
        seen++;
    }
    fclose(f);

    if (out_count)
        *out_count = (seen < (size_t)k) ? seen : (size_t)k;
    if (out_total)
        *out_total = seen;
    return res;
}

void search_free_set(char **set, size_t count)
{
    if (!set)
        return;
    for (size_t i = 0; i < count; ++i)
        free(set[i]);
    free(set);
}

/* ------------------------------------------------------------------ */
/*  1) RAW ARAMA — düz metinde doğrusal arama                          */
/* ------------------------------------------------------------------ */
int search_raw(const char *path, const char *query)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;

    char line[LINE_MAX_LEN];
    int  found = 0;
    while (fgets(line, sizeof(line), f)) {
        strip_newline(line);
        if (strcmp(line, query) == 0) {
            found = 1;
            break;             /* bulununca dur (erken çıkış) */
        }
    }
    fclose(f);
    return found;
}

/* ------------------------------------------------------------------ */
/*  2) HASHED ARAMA — sorgunun MD5'i hash dosyasında aranır            */
/* ------------------------------------------------------------------ */
int search_hashed(const char *path, const char *query)
{
    char want[33];
    if (md5_hex(query, want) != 0)
        return -1;

    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;

    char line[LINE_MAX_LEN];
    int  found = 0;
    while (fgets(line, sizeof(line), f)) {
        strip_newline(line);
        if (strcmp(line, want) == 0) {
            found = 1;
            break;
        }
    }
    fclose(f);
    return found;
}

/* ------------------------------------------------------------------ */
/*  3) COMPRESSED ARAMA — .gz akış halinde açılıp aranır               */
/* ------------------------------------------------------------------ */
/*
 * gzgets, veriyi okurken aynı anda açar (decompress). Yani arama süresi
 * hem açma hem karşılaştırma maliyetini içerir.
 */
int search_compressed(const char *path, const char *query)
{
    gzFile f = gzopen(path, "rb");
    if (!f)
        return -1;

    char line[LINE_MAX_LEN];
    int  found = 0;
    while (gzgets(f, line, sizeof(line)) != Z_NULL) {
        strip_newline(line);
        if (strcmp(line, query) == 0) {
            found = 1;
            break;
        }
    }
    gzclose(f);
    return found;
}

/* ------------------------------------------------------------------ */
/*  DECOMPRESSION SÜRESİ ÖLÇÜMÜ — sadece açma (arama yok)              */
/* ------------------------------------------------------------------ */
long search_decompress_all(const char *path)
{
    gzFile f = gzopen(path, "rb");
    if (!f)
        return -1;

    unsigned char buf[65536];
    long  total = 0;
    int   n;
    while ((n = gzread(f, buf, sizeof(buf))) > 0)
        total += n;

    gzclose(f);
    return (n < 0) ? -1 : total;
}
