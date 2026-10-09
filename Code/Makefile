# Dosya Organizasyonu Projesi - Makefile (Linux)
#
# Hedefler:
#   make            -> tum asamalari derler (preprocess, storage, search)
#   make preprocess -> Asama 1: On Isleme
#   make storage    -> Asama 2: Saklama        (OpenSSL + zlib)
#   make search     -> Asama 3: Arama/Analiz   (OpenSSL + zlib)
#   make run        -> Asama 3 programini calistirir
#   make clean      -> uretilen dosyalari siler
#
# Gerekli kutuphaneler (Linux):
#   sudo apt install build-essential libssl-dev zlib1g-dev

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2

# -lcrypto -> OpenSSL (MD5),  -lz -> zlib (sikistirma)
LIBS    = -lcrypto -lz

all: preprocess storage search

# ---- Asama 1: On Isleme ----
preprocess: main_pre.o preprocess.o
	$(CC) $(CFLAGS) -o preprocess main_pre.o preprocess.o

# ---- Asama 2: Saklama ----
storage: main_store.o storage.o compression.o
	$(CC) $(CFLAGS) -o storage main_store.o storage.o compression.o $(LIBS)

# ---- Asama 3: Arama / Performans Analizi ----
search: main.o search.o utils.o
	$(CC) $(CFLAGS) -o search main.o search.o utils.o $(LIBS)

# ---- Derleme kurallari (.c -> .o) ----
main.o: main.c search.h utils.h
	$(CC) $(CFLAGS) -c main.c

search.o: search.c search.h
	$(CC) $(CFLAGS) -c search.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

main_store.o: main_store.c storage.h compression.h
	$(CC) $(CFLAGS) -c main_store.c

storage.o: storage.c storage.h
	$(CC) $(CFLAGS) -c storage.c

compression.o: compression.c compression.h
	$(CC) $(CFLAGS) -c compression.c

main_pre.o: main_pre.c preprocess.h
	$(CC) $(CFLAGS) -c main_pre.c

preprocess.o: preprocess.c preprocess.h
	$(CC) $(CFLAGS) -c preprocess.c

run: search
	./search

clean:
	rm -f preprocess storage search *.o

.PHONY: all run clean
