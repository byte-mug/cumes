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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>

#include <sds.h>
#include <str_hol.h>

#define failfd(x,expr) if((x)<0){ perror(expr); abort(); }

#define failon(x) if(!(x))abort()

typedef FILE* FOBJ;

static const char* queue;
static sds mess,todo,info,local,remote,modmess,intd;

static char* buffer;
static size_t buflen;

static FOBJ
	F_todo,
	F_info,
	F_local,
	F_remote;

static void init_buffer(){
	size_t psize = getpagesize();
	size_t bsize = 0;
	if(psize<1) abort();

	while( bsize < 1024 ) bsize += psize;

	buffer = mmap(NULL,bsize,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
	if(buffer==MAP_FAILED) abort();
	buflen = bsize;
}

void queue_init() {
	size_t len;
	sds queue2;
	queue = getenv("QUEUE"); failon(queue);
	len = strlen(queue); failon(len);
	if( queue[len-1]!='/' ) {
		queue2 = sdsnew(queue); failon(queue2);
		queue2 = sdscat(queue2,"/"); failon(queue2);
		queue = queue2;
	}
	init_buffer();
	F_local = NULL;
	F_remote = NULL;
}

static void queue_process_init(const char* msgnam){
	// saves us several strlen calls
	sds MID = sdsnew(msgnam); failon(MID);
	mess    = sdsnew(queue); failon(mess);
	mess    = sdscatfmt(mess,"mess/%S",MID); failon(mess);
	todo    = sdsnew(queue); failon(todo);
	todo    = sdscatfmt(todo,"todo/%S",MID); failon(todo);
	info    = sdsnew(queue); failon(info);
	info    = sdscatfmt(todo,"info/%S",MID); failon(info);
	local   = sdsnew(queue); failon(local);
	local   = sdscatfmt(todo,"local/%S",MID); failon(local);
	remote  = sdsnew(queue); failon(remote);
	remote  = sdscatfmt(remote,"remote/%S",MID); failon(remote);
	modmess = sdsnew(queue); failon(modmess);
	modmess = sdscatfmt(remote,"modmess/%S",MID); failon(modmess);
	intd    = sdsnew(queue); failon(intd);
	intd    = sdscatfmt(remote,"intd/%S",MID); failon(intd);
	sdsfree(MID);
}

static int queue_proc_recipient_is_local(sds rec){
	return 0;
	// TODO: implement
}

static void queue_proc_recipient_add(int is_local,const char* rec){
	FOBJ f;
	if(is_local){
		if(!F_local) F_local = fopen(local,"w");
		f = F_local;
	}else{
		if(!F_remote) F_remote = fopen(remote,"w");
		f = F_remote;
	}
	if(!f)return;
	fprintf(f,"DONE:Y %s\n",rec);
}

#define breakon(x) if(!(x)) return 0;
#define FUBREAK { sdsfree(linebuf); return 0; }

static int queue_proc_todo(){
	sds linebuf;
	const char* line;
	linebuf = sdsempty(); breakon(linebuf);
	line = fgets(buffer,buflen,F_todo);
	/* No input, no action! */
	if(!line) FUBREAK
	fputs(line,F_info);
	fclose(F_info);
	for(;;) {
		line = fgets(buffer,buflen,F_todo);
		if(!line) break;
		if((*line)!='+') continue;
		linebuf = sdscat(linebuf,line+1); breakon(linebuf);
		sdstrim(linebuf," \r\n\t");
		queue_proc_recipient_add(queue_proc_recipient_is_local(linebuf),linebuf);
		sdssetlen(linebuf,0);
	}
	sdsfree(linebuf);
	return 1;
}
#undef FUBREAK
#undef breakon

void queue_process(const char* msgnam){
	int Locker;
	queue_process_init(msgnam);
	Locker = open(mess,O_RDONLY|O_CLOEXEC,0); failfd(Locker,"open(mess)");
	if(flock(Locker,LOCK_EX)<0) abort();
	unlink(info);
	unlink(local);
	unlink(remote);
	unlink(modmess);
	F_todo = fopen(todo,"r"); failon(F_todo);
	F_info = fopen(info,"w"); failon(F_info);
	if(!queue_proc_todo()) abort();
	unlink(intd);
	unlink(todo);
}


