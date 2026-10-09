#ifndef COMPRESSION_H
#define COMPRESSION_H

/*
 * Sıkıştırma (compression) modülü — Aşama 2
 * -----------------------------------------
 * Temizlenmiş veriyi zlib kütüphanesi ile sıkıştırır.
 * Üretilen dosya standart gzip (.gz) biçimindedir; istenirse
 * `gunzip` ile geri açılabilir.
 */

/*
 * in_path dosyasını sıkıştırıp out_path (.gz) dosyasına yazar.
 * Bellek dostu olması için veri parça parça (streaming) işlenir.
 * Dönüş: başarılıysa 0, hata durumunda negatif değer.
 */
int compression_compress_file(const char *in_path, const char *out_path);

#endif /* COMPRESSION_H */
