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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "common.h"
#include "encoding.h"
#include "base32.h"
#include "base64.h"
#include "base64u.h"
#include "base128.h"

size_t
get_raw_length_from_dns(size_t dns_hostlen, struct encoder *enc, const uint8_t *topdomain)
/* Returns the maximum length of raw data that can be encoded into enc_bytes */
{
	/* 1 byte for dot before topdomain */
	size_t enc_datalen = dns_hostlen - 1 - HOSTLEN(topdomain);

	/* Number of dots in length of encoded data -
	   Dots are not included in encoded data length */
	enc_datalen -= DNS_NUM_LABELS(enc_datalen) + 1;

	if (enc)
		return enc->get_raw_length(enc_datalen);
	else
		return enc_datalen;
}

size_t
get_encoded_dns_length(size_t raw_bytes, struct encoder *enc, const uint8_t *topdomain)
/* Returns length of encoded data from original data length orig_len; */
{
	size_t dots = 1; /* dot before topdomain */
	size_t len;
	if (enc)
		len = enc->get_encoded_length(raw_bytes);
	else
		len = raw_bytes;

	dots += len / DNS_MAXLABEL; /* number of dots needed in data */
	return len + dots + HOSTLEN(topdomain);
}

size_t
build_hostname(uint8_t *buf, size_t buflen, const uint8_t *data, const size_t datalen,
		const char *topdomain, struct encoder *encoder, size_t maxlen, size_t header_len)
/* Builds DNS-compatible hostname for data using specified encoder and topdomain
 * Encoded data is placed into buf. */
{
	size_t space, enc;
	uint8_t *b;

	buflen -= header_len;
	buf += header_len;
	maxlen -= header_len;
	memset(buf, 0, buflen);

	maxlen = MIN(maxlen, buflen);

	/* 1 byte for dot before topdomain + 1 byte extra for something */
	space = maxlen - strlen(topdomain) - (maxlen / DNS_MAXLABEL) - 2;

	enc = encoder->encode(buf, &space, data, datalen);
//	warnx("build_hostname: enc %lu, predicted %lu; maxlen %lu, header %lu, datalen %lu, space %lu",
//		  encdata_len, encoder->get_encoded_length(datalen), maxlen, header_len, datalen, space);

	enc = inline_dotify(buf - header_len, buflen + header_len) - header_len;

	b = buf + enc;

	/* move b back one step to see if the dot is there */
	b--;
	if (*b != '.')
		*++b = '.';
	b++;
	/* move b ahead of the string so we can copy to it */

	strncpy((char *)b, topdomain, strlen(topdomain)+1);
//	warnx("build_hostname: host '%s' (sl %lu, actual %lu), topdomain '%s'",
//			buf - header_len, strlen(buf - header_len), encdata_len + header_len + strlen(topdomain)+1, b);

	return space;
}

size_t
inline_dotify(uint8_t *buf, size_t buflen)
{
	unsigned dots;
	size_t pos, total;
	uint8_t *reader, *writer;

	total = strlen((char *)buf);
	dots = total / DNS_MAXLABEL;

	writer = buf;
	writer += total;
	writer += dots;

	total += dots;
	if (strlen((char *)buf) + dots > buflen) {
		writer = buf;
		writer += buflen;
		total = buflen;
	}

	reader = writer - dots;
	pos = (reader - buf) + 1;

	while (dots) {
		*writer-- = *reader--;
		pos--;
		if (pos % DNS_MAXLABEL == 0) {
			*writer-- = '.';
			dots--;
		}
	}

	/* return new length of string */
	return total;
}

size_t
inline_undotify(uint8_t *buf, size_t len)
{
	size_t pos;
	unsigned dots;
	uint8_t *reader, *writer;

	writer = buf;
	reader = writer;

	pos = 0;
	dots = 0;

	while (pos < len) {
		if (*reader == '.') {
			reader++;
			pos++;
			dots++;
			continue;
		}
		*writer++ = *reader++;
		pos++;
	}

	/* return new length of string */
	return len - dots;
}

struct encoder *
get_encoder(uint8_t codec)
{
	switch (codec & 0x7) {
	case C_BASE32:
		return b32;
	case C_BASE64:
		return b64;
	case C_BASE64U:
		return b64u;
	case C_BASE128:
		return b128;
	case C_RAW:
	default:
		return NULL;
	}
}

size_t
encode_data(uint8_t *buf, size_t buflen, uint8_t *data, size_t datalen, uint8_t codec)
/* Returns #bytes of data that were encoded */
{
	struct encoder *enc;

	enc = get_encoder(codec);
	if (enc == NULL) {
		memcpy(buf, data, MIN(buflen, datalen));
		return MIN(buflen, datalen);
	}

	buflen--; /* encoders add trailing zero byte which is not counted in buflen */
	return enc->encode(buf, &buflen, data, datalen);
}

size_t
unpack_data(uint8_t *buf, size_t buflen, uint8_t *data, size_t datalen, uint8_t codec)
{
	struct encoder *enc;

	enc = get_encoder(codec);
	if (enc == NULL) {
		memcpy(buf, data, MIN(buflen, datalen));
		return MIN(buflen, datalen);
	}

	return enc->decode(buf, &buflen, data, datalen);
}

