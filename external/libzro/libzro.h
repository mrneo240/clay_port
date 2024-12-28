#ifndef LIBZRO
#define LIBZRO

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>
#include <locale.h>
#include <stdint.h>
#ifdef ZROA_FUCK_NAMES
#define i8   int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t
#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#endif
typedef wchar_t Rune;

/* LIBZRO_ALLOCATOR */

#ifndef ZROA_NO_SHORT_NAMES
#define alloc_t         zroa_alloc_t
#define make            zroa_make
#define del             zroa_free
#define std_alloc       zroa_std_alloc_init
#define arena_alloc     zroa_arena_alloc_init
#define arena_free      zroa_arena_free_all
#define arena_t         zroa_arena_t
#define arena_init      zroa_arena_init
#endif
#ifndef ZRO_ARENA_ALIGNMENT
#define ZRO_ARENA_ALIGNMENT (2 * sizeof(void *))
#endif
#define is_power_of_two(x) ((x != 0) && ((x & (x - 1)) == 0))
#define zroa_make(T, n, a) ((T *)((a).alloc(sizeof(T) * n, (a).context)))
#define zroa_free(s, p, a) ((a).free(s, p, (a).context))
#define zroa_std_alloc_init() (zroa_alloc_t){_zroa_std_alloc, _zroa_std_free, _zroa_std_realloc, NULL}
#define zroa_arena_alloc_init(a) (zroa_alloc_t){_zroa_arena_alloc, _zroa_arena_free, _zroa_arena_realloc, a}

typedef struct {
    void *(*alloc)(size_t size, void *context);
    void (*free)(size_t size, void *ptr, void *context);
    void *(*realloc)(void *ptr, size_t old_size, size_t new_size, void *context);
    void *context;
} zroa_alloc_t;
typedef struct {
    void *base;
    size_t size;
    size_t offset;
    size_t committed;
} zroa_arena_t;
void *_zroa_std_alloc(size_t bytes, void *context) {
    (void)context;
    return malloc(bytes);
}
void _zroa_std_free(size_t size, void *ptr, void *context) {
    (void)ptr; (void)context;
    free(ptr);
}
void *_zroa_std_realloc(void *ptr, size_t old_size, size_t new_size, void *context){
    //(void)old_size;
    return realloc(ptr, new_size);
}
uintptr_t _zroa_arena_align_forward(uintptr_t ptr, size_t alignment) {
    uintptr_t p, a, modulo;
    if (!is_power_of_two(alignment)) {
        return 0;
    }

    p = ptr;
    a = (uintptr_t)alignment;
    modulo = p & (a - 1);

    if (modulo) {
        p += a - modulo;
    }

    return p;
}
void *_zroa_arena_alloc_aligned(zroa_arena_t *a, size_t size, size_t alignment) {
    uintptr_t curr_ptr = (uintptr_t)a->base + (uintptr_t)a->offset;
    uintptr_t offset = _zroa_arena_align_forward(curr_ptr, alignment);
    offset -= (uintptr_t)a->base;

    const size_t size_record_size = sizeof(size_t);

    if (offset + size + size_record_size > a->size) {
        return 0;
    }

    a->committed += size + size_record_size;
    void *ptr = (char*)a->base + offset;
    a->offset = offset + size + size_record_size;
    uint8_t *size_record_offset = (uint8_t*)ptr-size_record_size;
    memcpy(size_record_offset, &size, sizeof(uint32_t) );

    return ptr;
}
void *_zroa_arena_alloc(size_t size, void *context) {
    if (!size) {
        return 0;
    }
    return _zroa_arena_alloc_aligned((zroa_arena_t *)context, size, ZRO_ARENA_ALIGNMENT);
}
void* _zroa_arena_realloc(void *ptr, size_t old_size, size_t new_size, void *context){
    void *new_mem = _zroa_arena_alloc(new_size, context);
    if (ptr && new_mem)
        memcpy(new_mem, ptr, old_size);
        //if (h->a->free) {
        //    h->a->free(old_size, h, h->a->context);
        //}
    return new_mem;
}
void _zroa_arena_free(size_t size, void *ptr, void *context) {
    (void)ptr; (void)size; (void)context;
}
void zroa_arena_free_all(void *context) {
    zroa_arena_t *a = (zroa_arena_t *)context;
    a->offset = 0;
    a->committed = 0;
}
zroa_arena_t zroa_arena_init(void *buffer, size_t size) {
    return (zroa_arena_t){.base = buffer, .size = size-sizeof(size_t), .offset = sizeof(size_t)};
}

