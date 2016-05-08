/* sha.h sha512 and sha384 hash functions */
#ifndef SHA512_H
#define SHA512_H

#if _MSC_VER >= 1300

# define int64_t __int64
# define int32_t __int32
# define int16_t __int16
# define int8_t  __int8
# define uint64_t unsigned __int64
# define uint32_t unsigned __int32
# define uint16_t unsigned __int16
# define uint8_t  unsigned __int8

/* disable warnings: The POSIX name for this item is deprecated. Use the ISO C++ conformant name. */
#pragma warning(disable : 4996)

#else /* _MSC_VER >= 1300 */

# include <stdint.h>
# include <unistd.h>

#endif /* _MSC_VER >= 1300 */

#ifdef __cplusplus
extern "C" {
#endif

#define sha512_block_size 128
#define sha512_hash_size  64
#define sha384_hash_size  48

/* algorithm context */
typedef struct sha512_ctx
{
	uint64_t message[16];   /* 1024-bit buffer for leftovers */
	uint64_t length;        /* number of processed bytes */
	uint64_t hash[8];       /* 512-bit algorithm internal hashing state */
	unsigned digest_length; /* length of the algorithm digest in bytes */
} sha512_ctx;

void rhash_sha384_init(sha512_ctx *ctx);
void rhash_sha512_init(sha512_ctx *ctx);
void rhash_sha512_update(sha512_ctx *ctx, const unsigned char* data, size_t length);
void rhash_sha512_final(sha512_ctx *ctx, unsigned char* result);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* SHA512_H */
