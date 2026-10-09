#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <stddef.h>

/*
 * Ön işleme (preprocessing) modülü
 * --------------------------------
 * RawData/ klasöründeki ham parola listelerini okur,
 * boş satırları ve tekrar eden parolaları temizler,
 * birleştirilmiş benzersiz sonucu tek bir dosyaya yazar.
 */

/* İşlem sonunda elde edilen istatistikleri tutar. */
typedef struct {
    size_t files_processed;    /* başarıyla okunan dosya sayısı   */
    size_t total_lines;        /* okunan toplam ham satır         */
    size_t empty_skipped;      /* temizlenen boş satır sayısı     */
    size_t duplicates_skipped; /* atlanan tekrar (duplicate) sayısı */
    size_t unique_written;     /* dosyaya yazılan benzersiz parola */
} PreprocessStats;

/*
 * raw_dir içindeki tüm .txt ve .csv dosyalarını işler,
 * temizlenmiş ve tekilleştirilmiş parolaları out_path dosyasına yazar.
 *
 * stats NULL değilse, çalışma sonundaki istatistikler buraya doldurulur.
 *
 * Dönüş: başarılıysa 0, hata durumunda negatif bir değer.
 */
int preprocess_run(const char *raw_dir, const char *out_path,
                   PreprocessStats *stats);

#endif /* PREPROCESS_H */
