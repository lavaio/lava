#define BOOST_TEST_MODULE maintest
#include <boost/test/unit_test.hpp>
#include <string>
#include <immintrin.h>
#include "../../src/shabal/shabal.h"

using namespace std;

int xdigit(char const digit) {
    int val;
    if ('0' <= digit && digit <= '9') val = digit - '0';
    else if ('a' <= digit && digit <= 'f') val = digit - 'a' + 10;
    else if ('A' <= digit && digit <= 'F') val = digit - 'A' + 10;
    else val = -1;
    return val;
}

size_t xstr2strr(char *buf, size_t const bufsize, const char *const in) {
    if (!in) return 0; // missing input string

    size_t inlen = (size_t)strlen(in);
    if (inlen % 2 != 0) inlen--; // hex string must even sized

    size_t i, j;
    for (i = 0; i < inlen; i++)
        if (xdigit(in[i]) < 0) return 0; // bad character in hex string

    if (!buf || bufsize < inlen / 2 + 1) return 0; // no buffer or too small

    for (i = 0, j = 0; i < inlen; i += 2, j++)
        buf[j] = xdigit(in[i]) * 16 + xdigit(in[i + 1]);

    buf[inlen / 2] = '\0';
    return inlen / 2 + 1;
}


BOOST_AUTO_TEST_CASE(test_shabal)
{
    string sigGen("5931be9dbf68ca08a2f6b3f52adfd18d4d1802bb2e479f0dcfa09d1c8adbe56b");
    char signature[33] = { 0 };
    xstr2strr(signature, 33, sigGen.c_str());
    shabal_context ctx;
    _mm256_zeroupper();
    shabal_init(&ctx, 256);
    shabal(&ctx, signature, 32);
    char hash[32];
    shabal_close(&ctx, 0, 0, hash);
    char check[] = { 
        0xaf, 0x78, 0x6f, 0xe8, 0x49, 0x60, 0xea, 0x95, 
        0xc1, 0xfd, 0x5c, 0x4a, 0x1d, 0xbc, 0x8d, 0xb6,
        0x57, 0xc3, 0x1d, 0xc5, 0x17, 0x42, 0x4d, 0x96,
        0x49, 0x25, 0x6b, 0x83, 0x99, 0xbc, 0x31, 0xd8 
    };
    size_t ret = memcmp(check, hash, 32);
    BOOST_CHECK_EQUAL(ret, 0);
}
