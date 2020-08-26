/* src/libsecp256k1-config.h.  Generated from libsecp256k1-config.h.in
 by configure.  */
/* src/libsecp256k1-config.h.in.  Generated from configure.ac by autoh
eader.  */

#ifndef LIBSECP256K1_CONFIG_H

#define LIBSECP256K1_CONFIG_H

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define this symbol to compile out all VERIFY code */
/* #undef COVERAGE */

/* Define this symbol to enable the Grin Aggsig module */
#define ENABLE_MODULE_AGGSIG 1

/* Define this symbol to enable the Pedersen / zero knowledge bulletp
roof
   module */
#define ENABLE_MODULE_BULLETPROOF 1

/* Define this symbol to enable the Pedersen commitment module */
#define ENABLE_MODULE_COMMITMENT 1

/* Define this symbol to enable the ECDH module */
#define ENABLE_MODULE_ECDH 1

/* Define this symbol to enable the NUMS generator module */
#define ENABLE_MODULE_GENERATOR 1

/* Define this symbol to enable the zero knowledge range proof modu
le */
#define ENABLE_MODULE_RANGEPROOF 1

/* Define this symbol to enable the ECDSA pubkey recovery module */
#define ENABLE_MODULE_RECOVERY 1

/* Define this symbol to enable the schnorrsig module */
#define ENABLE_MODULE_SCHNORRSIG 1

/* Define this symbol to enable the surjection proof module */
#define ENABLE_MODULE_SURJECTIONPROOF 1

/* Define this symbol to enable the key whitelisting module */
/* #undef ENABLE_MODULE_WHITELIST */

/* Define this symbol if OpenSSL EC functions are available */
/* #undef ENABLE_OPENSSL_TESTS */

/* Name of package */
#define PACKAGE "libsecp256k1"

/* Define to the address where bug reports for this package should be
 sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "libsecp256k1"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libsecp256k1 0.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libsecp256k1"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.1"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define this symbol to enable x86_64 assembly optimizations */
#define USE_ASM_X86_64 1

/* Define this symbol to use a statically generated ecmult table */
#undef USE_ECMULT_STATIC_PRECOMPUTATION

/* Define this symbol to use endomorphism optimization */
/* #undef USE_ENDOMORPHISM */

/* Define this symbol if an external (non-inline) assembly implementatio
n is
   used */
/* #undef USE_EXTERNAL_ASM */

/* Define this symbol to use the FIELD_10X26 implementation */
#define USE_FIELD_10X26

/* Define this symbol to use the FIELD_5X52 implementation */
#define USE_FIELD_5X52 1

/* Define this symbol to use the native field inverse implementation */
#define USE_FIELD_INV_BUILTIN 1

/* Define this symbol to use the num-based field inverse implementatio
n */
/* #undef USE_FIELD_INV_NUM */

/* Define this symbol to use the gmp implementation for num */
/* #undef USE_NUM_GMP */

/* Define this symbol to use no num implementation */
#define USE_NUM_NONE 1

/* Define this symbol to use the 4x64 scalar implementation */
#define USE_SCALAR_4X64 1

/* Define this symbol to use the 8x32 scalar implementation */
#define USE_SCALAR_8X32

/* Define this symbol to use the native scalar inverse implementation */
#define USE_SCALAR_INV_BUILTIN 1

/* Define this symbol to use the num-based scalar inverse implementat
ion */
/* #undef USE_SCALAR_INV_NUM */

/* Version number of package */
#define VERSION "0.1"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with
 the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

#undef USE_ASM_X86_64
#undef USE_ENDOMORPHISM
#undef USE_FIELD_5X52
#undef USE_FIELD_INV_NUM
#undef USE_NUM_GMP
#undef USE_SCALAR_4X64
#undef USE_SCALAR_INV_NUM

#endif /*LIBSECP256K1_CONFIG_H*/
