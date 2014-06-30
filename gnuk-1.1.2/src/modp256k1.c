/*
 * modp256k1.c -- modulo arithmetic for p256k1
 *
 * Copyright (C) 2014 Free Software Initiative of Japan
 * Author: NIIBE Yutaka <gniibe@fsij.org>
 *
 * This file is a part of Gnuk, a GnuPG USB Token implementation.
 *
 * Gnuk is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Gnuk is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * p256k1 =  2^256 - 2^32 - 2^9 - 2^8 - 2^7 - 2^6 - 2^4 - 1
 */
#include <stdint.h>
#include <string.h>

#include "bn.h"
#include "modp256k1.h"

/*
256      224      192      160      128       96       64       32        0
2^256
  1 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
2^256 - 2^32
  0 ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff 00000000
2^256 - 2^32 - 2^9
  0 ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffe fffffe00
2^256 - 2^32 - 2^9 - 2^8
  0 ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffe fffffd00
2^256 - 2^32 - 2^9 - 2^8 - 2^7
  0 ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffe fffffc80
2^256 - 2^32 - 2^9 - 2^8 - 2^7 - 2^6
  0 ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffe fffffc40
2^256 - 2^32 - 2^9 - 2^8 - 2^7 - 2^6 - 2^4
  0 ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffe fffffc30
2^256 - 2^32 - 2^9 - 2^8 - 2^7 - 2^6 - 2^4 - 1
  0 ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffe fffffc2f
*/
const bn256 p256k1 = { {0xfffffc2f, 0xfffffffe, 0xffffffff, 0xffffffff,
			0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff } };

/**
 * @brief  X = (A + B) mod p256k1
 */
void
modp256k1_add (bn256 *X, const bn256 *A, const bn256 *B)
{
  uint32_t carry;
  bn256 tmp[1];

  carry = bn256_add (X, A, B);
  if (carry)
    bn256_sub (X, X, P256K1);
  else
    bn256_sub (tmp, X, P256K1);
}

/**
 * @brief  X = (A - B) mod p256
 */
void
modp256k1_sub (bn256 *X, const bn256 *A, const bn256 *B)
{
  uint32_t borrow;
  bn256 tmp[1];

  borrow = bn256_sub (X, A, B);
  if (borrow)
    bn256_add (X, X, P256K1);
  else
    bn256_add (tmp, X, P256K1);
}

/**
 * @brief  X = A mod p256k1
 */
