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
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <input.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

static char* buffer;
static size_t max;
static size_t curr;
static size_t rpos;
static int status;

#define MATCH(chr,st) (((chr)<<2)|(st))

enum {
	STATUS_0N   = MATCH('\n',0),
	STATUS_1N   = MATCH('\n',1),
	STATUS_1DOT = MATCH( '.',1),
	STATUS_2R   = MATCH('\r',2),
	STATUS_2N   = MATCH('\n',2),
};

/*
 * The suicide loop loops until the process died.
 */
#define SUICIDE_LOOP do{ abort(); raise(SIGKILL); _exit(1); }while(1)

void input_init(){
	size_t psize = getpagesize();
	size_t bsize = 0;
	if(psize<1) SUICIDE_LOOP;

	while( bsize < 64000 ) bsize += psize;

	buffer = mmap(NULL,bsize,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
	if(buffer==MAP_FAILED) SUICIDE_LOOP;
	max = bsize;
	curr = 0;
	rpos = 0;
	status = 0;
}

static int input_fill(){
	ssize_t res;
	size_t goal = curr-rpos;
	if(!curr){
		res = read(0,buffer,max);
		if(res<1) return 1;
		curr = res;
	}else{
		if(goal>0) memmove(buffer,buffer+rpos,goal);
		curr-= rpos;
		rpos = 0;
		if(goal<max){
			/*
			 * Try to read-in data using the recv() syscall. This is necessary in
			 * order to perform a non-blocking read. If stdin is not a socket, recv
			 * will fail with a ENOTSOCK error.
			 */
			res = recv(0,buffer+curr,max-curr,MSG_DONTWAIT);
			if(res>0) curr+=res;
		}
	}
	return 0;
}

IOV  input_readline(){
	size_t i;
	if(input_fill()) return (IOV){NULL,0,0};
	for(i=0;i<curr;++i){
		if(buffer[i]=='\n'){
			status = 0;
			++i;
			rpos = i;
			return (IOV){buffer,i,0};
		}
	}
	rpos = curr;
	return (IOV){buffer,curr,1};
}
IOV  input_readdot(){
	size_t i;
	if(input_fill()) return (IOV){NULL,0,0};
	/* matching \n.\n or \n.\r\n */
	for(i=0;i<curr;++i){
		switch(MATCH(buffer[i],status)){
		case STATUS_0N:
		case STATUS_1N: status = 1; break; /* '\n' */
		case STATUS_1DOT: status = 2; break; /* '.' */
		case STATUS_2R: status = 2; break; /* '\r' */
		case STATUS_2N: /* '\n' */
			status = 0; /* reset status */
			++i;
			rpos = i;
			return (IOV){buffer,i,0};
		default:
			status = 0;
		}
	}
	rpos = curr;
	return (IOV){buffer,curr,1};
}

