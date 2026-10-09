#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>   /* MD5 için OpenSSL EVP arayüzü */

/* ------------------------------------------------------------------ */
/*  1) RAW STORAGE — Düz metin birebir kopya                           */
/* ------------------------------------------------------------------ */
/*
 * Dosyayı bayt bayt (binary) okuyup kopyalar. Büyük dosyalarda
 * verimli olması için sabit boyutlu bir tampon (buffer) kullanılır.
 */
int storage_save_raw(const char *in_path, const char *out_path)
{
    FILE *in = fopen(in_path, "rb");
    if (!in) {
        fprintf(stderr, "[HATA] Okunamadi: %s\n", in_path);
        return -1;
    }
    FILE *out = fopen(out_path, "wb");
    if (!out) {
        fprintf(stderr, "[HATA] Yazilamadi: %s\n", out_path);
        fclose(in);
        return -1;
    }

    char   buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fprintf(stderr, "[HATA] Yazma hatasi: %s\n", out_path);
            fclose(in);
            fclose(out);
            return -1;
        }
    }

    fclose(in);
    fclose(out);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  2) HASHED STORAGE — MD5 özetleri                                   */
/* ------------------------------------------------------------------ */

/* Satır sonundaki '\r' / '\n' karakterlerini temizler. */
static void strip_newline(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}

/*
 * Verilen veriden MD5 hesaplayıp 32 karakterlik onaltılık (hex)
 * metni out (en az 33 bayt) içine yazar.
 * OpenSSL 1.1 ve 3.x ile uyumlu modern EVP arayüzü kullanılır.
 * Dönüş: başarılıysa 0, hata durumunda -1.
 */
static int md5_hex(const unsigned char *data, size_t len, char *out)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  dlen = 0;

    if (EVP_Digest(data, len, digest, &dlen, EVP_md5(), NULL) != 1)
        return -1;

    static const char hx[] = "0123456789abcdef";
    for (unsigned int i = 0; i < dlen; ++i) {
        out[i * 2]     = hx[digest[i] >> 4];
        out[i * 2 + 1] = hx[digest[i] & 0x0F];
    }
    out[dlen * 2] = '\0';
    return 0;
}

int storage_save_hashed(const char *in_path, const char *out_path)
{
    FILE *in = fopen(in_path, "rb");
    if (!in) {
        fprintf(stderr, "[HATA] Okunamadi: %s\n", in_path);
        return -1;
    }
    FILE *out = fopen(out_path, "wb");
    if (!out) {
        fprintf(stderr, "[HATA] Yazilamadi: %s\n", out_path);
        fclose(in);
        return -1;
    }

    char   line[1024];   /* parolalar kısa olduğu için yeterli */
    char   hex[33];      /* 32 hex karakter + '\0' */
    size_t count = 0;

    while (fgets(line, sizeof(line), in)) {
        strip_newline(line);
        if (line[0] == '\0')
            continue;    /* güvenlik için boş satırı atla */

        if (md5_hex((const unsigned char *)line, strlen(line), hex) != 0) {
            fprintf(stderr, "[HATA] MD5 hesaplanamadi.\n");
            fclose(in);
            fclose(out);
            return -1;
        }
        fprintf(out, "%s\n", hex);
        count++;
    }

    printf("    (%zu parola hash'lendi)\n", count);
    fclose(in);
    fclose(out);
    return 0;
}
