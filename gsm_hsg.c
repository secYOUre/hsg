/*
 * GSM, Hopping Sequence Generator
 * Copyright (c) 2008, 
 *      Alfonso De Gregorio <adg@crypto.lo.gy>. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Alfonso De Gregorio nor the names of its
 *    contributors may be used to endorse or promote products derived from 
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdint.h>
#include <math.h>

static const uint8_t RNTABLE[114] = {
         48,  98,  63,   1,  36,  95,  78, 102,  94,  73,
          0,  64,  25,  81,  76,  59, 124,  23, 104, 100,
        101,  47, 118,  85,  18,  56,  96,  86,  54,   2,
         80,  34, 127,  13,   6,  89,  57, 103,  12,  74,
         55, 111,  75,  38, 109,  71, 112,  29,  11,  88,
         87,  19,   3,  68, 110,  26,  33,  31,   8,  45,
         82,  58,  40, 107,  32,   5, 106,  92,  62,  67,
         77, 108, 122,  37,  60,  66, 121,  42,  51, 126,
        117, 114,   4,  90,  43,  52,  53, 113, 120,  72,
         16,  49,   7,  79, 119,  61,  22,  84,   9,  97,
         91,  15,  21,  24,  46,  39,  93, 105,  65,  70,
        125,  99,  17, 123
};


/* 
 * Hopping Sequence Generator
 * as described in 3GPP TS 05.02 v8.10.0 (2001-08) - sec. 6.2.3
 * hsg() returns "the index to an absolute radio frequency channel (ARFCN)
 * within the mobile allocation (MAI from 0 to N-1, where MAI=0 represents
 * the lowest ARFCN in the mobile allocation, ARFCN is in the range 
 * 0 to 1023 and the frequency value can be determined according to
 * 3GPP TS 05.05"
 *
 * Pseudo-code:
 * FN :=  51 * ((T3-T2) mod 26) + T3 + 51 * 26 * T1
 * 
 * if HSN == 0
 * MAI := (FN + MAIO) mod N
 * else
 * M   := (T2 + RNTABLE[HSN XOR ((T1 mod 64) + T3)])
 * M'  := (M mod 2^(INT(log2(N)+1)))
 * T'  := (T3 mod 2^(INT(log2(N)+1)))
 * if M' < N    
 * S := M'
 * else
 * S := (M' + T') mod N
 * MAI := (MAIO + S) mod N

 * RFCHN := MA[MAI]
 */
static int16_t
hsg (const uint8_t t1, const uint8_t t2, const uint8_t t3,
     const uint16_t maio, const uint8_t hsn, const uint8_t hsg_n)
{
  uint16_t mai;                 /* Mobile Allocation Index */
  uint16_t hsg_nbin, hsg_modulo_nbin;

  /* Sanity check */
  if (t1 > 63 || t2 > 25 || t3 > 50 || hsg_n > 64) return -1;

  /* Compute the Mobile Allocation Index */
  if (hsn == 0) {
    register uint16_t fn;                /* Frame Number */

    /* Retrieve the Frame Number (FN) from T1, T2 and T3 */
    fn = 51 * ((t3 - t2) % 26) + t3 + 51 * 26 * t1;
 mai = (fn + maio) % hsg_n;
  } else {
    register uint16_t m;

    /* how many bits do we need to encode N? */
    hsg_nbin = (uint8_t) floor (log2 ((double) hsg_n) + 1.0);
    hsg_modulo_nbin = 2 << hsg_nbin;

    m = t2 + RNTABLE[hsn ^ ((t1 % 64) + t3)];
    m = m % hsg_modulo_nbin;

    if (m < hsg_n)
      mai = (maio + m) % hsg_n;
    else {
      register uint16_t t;

      t = t3 % hsg_modulo_nbin;
      mai = (maio + ((m + t) % hsg_n)) % hsg_n;
    }
  }

  /* return the index to the radio frequency channel */
  return mai;
}


#if defined (__MAIN__)
#include <stdio.h>

int
main (int argc, char *argv[])
{
  uint8_t t1, t2, t3, maio;
  static const uint16_t MA_TEST[6] = {
    813, 820, 826, 850, 857, 880
  };

  /* Hopping Sequence Number */
  /* cyclic pattern */
  /* # define HSN  0 */

  /* pseudo-random pattern */
# define HSN 51

  /* 6 * 84864 rounds */
  for (t1 = 0; t1 <= 63; t1++) {
    for (t2 = 0; t2 <= 25; t2++) {
      for (t3 = 0; t3 <= 50; t3++) {
	/* where n = sizeof(MA_TEST)/sizeof(uint16_t) */
        for (maio = 0; maio < 6; maio++) {
          int16_t mai = hsg (t1, t2, t3, maio, HSN, 6);
          if (mai < 0 || mai > 6)
            printf ("Error: MAI - consistency check failed.\n");
          else
            printf ("HSN: %d\tT1: %d\tT2: %d\tT3: %d\tMAIO: %d\t" \
                    "MAI: %d\tMA[MAI]: %d\n",                     \
            HSN, t1, t2, t3, maio, mai, MA_TEST[mai]);
        }
      }
    }
  }

  return 0;
}
#endif /* __MAIN__  */