void
modp256k1_reduce (bn256 *X, const bn512 *A)
{
  bn256 tmp[1];
  uint32_t carry;
#define borrow carry
  uint32_t s0, s1;
#define s00 tmp->word[0]
#define s01 tmp->word[1]
#define s02 tmp->word[2]

#define W0 X
#define W1 tmp
#define W2 tmp
#define W3 tmp
#define W4 tmp
#define W5 tmp
#define W6 tmp
#define W7 tmp
#define S  tmp

  /*
   * Suppose: P256K1 = 2^256 - CONST
   * Then, compute: W = A_low + A_high * CONST
   *                256-bit W0 = W mod 2^256
   *                64-bit (S1, S0) = W / 2^256
   * where: CONST = 2^32 + 2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1
   */

  /* W0 = A_low   */
  /* W7 = A_high  */
  /* W0 += W7 */
  carry = bn256_add (W0, (const bn256 *)&A->word[8], (const bn256 *)A);

  /* W6 = W7 << 4 */
  /* W0 += W6 */
  bn256_shift (W6, (const bn256 *)&A->word[8], 4);
  carry += bn256_add (W0, W0, W6);

  /* W5 = W6 << 2 */
  /* W0 += W5 */
  bn256_shift (W5, W6, 2);
  carry += bn256_add (W0, W0, W5);

  /* W4 = W5 << 1 */
  /* W0 += W4 */
  bn256_shift (W4, W5, 1);
  carry += bn256_add (W0, W0, W4);

  /* W3 = W4 << 1 */
  /* W0 += W3 */
  bn256_shift (W3, W4, 1);
  carry += bn256_add (W0, W0, W3);

  /* W2 = W3 << 1 */
  /* W0 += W2 */
  bn256_shift (W2, W3, 1);
  carry += bn256_add (W0, W0, W2);

  /* W1 = A_high << 32 */
  /* W0 += W1 */
  W1->word[7] = A->word[14];
  W1->word[6] = A->word[13];
  W1->word[5] = A->word[12];
  W1->word[4] = A->word[11];
  W1->word[3] = A->word[10];
  W1->word[2] = A->word[9];
  W1->word[1] = A->word[8];
  W1->word[0] = 0;
  carry += bn256_add (W0, W0, W1);

  /* (S1, S0) = W / 2^256 */
  s0 = A->word[15];
  carry += (s0 >> 28) + (s0 >> 26) + (s0 >> 25) + (s0 >> 24) + (s0 >> 23);
  carry += s0;
  s1 = (carry < s0) ? 1 : 0;
  s0 = carry;

  /*
   * Compute: S:=(S02, S01, S00), S = (S1,S0)*CONST
   */
  S->word[7] = S->word[6] = S->word[5] = S->word[4] = S->word[3] = 0;

  /* (S02, S01, S00) = (S1, S0) + (S1, S0)*2^32 */ 
  s00 = s0;
  s01 = s0 + s1;
  s02 = s1 + ((s01 < s0)? 1 : 0);

  /* (S02, S01, S00) += (S1, S0)*2^9 */ 
  carry = (s0 >> 23) + s01;
  s02 += (s1 >> 23) + ((carry < s01)? 1 : 0);
  s01 = (s1 << 9) + carry;
  s02 += ((s01 < carry)? 1 : 0);
  s00 += (s0 << 9);
  carry = ((s00 < (s0 << 9))? 1 : 0);
  s01 += carry;
  s02 += ((s01 < carry)? 1 : 0);

  /* (S02, S01, S00) += (S1, S0)*2^8 */ 
  carry = (s0 >> 24) + s01;
  s02 += (s1 >> 24) + ((carry < s01)? 1 : 0);
  s01 = (s1 << 8) + carry;
  s02 += ((s01 < carry)? 1 : 0);
  s00 += (s0 << 8);
  carry = ((s00 < (s0 << 8))? 1 : 0);
  s01 += carry;
  s02 += ((s01 < carry)? 1 : 0);

  /* (S02, S01, S00) += (S1, S0)*2^7 */ 
  carry = (s0 >> 25) + s01;
  s02 += (s1 >> 25) + ((carry < s01)? 1 : 0);
  s01 = (s1 << 7) + carry;
  s02 += ((s01 < carry)? 1 : 0);
  s00 += (s0 << 7);
  carry = ((s00 < (s0 << 7))? 1 : 0);
  s01 += carry;
  s02 += ((s01 < carry)? 1 : 0);

  /* (S02, S01, S00) += (S1, S0)*2^6 */ 
  carry = (s0 >> 26) + s01;
  s02 += (s1 >> 26) + ((carry < s01)? 1 : 0);
  s01 = (s1 << 6) + carry;
  s02 += ((s01 < carry)? 1 : 0);
  s00 += (s0 << 6);
  carry = ((s00 < (s0 << 6))? 1 : 0);
  s01 += carry;
  s02 += ((s01 < carry)? 1 : 0);

  /* (S02, S01, S00) += (S1, S0)*2^4 */ 
  carry = (s0 >> 28) + s01;
  s02 += (s1 >> 28) + ((carry < s01)? 1 : 0);
  s01 = (s1 << 4) + carry;
  s02 += ((s01 < carry)? 1 : 0);
  s00 += (s0 << 4);
  carry = ((s00 < (s0 << 4))? 1 : 0);
  s01 += carry;
  s02 += ((s01 < carry)? 1 : 0);

  /* W0 += S */
  modp256k1_add (W0, W0, S);

  borrow = bn256_sub (tmp, W0, P256K1);
  if (borrow)
    memcpy (tmp, W0, sizeof (bn256));
  else
    memcpy (W0, tmp, sizeof (bn256));

#undef W0
#undef W1
#undef W2
#undef W3
#undef W4
#undef W5
#undef W6
#undef W7
#undef S
#undef s00
#undef s01
#undef s02
#undef borrow
}

/**
 * @brief  X = (A * B) mod p256k1
 */
void
modp256k1_mul (bn256 *X, const bn256 *A, const bn256 *B)
{
  bn512 AB[1];

  bn256_mul (AB, A, B);
  modp256k1_reduce (X, AB);
}

/**
 * @brief  X = A * A mod p256k1
 */
void
modp256k1_sqr (bn256 *X, const bn256 *A)
{
  bn512 AA[1];

  bn256_sqr (AA, A);
  modp256k1_reduce (X, AA);
}

/**
 * @brief  C = (1 / a)  mod p256k1
 *
 * Return -1 on error.
 * Return 0 on success.
 */
#define MAX_N_BITS 256

