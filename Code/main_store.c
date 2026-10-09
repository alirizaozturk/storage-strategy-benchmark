#include "storage.h"
#include "compression.h"

#include <stdio.h>

/*
 * Program akışı (Aşama 2 — Veri Saklama / Storage)
 * ------------------------------------------------
 * Kaynak SADECE Processed/passwords.txt dosyasıdır (RawData kullanılmaz).
 * Temizlenmiş veri üç farklı yöntemle Storage/ altına kaydedilir.
 * (Aşama 3 için bu programı tekrar çalıştırmaya gerek yoktur.)
 */

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

int main(int argc, char *argv[])
{
    const char *in_path  = (argc > 1) ? argv[1] : "../Processed/passwords.txt";
    const char *raw_out  = "../Storage/Raw/passwords.txt";
    const char *hash_out = "../Storage/Hashed/passwords.md5.txt";
    const char *comp_out = "../Storage/Compressed/passwords.txt.gz";

    printf("==============================================\n");
    printf(" Dosya Organizasyonu - Asama 2: Saklama\n");
    printf("==============================================\n");
    printf("Kaynak: %s\n\n", in_path);

    printf("[1/3] Raw (duz metin) kaydediliyor...\n");
    if (storage_save_raw(in_path, raw_out) != 0)
        return 1;
    printf("      -> %s\n\n", raw_out);

    printf("[2/3] Hashed (MD5) kaydediliyor...\n");
    if (storage_save_hashed(in_path, hash_out) != 0)
        return 1;
    printf("      -> %s\n\n", hash_out);

    printf("[3/3] Compressed (zlib) kaydediliyor...\n");
    if (compression_compress_file(in_path, comp_out) != 0)
        return 1;
    printf("      -> %s\n\n", comp_out);

    long orig = file_size(in_path);
    long comp = file_size(comp_out);
    printf("---------------- OZET ----------------\n");
    printf("Orijinal boyut    : %ld bayt\n", orig);
    printf("Sikistirilmis     : %ld bayt\n", comp);
    if (orig > 0 && comp >= 0)
        printf("Sikistirma orani  : %%%.1f (kazanc)\n",
               100.0 * (1.0 - (double)comp / (double)orig));
    printf("--------------------------------------\n");
    printf("Tum saklama yontemleri tamamlandi.\n");

    return 0;
}
