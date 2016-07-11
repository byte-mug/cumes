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
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "smtp.h"

static void ignore_signal(int i){}

static struct sockaddr* build_address(const char* name,socklen_t *al){
	static struct sockaddr_un stun;
	size_t siz = strlen(name)+1;
	size_t asiz = sizeof(stun)-sizeof(stun.sun_path)+siz;
	struct sockaddr_un *un;
	if(asiz>sizeof(stun)) {
		*al = asiz;
		un = malloc(asiz);
		if(!un)return NULL;
	} else {
		*al = sizeof(stun);
		un = &stun;
	}
	un->sun_family = AF_UNIX;
	memcpy(un->sun_path,name,siz);
	return (struct sockaddr*)un;
}

int queue_open_sink(){
	/* The serving process should not die at a SIGPIPE-signal. */
	signal(SIGPIPE,ignore_signal);
	int epark;
	socklen_t addrl;
	const char* env = getenv("CUMES_QUEUE");
	if(!env) { errno = ENOENT; return -1; }
	struct sockaddr* addr = build_address(env,&addrl);
	if(!addr)return -1;
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(fd<0) return -1;
	if(connect(fd,addr,addrl)<0) { epark = errno; close(fd); errno = epark; return -1; }
	return fd;	
}


