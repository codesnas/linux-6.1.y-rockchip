/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright (c) 2018 Rockchip Electronics Co., Ltd. */

#ifndef __RK_CRYPTO_V1_REG_H__
#define __RK_CRYPTO_V1_REG_H__

#define _SBF(v, f)			((v) << (f))

/* Crypto control registers*/
#define RK_CRYPTO_INTSTS		0x0000
#define RK_CRYPTO_PKA_DONE_INT		BIT(5)
#define RK_CRYPTO_HASH_DONE_INT		BIT(4)
#define RK_CRYPTO_HRDMA_ERR_INT		BIT(3)
#define RK_CRYPTO_HRDMA_DONE_INT	BIT(2)
#define RK_CRYPTO_BCDMA_ERR_INT		BIT(1)
#define RK_CRYPTO_BCDMA_DONE_INT	BIT(0)

#define RK_CRYPTO_INTENA		0x0004
#define RK_CRYPTO_PKA_DONE_ENA		BIT(5)
#define RK_CRYPTO_HASH_DONE_ENA		BIT(4)
#define RK_CRYPTO_HRDMA_ERR_ENA		BIT(3)
#define RK_CRYPTO_HRDMA_DONE_ENA	BIT(2)
#define RK_CRYPTO_BCDMA_ERR_ENA		BIT(1)
#define RK_CRYPTO_BCDMA_DONE_ENA	BIT(0)

#define RK_CRYPTO_CTRL			0x0008
#define RK_CRYPTO_WRITE_MASK		_SBF(0xFFFF, 16)
#define RK_CRYPTO_TRNG_FLUSH		BIT(9)
#define RK_CRYPTO_TRNG_START		BIT(8)
#define RK_CRYPTO_PKA_FLUSH		BIT(7)
#define RK_CRYPTO_HASH_FLUSH		BIT(6)
#define RK_CRYPTO_BLOCK_FLUSH		BIT(5)
#define RK_CRYPTO_PKA_START		BIT(4)
#define RK_CRYPTO_HASH_START		BIT(3)
#define RK_CRYPTO_BLOCK_START		BIT(2)
#define RK_CRYPTO_TDES_START		BIT(1)
#define RK_CRYPTO_AES_START		BIT(0)

#define RK_CRYPTO_CONF			0x000c
/* HASH Receive DMA Address Mode:   fix | increment */
#define RK_CRYPTO_HR_ADDR_MODE		BIT(8)
/* Block Transmit DMA Address Mode: fix | increment */
#define RK_CRYPTO_BT_ADDR_MODE		BIT(7)
/* Block Receive DMA Address Mode:  fix | increment */
#define RK_CRYPTO_BR_ADDR_MODE		BIT(6)
#define RK_CRYPTO_BYTESWAP_HRFIFO	BIT(5)
#define RK_CRYPTO_BYTESWAP_BTFIFO	BIT(4)
#define RK_CRYPTO_BYTESWAP_BRFIFO	BIT(3)
/* AES = 0 OR DES = 1 */
#define RK_CRYPTO_DESSEL				BIT(2)
#define RK_CYYPTO_HASHINSEL_INDEPENDENT_SOURCE		_SBF(0x00, 0)
#define RK_CYYPTO_HASHINSEL_BLOCK_CIPHER_INPUT		_SBF(0x01, 0)
#define RK_CYYPTO_HASHINSEL_BLOCK_CIPHER_OUTPUT		_SBF(0x02, 0)

/* Block Receiving DMA Start Address Register */
#define RK_CRYPTO_BRDMAS		0x0010
/* Block Transmitting DMA Start Address Register */
#define RK_CRYPTO_BTDMAS		0x0014
/* Block Receiving DMA Length Register */
#define RK_CRYPTO_BRDMAL		0x0018
/* Hash Receiving DMA Start Address Register */
#define RK_CRYPTO_HRDMAS		0x001c
/* Hash Receiving DMA Length Register */
#define RK_CRYPTO_HRDMAL		0x0020

/* AES registers */
#define RK_CRYPTO_AES_CTRL			  0x0080
#define RK_CRYPTO_AES_BYTESWAP_CNT	BIT(11)
#define RK_CRYPTO_AES_BYTESWAP_KEY	BIT(10)
#define RK_CRYPTO_AES_BYTESWAP_IV	BIT(9)
#define RK_CRYPTO_AES_BYTESWAP_DO	BIT(8)
#define RK_CRYPTO_AES_BYTESWAP_DI	BIT(7)
#define RK_CRYPTO_AES_KEY_CHANGE	BIT(6)
#define RK_CRYPTO_AES_ECB_MODE		_SBF(0x00, 4)
#define RK_CRYPTO_AES_CBC_MODE		_SBF(0x01, 4)
#define RK_CRYPTO_AES_CTR_MODE		_SBF(0x02, 4)
#define RK_CRYPTO_AES_128BIT_key	_SBF(0x00, 2)
#define RK_CRYPTO_AES_192BIT_key	_SBF(0x01, 2)
#define RK_CRYPTO_AES_256BIT_key	_SBF(0x02, 2)
/* Slave = 0 / fifo = 1 */
#define RK_CRYPTO_AES_FIFO_MODE		BIT(1)
/* Encryption = 0 , Decryption = 1 */
#define RK_CRYPTO_AES_DEC		BIT(0)

