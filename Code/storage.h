#ifndef STORAGE_H
#define STORAGE_H

/*
 * Saklama (storage) modülü — Aşama 2
 * ----------------------------------
 * Processed/passwords.txt dosyasındaki temizlenmiş veriyi iki farklı
 * yöntemle Storage/ altına kaydeder:
 *   1) Raw     : düz metin birebir kopya (referans dosyası)
 *   2) Hashed  : her parolanın OpenSSL ile hesaplanmış MD5 değeri
 *
 * (Sıkıştırma yöntemi ayrı bir modülde: compression.h)
 */

/*
 * in_path dosyasını out_path'e hiçbir değişiklik yapmadan kopyalar.
 * Dönüş: başarılıysa 0, hata durumunda negatif değer.
 */
int storage_save_raw(const char *in_path, const char *out_path);

/*
 * in_path içindeki her parolanın MD5 özetini (32 karakter onaltılık)
 * hesaplar ve out_path dosyasına satır satır yazar.
 * Dönüş: başarılıysa 0, hata durumunda negatif değer.
 */
int storage_save_hashed(const char *in_path, const char *out_path);

#endif /* STORAGE_H */
