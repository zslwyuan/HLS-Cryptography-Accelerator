#include <stdio.h>
#include <assert.h>
//#include <stdlib.h> for crypt()
#include "SHA512.h"
#include "unix_cracker.h"

static const uint8_t P[] = {
  42, 21,  0,  1, 43, 22, 23,  2, 44,
  45, 24,  3,  4, 46, 25, 26,  5, 47,
  48, 27,  6,  7, 49, 28, 29,  8, 50,
  51, 30,  9, 10, 52, 31, 32, 11, 53,
  54, 33, 12, 13, 55, 34, 35, 14, 56,
  57, 36, 15, 16, 58, 37, 38, 17, 59,
  60, 39, 18, 19, 61, 40, 41, 20, 62,
  63
};

static const char b64t[65] =
"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";


static inline SHA512ByteHash runIters(int slen, int pwlen,
                                      const SHA512ByteHash &A,
                                      const uint8_t DS[SHA512Hasher::HASH_SIZE],
                                      const uint8_t DP[SHA512Hasher::HASH_SIZE],
                                      int nrounds=5000) {
  SHA512Hasher Ch;
  SHA512ByteHash C = A;

  for (int i=0; i < nrounds; i++) {
    Ch.reset();

    if (i % 2 == 1) {
      Ch.update<MAX_PWD_LEN>(DP, pwlen);
    } else {
      Ch.update<SHA512Hasher::HASH_SIZE>(C.hash, SHA512Hasher::HASH_SIZE);
    }

    if (i % 3 != 0) {
      Ch.update<MAX_SALT_LEN>(DS, slen);
    }

    if (i % 7 != 0) {
      Ch.update<MAX_PWD_LEN>(DP, pwlen);
    }

    if (i % 2 == 0) {
      Ch.update<MAX_PWD_LEN>(DP, pwlen);
    } else {
      Ch.update<SHA512Hasher::HASH_SIZE>(C.hash, SHA512Hasher::HASH_SIZE);
    }

    C = Ch.byte_digest();
  }
  return C;

}

void calc(char hash[86], const char pwd[MAX_PWD_LEN], const uint8_t pwlen, const char salt[MAX_SALT_LEN], const uint8_t slen, int nrounds) {
  assert(pwlen <= 64);
  assert(slen <= 64);
  // Compute B
  SHA512Hasher Bh;
  Bh.update<MAX_PWD_LEN>(pwd, pwlen);
  Bh.update<MAX_SALT_LEN>(salt, slen);
  Bh.update<MAX_PWD_LEN>(pwd, pwlen);
  SHA512ByteHash B = Bh.byte_digest();

  // Compute A
  SHA512Hasher Ah;
  Ah.update<MAX_PWD_LEN>(pwd, pwlen);
  Ah.update<MAX_SALT_LEN>(salt, slen);
  Ah.update<MAX_PWD_LEN>(B.hash, pwlen);

  uint8_t curr = pwlen;
  for (int i=0; i < 8; i++) {
    if (curr) {
      if (curr & 1) {
        Ah.update<SHA512Hasher::HASH_SIZE>(B.hash, sizeof(B.hash));
      } else {
        Ah.update<MAX_PWD_LEN>(pwd, pwlen);
      }
    }
    curr >>= 1;
  }
  SHA512ByteHash A = Ah.byte_digest();

  // Compute DP
  SHA512Hasher DPh;
  for (int i=0; i < pwlen; i++) {
    DPh.update<MAX_PWD_LEN>(pwd, pwlen);
  }
  SHA512ByteHash DP = DPh.byte_digest();
  // Compute DS
  SHA512Hasher DSh;
  for (int i=0; i < 16 + A.hash[0]; i++) {
    DSh.update<MAX_SALT_LEN>(salt, slen);
  }
  SHA512ByteHash DS = DSh.byte_digest();

  // Note P is the first N bytes of DP
  // We reuse A for C
  A = runIters(slen, pwlen, A, DS.hash, DP.hash, nrounds);

  // TODO: unroll this
  for (int i=0; i < 21; i++) {
    uint32_t C = A.hash[P[3*i]] | (A.hash[P[3*i + 1]] << 8) | (A.hash[P[3*i + 2]] << 16);
    for (int j=0; j < 4; j++) {
      hash[4*i + j] = b64t[(C >> (6*j)) & 0x3f];
    }
  }
  // Handle last byte
  uint8_t C = A.hash[P[63]];
  hash[84] = b64t[C & 0x3f];
  hash[85] = b64t[C >> 6];

}
