/*
 * call-rsa.c -- Glue code between RSA computation and OpenPGP card protocol
 *
 * Copyright (C) 2010, 2011, 2012, 2013 Free Software Initiative of Japan
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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

#include "gnuk.h"
#include "openpgp.h"
#include "random.h"
#include "polarssl/config.h"
#include "polarssl/rsa.h"

#define RSA_SIGNATURE_LENGTH KEY_CONTENT_LEN
 /* 256 byte == 2048-bit */
 /* 128 byte == 1024-bit */

static rsa_context rsa_ctx;

int
rsa_sign (const uint8_t *raw_message, uint8_t *output, int msg_len,
	  struct key_data *kd)
{
  mpi P1, Q1, H;
  int ret = 0;
  unsigned char temp[RSA_SIGNATURE_LENGTH];

  rsa_init (&rsa_ctx, RSA_PKCS_V15, 0);

  mpi_init (&P1);  mpi_init (&Q1);  mpi_init (&H);

  rsa_ctx.len = KEY_CONTENT_LEN;
  MPI_CHK( mpi_lset (&rsa_ctx.E, 0x10001) );
  MPI_CHK( mpi_read_binary (&rsa_ctx.P, &kd->data[0], rsa_ctx.len / 2) );
  MPI_CHK( mpi_read_binary (&rsa_ctx.Q, &kd->data[KEY_CONTENT_LEN/2],
			    rsa_ctx.len / 2) );
#if 0
  MPI_CHK( mpi_mul_mpi (&rsa_ctx.N, &rsa_ctx.P, &rsa_ctx.Q) );
#endif
  MPI_CHK( mpi_sub_int (&P1, &rsa_ctx.P, 1) );
  MPI_CHK( mpi_sub_int (&Q1, &rsa_ctx.Q, 1) );
  MPI_CHK( mpi_mul_mpi (&H, &P1, &Q1) );
  MPI_CHK( mpi_inv_mod (&rsa_ctx.D , &rsa_ctx.E, &H) );
  MPI_CHK( mpi_mod_mpi (&rsa_ctx.DP, &rsa_ctx.D, &P1) );
  MPI_CHK( mpi_mod_mpi (&rsa_ctx.DQ, &rsa_ctx.D, &Q1) );
  MPI_CHK( mpi_inv_mod (&rsa_ctx.QP, &rsa_ctx.Q, &rsa_ctx.P) );
 cleanup:
  mpi_free (&P1);  mpi_free (&Q1);  mpi_free (&H);
  if (ret == 0)
    {
      DEBUG_INFO ("RSA sign...");

      ret = rsa_rsassa_pkcs1_v15_sign (&rsa_ctx, NULL, NULL,
				       RSA_PRIVATE, SIG_RSA_RAW,
				       msg_len, raw_message, temp);
      memcpy (output, temp, RSA_SIGNATURE_LENGTH);
    }

  rsa_free (&rsa_ctx);
  if (ret != 0)
    {
      DEBUG_INFO ("fail:");
      DEBUG_SHORT (ret);
      return -1;
    }
  else
    {
      res_APDU_size = RSA_SIGNATURE_LENGTH;
      DEBUG_INFO ("done.\r\n");
      GPG_SUCCESS ();
      return 0;
    }
}

/*
 * LEN: length in byte
 */
uint8_t *
modulus_calc (const uint8_t *p, int len)
{
  mpi P, Q, N;
  uint8_t *modulus;
  int ret;

  modulus = malloc (len);
  if (modulus == NULL)
    return NULL;

  mpi_init (&P);  mpi_init (&Q);  mpi_init (&N);
  MPI_CHK( mpi_read_binary (&P, p, len / 2) );
  MPI_CHK( mpi_read_binary (&Q, p + len / 2, len / 2) );
  MPI_CHK( mpi_mul_mpi (&N, &P, &Q) );
  MPI_CHK( mpi_write_binary (&N, modulus, len) );
 cleanup:
  mpi_free (&P);  mpi_free (&Q);  mpi_free (&N);
  if (ret != 0)
    return NULL;
  else
    return modulus;
}


int
rsa_decrypt (const uint8_t *input, uint8_t *output, int msg_len,
	     struct key_data *kd)
{
  mpi P1, Q1, H;
  int ret;
  unsigned int output_len;

  DEBUG_INFO ("RSA decrypt:");
  DEBUG_WORD ((uint32_t)&output_len);

  rsa_init (&rsa_ctx, RSA_PKCS_V15, 0);
  mpi_init (&P1);  mpi_init (&Q1);  mpi_init (&H);

  rsa_ctx.len = msg_len;
  DEBUG_WORD (msg_len);

