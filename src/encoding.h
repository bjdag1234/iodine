/*
 * Copyright (c) 2006-2014 Erik Ekman <yarrick@kryo.se>,
 * 2006-2009 Bjorn Andersson <flex@kryo.se>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _ENCODING_H_
#define _ENCODING_H_

#include <stdint.h>

/* All-0, all-1, 01010101, 10101010: each 4 times to make sure the pattern
   spreads across multiple encoded chars -> 16 bytes total.
   Followed by 32 bytes from my /dev/random; should be enough.
 */
#define DOWNCODECCHECK1      "\000\000\000\000\377\377\377\377\125\125\125\125\252\252\252\252\201\143\310\322\307\174\262\027\137\117\316\311\111\055\122\041\141\251\161\040\045\263\006\163\346\330\104\060\171\120\127\277"
#define DOWNCODECCHECK1_LEN  48

/* Don't push the limit with DNS servers: potentially unwanted behaviour
 * if labels are all 63 chars long (DNS standard max label length) */
#define DNS_MAXLABEL 59

struct encoder {
	char name[8];
	size_t (*encode) (uint8_t *buf, size_t *buflen, const uint8_t *udata, size_t size);
	size_t (*decode) (uint8_t *buf, size_t *buflen, const uint8_t *udata, size_t size);
	int (*places_dots) (void);
	int (*eats_dots) (void);
	size_t (*blocksize_raw)(void);
	size_t (*blocksize_encoded)(void);
	size_t (*get_encoded_length)(size_t);
	size_t (*get_raw_length)(size_t);
};

size_t get_raw_length_from_dns(size_t enc_bytes, struct encoder *enc, const char *topdomain);
size_t get_encoded_dns_length(size_t raw_bytes, struct encoder *enc, const char *topdomain);

size_t build_hostname(uint8_t *, size_t, const uint8_t *, const size_t, const char *, struct encoder *, size_t, size_t);
size_t unpack_data(uint8_t *, size_t, uint8_t *, size_t, struct encoder *);
size_t inline_dotify(uint8_t *, size_t);
size_t inline_undotify(uint8_t *, size_t);
struct encoder *get_encoder(uint8_t codec);
size_t encode_data(uint8_t *, size_t, uint8_t *, size_t, uint8_t, int);



#endif /* _ENCODING_H_ */
