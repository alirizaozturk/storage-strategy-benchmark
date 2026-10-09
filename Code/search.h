#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>

/*
 * search modülü — Aşama 3
 * -----------------------
 * Arama setinin oluşturulması ve üç farklı yapıda arama:
 *   - Raw        : düz metin dosyasında doğrusal (linear) arama
 *   - Hashed     : sorgunun MD5'i hesaplanıp hash dosyasında aranır
 *   - Compressed : .gz dosyası akış halinde açılıp içinde aranır
 *
 * Üç arama fonksiyonu da aynı imzaya sahiptir, böylece tek bir
 * fonksiyon işaretçisiyle (function pointer) çağrılabilirler.
 */

/*
 * path dosyasından rastgele k parola seçer (reservoir sampling — tek
 * geçişte, tüm dosyayı belleğe almadan). Dönen dizi ve içindeki metinler
 * malloc'ludur; çağıran search_free_set ile serbest bırakmalıdır.
 *   out_count : gerçekte seçilen parola sayısı (NULL olabilir)
 *   out_total : dosyadaki toplam (boş olmayan) satır sayısı (NULL olabilir)
 * Hata durumunda NULL döner.
 */
char **search_pick_random(const char *path, int k,
                          size_t *out_count, size_t *out_total);
void   search_free_set(char **set, size_t count);

/* 1 = bulundu, 0 = bulunamadı, negatif = hata. (Ortak imza) */
int search_raw(const char *path, const char *query);
int search_hashed(const char *path, const char *query);
int search_compressed(const char *path, const char *query);

/*
 * .gz dosyasını baştan sona açar (arama yapmadan); açılan toplam bayt
 * sayısını döner. Yalnızca "decompression süresi" ölçümü içindir.
 * Hata durumunda -1.
 */
long search_decompress_all(const char *path);

#endif /* SEARCH_H */
