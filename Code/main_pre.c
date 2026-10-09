#include "preprocess.h"

#include <stdio.h>

/*
 * Program akışı (Aşama 1 — Veri Ön İşleme)
 * ----------------------------------------
 * RawData/ klasörünü okur ve sonucu Processed/passwords.txt dosyasına
 * yazar. Program Code/ klasöründen çalıştırıldığı için yollar ".." göredir.
 *
 *     ./preprocess <RawData_klasoru> <cikti_dosyasi>
 *
 * NOT: Aşama 2 (saklama) için bu programı tekrar çalıştırmanıza gerek
 * yoktur; Processed/passwords.txt zaten hazırdır. Bu hedef, projenin
 * ilk aşamasını yeniden üretebilmek için korunmuştur.
 */

int main(int argc, char *argv[])
{
    const char *raw_dir  = (argc > 1) ? argv[1] : "../RawData";
    const char *out_path = (argc > 2) ? argv[2] : "../Processed/passwords.txt";

    printf("==============================================\n");
    printf(" Dosya Organizasyonu - Asama 1: On Isleme\n");
    printf("==============================================\n");
    printf("Kaynak klasor : %s\n", raw_dir);
    printf("Cikti dosyasi : %s\n\n", out_path);

    PreprocessStats stats;
    if (preprocess_run(raw_dir, out_path, &stats) != 0) {
        fprintf(stderr, "\nIslem basarisiz oldu.\n");
        return 1;
    }

    printf("\n---------------- OZET ----------------\n");
    printf("Islenen dosya         : %zu\n", stats.files_processed);
    printf("Okunan toplam satir   : %zu\n", stats.total_lines);
    printf("Temizlenen bos satir  : %zu\n", stats.empty_skipped);
    printf("Atlanan tekrar        : %zu\n", stats.duplicates_skipped);
    printf("Yazilan benzersiz     : %zu\n", stats.unique_written);
    printf("--------------------------------------\n");
    printf("Tamamlandi -> %s\n", out_path);

    return 0;
}
