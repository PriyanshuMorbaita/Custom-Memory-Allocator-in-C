/* Autthor PRIYANSHU MORBAITA
   DATE :- 02-06-2025   
*/

#include "alloc.h"

extern heap *memspace;

static void zero(int8 *ptr, int32 n) {
    memset(ptr, 0, n);
}

bool destroy(void* addr) {
    header *p;
    int16 n;
    void* mem;
    word w;

    mem = (void*)((int8*)addr - 4);
    p = (header*)mem;
    w = (p->w == ZERO_WORDS) ? 0 : p->w;

    if ((!w) || (!p->alloced)) {
        RET_ERROR(ERR_DOUBLE_FREE);
    }

    n = ((p->w - 1) * 4);
    zero((int8*)addr, n);
    p->alloced = false;

    return true;
}

static header* findblock_(header *hdr, word allocation, word n) {
    bool ok;
    void* mem;
    header *hdr_;
    word n_, w;

    if ((n + allocation) > (MAX_WORDS - 2)) {
        RET_ERROR(ERR_NO_MEM);
    }

    w = (hdr->w == ZERO_WORDS) ? 0 : hdr->w;

    ok = (!hdr->w) || (!hdr->alloced && (w >= allocation));

    if (ok)
        return hdr;
    else {
        mem = (void*)((int8*)hdr + (w * 4) + 4);
        hdr_ = (header*)mem;
        n_ = n + w;
        return findblock_(hdr_, allocation, n_);
    }

    RET_ERROR(ERR_UNKNOWN);
}

static void* mkalloc(word words, header *hdr) {
    void *ret, *bytesin, *mem;
    word wordsin, diff;
    header *hdr_;

    bytesin = (void*)((int8*)hdr - (int8*)memspace);
    wordsin = ((word)(intptr_t)bytesin / 4) + 1;

    if (words > (MAX_WORDS - wordsin)) {
        RET_ERROR(ERR_NO_MEM);
    }

    if (hdr->w > words) {
        diff = hdr->w - words;
        mem = (void*)((int8*)hdr + (words * 4) + 4);
        hdr_ = (header*)mem;
        diff--;
        hdr_->w = (!diff) ? ZERO_WORDS : diff;
        hdr_->alloced = false;
    }

    hdr->w = words;
    hdr->alloced = true;
    ret = (void*)((int8*)hdr + 4);

    return ret;
}

void* alloc(int32 size_in_bytes) {
    word words;
    header *hdr;
    void *mem;

    words = (size_in_bytes + BLOCKSIZE - 1) / BLOCKSIZE;

    mem = (void*)memspace;
    hdr = (header*)mem;

    hdr = findblock_(hdr, words, 0);

    if (!hdr)
        return (void*)0;

    if (words > MAX_WORDS)
        RET_ERROR(ERR_NO_MEM);

    mem = mkalloc(words, hdr);
    if (!mem)
        return (void*)0;

    return mem;
}

void show_allocation(void) {
    header *p;
    void *mem;
    int32 n;
    word w;

    for (n = 1, p = (header*)memspace, w = (p->w == ZERO_WORDS) ? 0 : p->w; p->w;
         mem = (void*)((int8*)p + ((w + 1) * 4)), p = (header*)mem, w = (p->w == ZERO_WORDS) ? 0 : p->w, n++) {

        if (!w) {
            printf("Empty header at 0x%.08x, moving on\n", (int)(intptr_t)p);
            continue;
        }

        printf("0x%.08x Alloc %d = %d %s words\n", (int)(intptr_t)((int8*)p + 4),
               n, w, (p->alloced) ? "alloced" : "free");
    }

    return;
}

int main(int argc, char* argv[]) {
    int8 *p;
    int8 *p2;
    int8 *p3;
    int8 *p4;

    p = (int8*)alloc(7);
    p2 = (int8*)alloc(2000);
    p3 = (int8*)alloc(10);
    destroy(p2);
    p4 = (int8*)alloc(1996);
    show_allocation();

    return 0;
}
