/*
 * Copyright (c) 2006-2015, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2015, Oran Agra
 * Copyright (c) 2015, Redis Labs, Inc
 * Copyright (c) 2016 Simon Schmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sdsec.h>

static inline size_t _lenll(long long num) {
	size_t addlen=0;
	if(num<0){ addlen++; num = -num; }
	else if(num==0) return 1;
	while(num){
		num /= 10;
		addlen++;
	}
	return addlen;
}

static inline size_t _lenull( unsigned long long num) {
	size_t addlen=0;
	if(num==0)return 1;
	while(num){
		num /= 10;
		addlen++;
	}
	return addlen;
}

static inline size_t _addll(char* ptr,long long num) {
	size_t i=0,j=0,k=0;
	char temp;
	if(num<0){ *(ptr++)='-'; k++; num = -num; }
	else if(num==0) { *ptr = '0'; return 1; }
	while(num){
		ptr[i] = '0'+(num%10);
		num /= 10;
		i++;
	}
	k+=i;
	while(j<i){
		i--;
		temp = ptr[i];
		ptr[i] = ptr[j];
		ptr[j] = temp;
		j++;
	}
	return k;
}
static inline size_t _addull(char* ptr,unsigned long long num) {
	size_t i=0,j=0,k;
	char temp;
	if(num==0) { *ptr = '0'; return 1; }
	while(num){
		ptr[i++] = '0'+(num%10);
		num /= 10; 
	}
	k=i;
	while(j<i){
		i--;
		temp = ptr[i];
		ptr[i] = ptr[j];
		ptr[j] = temp;
		j++;
	}
	return k;
}


/* This function is similar to sdscatprintf, but much faster as it does
 * not rely on sprintf() family functions implemented by the libc that
 * are often very slow. Moreover directly handling the sds string as
 * new data is concatenated provides a performance improvement.
 *
 * However this function only handles an incompatible subset of printf-alike
 * format specifiers:
 *
 * %s - C String
 * %S - SDS string
 * %i - signed int
 * %I - 64 bit signed integer (long long, int64_t)
 * %u - unsigned int
 * %U - 64 bit unsigned integer (unsigned long long, uint64_t)
 * %% - Verbatim "%" character.
 */
sds csds_catfmt(sds s, char const *fmt, ...) {
    size_t initlen,addlen,i,l;
    const char *f;
    va_list ap;
    va_list ap2;
    char next, *str;
    long long num;
    unsigned long long unum;
    
    addlen = 0;
    initlen = sdslen(s);
    f = fmt;

    va_start(ap,fmt);
    va_copy(ap2,ap);
    f = fmt;    /* Next format specifier byte to process. */
    i = initlen; /* Position of the next byte to write to dest str. */
    while(*f) {
        switch(*f) {
        case '%':
            next = *(f+1);
            f++;
            switch(next) {
            case 's':
            case 'S':
                str = va_arg(ap,char*);
                l = (next == 's') ? strlen(str) : sdslen(str);
                addlen += l;
                break;
            case 'i':
            case 'I':
                if (next == 'i')
                    num = va_arg(ap,int);
                else
                    num = va_arg(ap,long long);
                addlen += _lenll(num);
                break;
            case 'u':
            case 'U':
                if (next == 'u')
                    unum = va_arg(ap,unsigned int);
                else
                    unum = va_arg(ap,unsigned long long);
                addlen += _lenull(unum);
                break;
            default: /* Handle %% and generally %<unknown>. */
                addlen++;
                break;
            }
            break;
        default:
            addlen++;
            break;
        }
        f++;
    }
    va_end(ap);
    f = fmt;
    s = sdsMakeRoomFor(s,addlen);
    sdsinclen(s,addlen);
    while(*f) {
        switch(*f) {
        case '%':
            next = *(f+1);
            f++;
            switch(next) {
            case 's':
            case 'S':
                str = va_arg(ap2,char*);
                l = (next == 's') ? strlen(str) : sdslen(str);
                memcpy(s+i,str,l);
                i += l;
                break;
            case 'i':
            case 'I':
                if (next == 'i')
                    num = va_arg(ap2,int);
                else
                    num = va_arg(ap2,long long);
                i += _addll(s+i,num);
                break;
            case 'u':
            case 'U':
                if (next == 'u')
                    unum = va_arg(ap2,unsigned int);
                else
                    unum = va_arg(ap2,unsigned long long);
                i += _addull(s+i,unum);
                break;
            default: /* Handle %% and generally %<unknown>. */
                s[i++] = next;
                break;
            }
            break;
        default:
            s[i++] = *f;
            break;
        }
        f++;
    }
    va_end(ap2);
    /* Add null-term */
    s[i] = '\0';
    return s;
}