int
modp256k1_inv (bn256 *C, const bn256 *a)
{
  bn256 u[1], v[1], tmp[1];
  bn256 A[1] = { { { 1, 0, 0, 0, 0, 0, 0, 0 } } };
  uint32_t carry;
  int n = MAX_N_BITS * 3;

  if (bn256_is_zero (a))
    return -1;

  memset (C, 0, sizeof (bn256));
  memcpy (u, a, sizeof (bn256));
  memcpy (v, P256K1, sizeof (bn256));

  while (n--)
    {
      int c = (bn256_is_even (u) << 1) + bn256_is_even (v);

      switch (c)
	{
	case 3:
	  bn256_shift (u, u, -1);
	  if (bn256_is_even (A))
	    {
	      bn256_add (tmp, A, P256K1);
	      carry = 0;
	    }
	  else
	    carry = bn256_add (A, A, P256K1);

	  bn256_shift (A, A, -1);
	  A->word[7] |= carry * 0x80000000;

	  bn256_shift (v, v, -1);
	  if (bn256_is_even (C))
	    {
	      bn256_add (tmp, C, P256K1);
	      carry = 0;
	    }
	  else
	    carry = bn256_add (C, C, P256K1);

	  bn256_shift (C, C, -1);
	  C->word[7] |= carry * 0x80000000;

	  if (bn256_is_ge (tmp, tmp))
	    {
	      bn256_sub (tmp, tmp, tmp);
	      modp256k1_sub (tmp, tmp, tmp);
	    }
	  else
	    {
	      bn256_sub (tmp, tmp, tmp);
	      modp256k1_sub (tmp, tmp, A);
	    }
	  break;

	case 1:
	  bn256_shift (tmp, tmp, -1);
	  if (bn256_is_even (tmp))
	    {
	      bn256_add (tmp, tmp, P256K1);
	      carry = 0;
	    }
	  else
	    carry = bn256_add (tmp, tmp, P256K1);

	  bn256_shift (tmp, tmp, -1);
	  tmp->word[7] |= carry * 0x80000000;

	  bn256_shift (v, v, -1);
	  if (bn256_is_even (C))
	    {
	      bn256_add (tmp, C, P256K1);
	      carry = 0;
	    }
	  else
	    carry = bn256_add (C, C, P256K1);

	  bn256_shift (C, C, -1);
	  C->word[7] |= carry * 0x80000000;

	  if (bn256_is_ge (tmp, tmp))
	    {
	      bn256_sub (tmp, tmp, tmp);
	      modp256k1_sub (tmp, tmp, tmp);
	    }
	  else
	    {
	      bn256_sub (tmp, tmp, tmp);
	      modp256k1_sub (tmp, tmp, A);
	    }
	  break;

	case 2:
	  bn256_shift (u, u, -1);
	  if (bn256_is_even (A))
	    {
	      bn256_add (tmp, A, P256K1);
	      carry = 0;
	    }
	  else
	    carry = bn256_add (A, A, P256K1);

	  bn256_shift (A, A, -1);
	  A->word[7] |= carry * 0x80000000;

	  bn256_shift (tmp, tmp, -1);
	  if (bn256_is_even (tmp))
	    {
	      bn256_add (tmp, tmp, P256K1);
	      carry = 0;
	    }
	  else
	    carry = bn256_add (tmp, tmp, P256K1);

	  bn256_shift (tmp, tmp, -1);
	  tmp->word[7] |= carry * 0x80000000;

	  if (bn256_is_ge (tmp, tmp))
	    {
	      bn256_sub (tmp, tmp, tmp);
	      modp256k1_sub (tmp, tmp, tmp);
	    }
	  else
	    {
	      bn256_sub (tmp, tmp, tmp);
	      modp256k1_sub (tmp, tmp, A);
	    }
	  break;

	case 0:
	  bn256_shift (tmp, tmp, -1);
	  if (bn256_is_even (tmp))
	    {
	      bn256_add (tmp, tmp, P256K1);
	      carry = 0;
	    }
	  else
	    carry = bn256_add (tmp, tmp, P256K1);

	  bn256_shift (tmp, tmp, -1);
	  tmp->word[7] |= carry * 0x80000000;

	  bn256_shift (tmp, tmp, -1);
	  if (bn256_is_even (tmp))
	    {
	      bn256_add (tmp, tmp, P256K1);
	      carry = 0;
	    }
	  else
	    carry = bn256_add (tmp, tmp, P256K1);

	  bn256_shift (tmp, tmp, -1);
	  tmp->word[7] |= carry * 0x80000000;

	  if (bn256_is_ge (u, v))
	    {
	      bn256_sub (u, u, v);
	      modp256k1_sub (A, A, C);
	    }
	  else
	    {
	      bn256_sub (v, v, u);
	      modp256k1_sub (C, C, A);
	    }
	  break;
	}
    }

  return 0;
}

/**
 * @brief  X = (A << shift) mod p256k1
 * @note   shift <= 32
 */
void
modp256k1_shift (bn256 *X, const bn256 *A, int shift)
{
  uint32_t carry;
  bn256 tmp[1];

  carry = bn256_shift (X, A, shift);
  if (shift < 0)
    return;

  memset (tmp, 0, sizeof (bn256));
  tmp->word[0] = carry + (carry << 9);
  tmp->word[1] = carry + (tmp->word[0] < (carry << 9)) + (carry >> 23);
  tmp->word[0] = tmp->word[0] + (carry << 8);
  tmp->word[1] = tmp->word[1] + (tmp->word[0] < (carry << 8)) + (carry >> 24);
  tmp->word[0] = tmp->word[0] + (carry << 7);
  tmp->word[1] = tmp->word[1] + (tmp->word[0] < (carry << 7)) + (carry >> 25);
  tmp->word[0] = tmp->word[0] + (carry << 6);
  tmp->word[1] = tmp->word[1] + (tmp->word[0] < (carry << 6)) + (carry >> 26);
  tmp->word[0] = tmp->word[0] + (carry << 4);
  tmp->word[1] = tmp->word[1] + (tmp->word[0] < (carry << 4)) + (carry >> 28);

  modp256k1_add (X, X, tmp);
}
