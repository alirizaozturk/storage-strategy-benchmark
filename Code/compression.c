#include "compression.h"

#include <stdio.h>
#include <zlib.h>   /* zlib: gzopen / gzwrite / gzclose */

/* ------------------------------------------------------------------ */
/*  COMPRESSED STORAGE — zlib (gzip) ile sıkıştırma                     */
/* ------------------------------------------------------------------ */
/*
 * Girdi dosyasını normal şekilde açar, çıktıyı ise zlib'in gzFile
 * akışıyla açar. Veriyi 64 KB'lik parçalar halinde okuyup gzwrite
 * ile sıkıştırılmış olarak yazar; böylece dosyanın tamamı belleğe
 * yüklenmez (130 MB'lik dosyada bile düşük bellek kullanımı).
 */
int compression_compress_file(const char *in_path, const char *out_path)
{
    FILE *in = fopen(in_path, "rb");
    if (!in) {
        fprintf(stderr, "[HATA] Okunamadi: %s\n", in_path);
        return -1;
    }

    /* "wb6" => yazma + binary + sıkıştırma seviyesi 6 (varsayılan denge) */
    gzFile out = gzopen(out_path, "wb6");
    if (!out) {
        fprintf(stderr, "[HATA] Sikistirilmis dosya acilamadi: %s\n", out_path);
        fclose(in);
        return -1;
    }

    unsigned char buf[65536];
    size_t        n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (gzwrite(out, buf, (unsigned)n) != (int)n) {
            fprintf(stderr, "[HATA] Sikistirma yazma hatasi: %s\n", out_path);
            fclose(in);
            gzclose(out);
            return -1;
        }
    }

    fclose(in);
    if (gzclose(out) != Z_OK) {
        fprintf(stderr, "[HATA] Sikistirma kapatma hatasi: %s\n", out_path);
        return -1;
    }
    return 0;
}
