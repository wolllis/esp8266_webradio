#ifndef USER_SHA256_H_
#define USER_SHA256_H_

/* MIT License
 *
 * Copyright (c) 2019, wolllis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

typedef struct
{
        unsigned char data[64];
        unsigned int datalen;
        unsigned int bitlen[2];
        unsigned int state[8];
} SHA256_CTX;

void ICACHE_FLASH_ATTR sha256_init(SHA256_CTX *ctx);
void ICACHE_FLASH_ATTR sha256_update(SHA256_CTX *ctx, unsigned char data[], unsigned int len);
void ICACHE_FLASH_ATTR sha256_final(SHA256_CTX *ctx, unsigned char hash[]);

#endif /* USER_SHA256_H_ */
