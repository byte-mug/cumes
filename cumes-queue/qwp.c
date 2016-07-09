/*
 * Copyright (c) 2016 Simon Schmidt
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Our Library. */
#include <sds.h>
#include <input.h>

#include "queue.h"

#define MAX_TO 128

static sds envel[MAX_TO];
static int envelc;

static sds qwp_readline() {
	IOV v;
	sds o;
	sds s = sdsempty();
	if(s==NULL)return NULL;
	for(;;){
		v = input_readline();
		if(v.ptr) s = sdscatlen(o = s,v.ptr,v.size);
		if(s==NULL){ sdsfree(o); return NULL; }
		if(v.more) continue;
		return s;
	}
	abort(); /* unreachable */
}
static int qwp_copydot(int dest) {
	IOV v;
	int n;
	for(;;){
		v = input_readdot();
		if(v.ptr) while(v.size){
			n = write(dest,v.ptr,v.size);
			if(n<1)return -1;
			v.size-=n;
			v.ptr+=n;
		}
		if(v.more) continue;
		return 0;
	}
	return 0;
}


int qwp_main() {
	sds line;
	int fd;
	envelc=0;
	
	queue_init();
	
	line = qwp_readline();
	if(!line)abort();
	switch(*line) {
	case 'M':
	case 'S':
	case 'O':
	case 'A':
		// type = *line;
		if(envelc>=MAX_TO)abort();
		envel[envelc++] = line;
		break;
	default:
		abort();
	}
	for(;;) {
		line = qwp_readline();
		if(!line) abort();
		switch(*line) {
		case '+':
			if(envelc>=MAX_TO)abort();
			envel[envelc++] = line;
			break;
		case ';':
			sdsfree(line);
			goto endLoop;
		default:
			abort();
		}
	}
	endLoop:
	fd = queue_add();
	if(fd<0)abort();
	qwp_copydot(fd);
	close(fd);
	queue_envelope(envel,envelc);
	return 0;
}