/* LIBZRO_DS */
#ifndef ZRODS_NO_SHORT_NAMES
#define array_header_t  zrods_array_header_t
#define array           zrods_array
#define ahead           zrods_array_header
#define alen            zrods_array_lenght
#define acap            zrods_array_capacity
#define apush           zrods_array_append
#define aundel          zrods_array_unordered_remove
#define aordel          zrods_array_ordered_remove
#define apop            zrods_array_pop_back
#endif
#define ARRAY_INITIAL_CAPACITY 16
typedef struct {
    size_t capacity;
    size_t lenght;
    // NOTE : is padding still necessary since Allocator is now aligned
    // thanks to realloc function pointer ?
    // --> size_t padding; <--
    zroa_alloc_t *a;
} zrods_array_header_t;
#define zrods_array(T, a) _zrods_array_init(sizeof(T), ARRAY_INITIAL_CAPACITY, a)
#define zrods_array_header(a) ((zrods_array_header_t *)(a) - 1)
#define zrods_array_lenght(a) (zrods_array_header(a)->lenght)
#define zrods_array_capacity(a) (zrods_array_header(a)->capacity)
#define zrods_array_append(a, v) ( \
    (a) = _zrods_array_ensure_capacity(a, 1, sizeof(v)), \
    (a)[zrods_array_header(a)->lenght] = (v), \
    &(a)[zrods_array_header(a)->lenght++])
#define zrods_array_unordered_remove(a, i) do { \
    zrods_array_header_t *h = zrods_array_header(a); \
    if (i == h->lenght - 1) { \
        h->lenght -= 1; \
    } else if (h->lenght > 1) { \
        void *ptr = &a[i]; \
        void *last = &a[h->lenght - 1]; \
        h->lenght -= 1; \
        memcpy(ptr, last, sizeof(*a)); \
    } \
} while (0);
#define zrods_array_ordered_remove(a, i) do { \
    zrods_array_header_t *h = zrods_array_header(a); \
    if (i == h->lenght - 1) { \
        h->lenght -= 1; \
    } else if (h->lenght > 1) { \
        void *ptr = &a[i]; \
        memmove(ptr, (char*)(ptr) + sizeof(*a),  (h->lenght - (i + 1)) * sizeof(*a)); \
        h->lenght -= 1; \
    } \
} while (0);
#define zrods_array_pop_back(a) (zrods_array_header(a)->lenght -= 1)
void *_zrods_array_init(size_t item_size, size_t capacity, zroa_alloc_t *a) {
    void *ptr = 0;
    size_t size = item_size * capacity + sizeof(zrods_array_header_t);
    zrods_array_header_t *h = a->alloc(size, a->context);

    if (h) {
        h->capacity = capacity;
        h->lenght = 0;
        h->a = a;
        ptr = h + 1;
    }

    return ptr;
}
void *_zrods_array_ensure_capacity(void *a, size_t item_count, size_t item_size) {
    zrods_array_header_t *h = zrods_array_header(a);
    size_t desired_capacity = h->lenght + item_count;

    if (h->capacity < desired_capacity) {
        size_t new_capacity = h->capacity * 2;
        while (new_capacity < desired_capacity) {
            new_capacity *= 2;
        }
        size_t new_size = sizeof(zrods_array_header_t) + new_capacity * item_size;
        size_t old_size = sizeof(*h) + h->lenght * item_size;
        zrods_array_header_t *new_h = h->a->realloc(h, old_size, new_size, h->a->context);
        if (new_h) {
            new_h->capacity = new_capacity;
            h = new_h + 1;
        } else {
            h = 0;
        }
    } else { h += 1; }
    return h;
}
#endif