  MPI_CHK( mpi_lset (&rsa_ctx.E, 0x10001) );
  MPI_CHK( mpi_read_binary (&rsa_ctx.P, &kd->data[0], KEY_CONTENT_LEN / 2) );
  MPI_CHK( mpi_read_binary (&rsa_ctx.Q, &kd->data[KEY_CONTENT_LEN/2],
			    KEY_CONTENT_LEN / 2) );
#if 0
  MPI_CHK( mpi_mul_mpi (&rsa_ctx.N, &rsa_ctx.P, &rsa_ctx.Q) );
#endif
  MPI_CHK( mpi_sub_int (&P1, &rsa_ctx.P, 1) );
  MPI_CHK( mpi_sub_int (&Q1, &rsa_ctx.Q, 1) );
  MPI_CHK( mpi_mul_mpi (&H, &P1, &Q1) );
  MPI_CHK( mpi_inv_mod (&rsa_ctx.D , &rsa_ctx.E, &H) );
  MPI_CHK( mpi_mod_mpi (&rsa_ctx.DP, &rsa_ctx.D, &P1) );
  MPI_CHK( mpi_mod_mpi (&rsa_ctx.DQ, &rsa_ctx.D, &Q1) );
  MPI_CHK( mpi_inv_mod (&rsa_ctx.QP, &rsa_ctx.Q, &rsa_ctx.P) );
 cleanup:
  mpi_free (&P1);  mpi_free (&Q1);  mpi_free (&H);
  if (ret == 0)
    {
      DEBUG_INFO ("RSA decrypt ...");
      ret = rsa_rsaes_pkcs1_v15_decrypt (&rsa_ctx, NULL, NULL,
					 RSA_PRIVATE, &output_len, input,
					 output, MAX_RES_APDU_DATA_SIZE);
    }

  rsa_free (&rsa_ctx);
  if (ret != 0)
    {
      DEBUG_INFO ("fail:");
      DEBUG_SHORT (ret);
      return -1;
    }
  else
    {
      res_APDU_size = output_len;
      DEBUG_INFO ("done.\r\n");
      GPG_SUCCESS ();
      return 0;
    }
}

int
rsa_verify (const uint8_t *pubkey, const uint8_t *hash, const uint8_t *sig)
{
  int ret;

  rsa_init (&rsa_ctx, RSA_PKCS_V15, 0);
  rsa_ctx.len = KEY_CONTENT_LEN;
  MPI_CHK( mpi_lset (&rsa_ctx.E, 0x10001) );
  MPI_CHK( mpi_read_binary (&rsa_ctx.N, pubkey, KEY_CONTENT_LEN) );

  DEBUG_INFO ("RSA verify...");

  MPI_CHK( rsa_rsassa_pkcs1_v15_verify (&rsa_ctx, NULL, NULL,
					RSA_PUBLIC, SIG_RSA_SHA256, 32,
					hash, sig) );
 cleanup:
  rsa_free (&rsa_ctx);
  if (ret != 0)
    {
      DEBUG_INFO ("fail:");
      DEBUG_SHORT (ret);
      return -1;
    }
  else
    {
      DEBUG_INFO ("verified.\r\n");
      return 0;
    }
}

#define RSA_EXPONENT 0x10001

#ifdef KEYGEN_SUPPORT
uint8_t *
rsa_genkey (void)
{
  int ret;
  uint8_t index = 0;
  uint8_t *p_q_modulus = (uint8_t *)malloc (KEY_CONTENT_LEN*2);
  uint8_t *p = p_q_modulus;
  uint8_t *q = p_q_modulus + KEY_CONTENT_LEN/2;
  uint8_t *modulus = p_q_modulus + KEY_CONTENT_LEN;
  extern int prng_seed (int (*f_rng)(void *, unsigned char *, size_t),
			void *p_rng);
  extern void neug_flush (void);

  if (p_q_modulus == NULL)
    return NULL;

  neug_flush ();
  prng_seed (random_gen, &index);

  rsa_init (&rsa_ctx, RSA_PKCS_V15, 0);
  MPI_CHK( rsa_gen_key (&rsa_ctx, random_gen, &index,
			KEY_CONTENT_LEN * 8, RSA_EXPONENT) );
  if (ret != 0)
    {
      free (p_q_modulus);
      rsa_free (&rsa_ctx);
      return NULL;
    }

  MPI_CHK( mpi_write_binary (&rsa_ctx.P, p, KEY_CONTENT_LEN/2) );
  MPI_CHK( mpi_write_binary (&rsa_ctx.Q, q, KEY_CONTENT_LEN/2) );
  MPI_CHK( mpi_write_binary (&rsa_ctx.N, modulus, KEY_CONTENT_LEN) );

 cleanup:
  rsa_free (&rsa_ctx);
  if (ret != 0)
      return NULL;
  else
    return p_q_modulus;
}
#endif
