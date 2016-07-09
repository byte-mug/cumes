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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/* Our Library. */
#include <sds.h>
#include <str_hol.h>

static int MSGFD;
static long long MSGID;
static sds path;
static sds path2;
static size_t pathlen;

#define failon(x) if(!(x))abort();

#ifdef DEBUG
#define should(x) if(!(x))abort();
#else
#define should(x)
#endif

static int writeAll(int fd,const char* data,size_t len){
	int n;
	while(len){
		n = write(fd,data,len);
		if(n<1)return -1;
		data+=n;
		len-=n;
	}
	return 0;
}

void queue_init() {
	char* e = getenv("QUEUE");
	if(!e)abort();
	path = sdsnew(e);
	failon(path);
	path = sdsMakeRoomFor(path,100);
	failon(path);
	if(!csds_ends_with(path,'/')) {
		path = sdscat(path,"/");
		failon(path);
	}
	pathlen = sdslen(path);
	path2 = sdsdup(path);
	failon(path2);
	path2 = sdscatfmt(path2,"pid/%I",(long long)getpid());
	failon(path2);
}

int queue_add() {
	struct stat buf;
	MSGFD = open(path2,O_CREAT|O_WRONLY,0600);
	if(MSGFD<0) return -1;
	if(stat(path2,&buf)<0) return -1;
	MSGID = buf.st_ino;
	path = sdscatfmt(path,"mess/%I",MSGID); should(path);
	if(rename(path2,path)<0) return -1;
	sdssetlen(path,pathlen);
	return MSGFD;
}

void queue_envelope(sds envel[],int envelc) {
	int i;
	path = sdscatfmt(path,"intd/%I",MSGID); should(path);
	sds resc = sdsdup(path); failon(resc);
	sdssetlen(path,pathlen);
	path = sdscatfmt(path,"todo/%I",MSGID); should(path);
	MSGFD = open(resc,O_CREAT|O_WRONLY,0600);
	if(MSGFD<0) abort();
	for(i=0;i<envelc;++i){
		sdstrim(envel[i],"\r\n\t ");
		if(writeAll(MSGFD,envel[i],sdslen(envel[i]))<0) abort();
		if(write(MSGFD,"\n",1)<1) abort();
	}
	close(MSGFD);
	link(resc,path);
	sdsfree(resc);
}

