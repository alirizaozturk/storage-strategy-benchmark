# Storage Strategy Benchmark — Raw vs. Hashed vs. Compressed

A C project that measures the **disk / speed / security trade-off** of storing a large password dataset (13.6 million entries) in three different formats, and searching it.

## Pipeline

The project is split into three separate programs, one per stage:

1. **Preprocess** (`preprocess`) — merges 37 password lists from [SecLists](https://github.com/danielmiessler/SecLists), drops empty lines and removes duplicates with a hand-written hash set (separate chaining, djb2 hash, resizes as it grows).
   Result: **13,644,552 unique passwords (123.6 MB)**.
2. **Store** (`storage`) — writes the cleaned data three ways:
   - **Raw** — plain-text copy
   - **Hashed** — one MD5 digest per line (OpenSSL EVP API)
   - **Compressed** — gzip, streamed through zlib
3. **Search & measure** (`search`) — picks 100 random passwords in a single pass (reservoir sampling), runs a linear search for each one in all three formats and records:
   - wall time per query (`clock_gettime`)
   - CPU usage (`getrusage`)
   - memory (`/proc/self/statm`, `ru_maxrss`)

The search stage reads files line by line instead of loading them into memory, so its peak memory stays around **5 MB**, even on the 430 MB hash file.

## Results

From [`Results/performance_results.txt`](Results/performance_results.txt). Each method ran 100 lookups and found all 100.

| Method | Disk (MB) | Avg. search (ms) | Max. search (ms) | Memory (MB) |
|---|---:|---:|---:|---:|
| Raw | 123.56 | 146.86 | 318.96 | 2.88 |
| Hashed (MD5) | 429.41 | 132.25 | 277.32 | 5.12 |
| Compressed (gzip) | **56.67** | 416.39 | 858.42 | 5.12 |

- Compression ratio: **54.1% smaller** (2.18×)
- Full decompression: 579.8 ms
- Peak RSS: 5.25 MB

### Takeaways

- **Compression** cuts disk usage in half but makes search **~2.8× slower**, because the data has to be decompressed on every lookup.
- **Hashing** makes the file **~3.5× larger**: every password becomes a fixed 32-character hex digest, while the average password is only 8–9 characters. Search speed stays about the same as Raw, since both are still linear scans. Hashing pays off in security, not speed.
- **The real speed-up comes from indexing, not from the storage format.** A hash table would give O(1) lookups and a sorted file with binary search would give O(log n). This is the natural next step for the project.

## Build & run (Linux)

```bash
sudo apt install build-essential libssl-dev zlib1g-dev

# 1. Get the raw data (not included in this repo)
git clone --depth 1 https://github.com/danielmiessler/SecLists.git
cp SecLists/Passwords/Common-Credentials/*.txt \
   SecLists/Passwords/Common-Credentials/*.csv RawData/

# 2. Build and run each stage (paths are relative to Code/)
cd Code
make
./preprocess      # RawData/          -> Processed/passwords.txt
./storage         # Processed/        -> Storage/{Raw,Hashed,Compressed}/
./search 100      # 100 random lookups -> Results/performance_results.txt
```

The measurement code uses Linux-only interfaces (`/proc`, `getrusage`), so it won't run on Windows or macOS as-is.

## Repository layout

```
Code/        C sources + Makefile (preprocess, storage, search, utils)
Report/      Full project report (Turkish): design, complexity analysis, results
Results/     Measured output of the search stage
RawData/     Input lists          (generated locally, not committed)
Processed/   Cleaned dataset      (generated locally, not committed)
Storage/     Raw / Hashed / Compressed outputs (generated locally, not committed)
```

## Notes

- The datasets are not committed. The source lists are public on SecLists, and the generated files are 56–430 MB, beyond what belongs in a Git repo.
- The timings come from a single run (sampling seed `1781818109`). They depend on the hardware and the OS page cache, so expect different absolute numbers on your machine. The relative ordering is what matters.
- This was built as a course project for *File Organization* (Spring 2026).