#define RK_CRYPTO_AES_STS		0x0084
#define RK_CRYPTO_AES_DONE		BIT(0)

/* AES Input Data 0-3 Register */
#define RK_CRYPTO_AES_DIN_0		0x0088
#define RK_CRYPTO_AES_DIN_1		0x008c
#define RK_CRYPTO_AES_DIN_2		0x0090
#define RK_CRYPTO_AES_DIN_3		0x0094

/* AES output Data 0-3 Register */
#define RK_CRYPTO_AES_DOUT_0		0x0098
#define RK_CRYPTO_AES_DOUT_1		0x009c
#define RK_CRYPTO_AES_DOUT_2		0x00a0
#define RK_CRYPTO_AES_DOUT_3		0x00a4

/* AES IV Data 0-3 Register */
#define RK_CRYPTO_AES_IV_0		0x00a8
#define RK_CRYPTO_AES_IV_1		0x00ac
#define RK_CRYPTO_AES_IV_2		0x00b0
#define RK_CRYPTO_AES_IV_3		0x00b4

/* AES Key Data 0-3 Register */
#define RK_CRYPTO_AES_KEY_0		0x00b8
#define RK_CRYPTO_AES_KEY_1		0x00bc
#define RK_CRYPTO_AES_KEY_2		0x00c0
#define RK_CRYPTO_AES_KEY_3		0x00c4
#define RK_CRYPTO_AES_KEY_4		0x00c8
#define RK_CRYPTO_AES_KEY_5		0x00cc
#define RK_CRYPTO_AES_KEY_6		0x00d0
#define RK_CRYPTO_AES_KEY_7		0x00d4

/* des/tdes */
#define RK_CRYPTO_TDES_CTRL		0x0100
#define RK_CRYPTO_TDES_BYTESWAP_KEY	BIT(8)
#define RK_CRYPTO_TDES_BYTESWAP_IV	BIT(7)
#define RK_CRYPTO_TDES_BYTESWAP_DO	BIT(6)
#define RK_CRYPTO_TDES_BYTESWAP_DI	BIT(5)
/* 0: ECB, 1: CBC */
#define RK_CRYPTO_TDES_CHAINMODE_CBC	BIT(4)
/* TDES Key Mode, 0 : EDE, 1 : EEE */
#define RK_CRYPTO_TDES_EEE		BIT(3)
/* 0: DES, 1:TDES */
#define RK_CRYPTO_TDES_SELECT		BIT(2)
/* 0: Slave, 1:Fifo */
#define RK_CRYPTO_TDES_FIFO_MODE	BIT(1)
/* Encryption = 0 , Decryption = 1 */
#define RK_CRYPTO_TDES_DEC		BIT(0)

#define RK_CRYPTO_TDES_STS		0x0104
#define RK_CRYPTO_TDES_DONE		BIT(0)

#define RK_CRYPTO_TDES_DIN_0		0x0108
#define RK_CRYPTO_TDES_DIN_1		0x010c
#define RK_CRYPTO_TDES_DOUT_0		0x0110
#define RK_CRYPTO_TDES_DOUT_1		0x0114
#define RK_CRYPTO_TDES_IV_0		0x0118
#define RK_CRYPTO_TDES_IV_1		0x011c
#define RK_CRYPTO_TDES_KEY1_0		0x0120
#define RK_CRYPTO_TDES_KEY1_1		0x0124
#define RK_CRYPTO_TDES_KEY2_0		0x0128
#define RK_CRYPTO_TDES_KEY2_1		0x012c
#define RK_CRYPTO_TDES_KEY3_0		0x0130
#define RK_CRYPTO_TDES_KEY3_1		0x0134

/* HASH */
#define RK_CRYPTO_HASH_CTRL		0x0180
#define RK_CRYPTO_HASH_SWAP_DO		BIT(3)
#define RK_CRYPTO_HASH_SWAP_DI		BIT(2)
#define RK_CRYPTO_HASH_SHA1		_SBF(0x00, 0)
#define RK_CRYPTO_HASH_MD5		_SBF(0x01, 0)
#define RK_CRYPTO_HASH_SHA256		_SBF(0x02, 0)
#define RK_CRYPTO_HASH_PRNG		_SBF(0x03, 0)

#define RK_CRYPTO_HASH_STS		0x0184
#define RK_CRYPTO_HASH_DONE		BIT(0)

#define RK_CRYPTO_HASH_MSG_LEN		0x0188
#define RK_CRYPTO_HASH_DOUT_0		0x018c
#define RK_CRYPTO_HASH_DOUT_1		0x0190
#define RK_CRYPTO_HASH_DOUT_2		0x0194
#define RK_CRYPTO_HASH_DOUT_3		0x0198
#define RK_CRYPTO_HASH_DOUT_4		0x019c
#define RK_CRYPTO_HASH_DOUT_5		0x01a0
#define RK_CRYPTO_HASH_DOUT_6		0x01a4
#define RK_CRYPTO_HASH_DOUT_7		0x01a8

#endif
