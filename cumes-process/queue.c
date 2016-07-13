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
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>

/* Our Library. */
#include <sds.h>
#include <str_hol.h>
#include <str_match.h>

#define failfd(x,expr) if((x)<0){ perror(expr); abort(); }

#define failon(x) if(!(x))abort()

typedef FILE* FOBJ;

#define MAX_DOMS 128

static sds arrdoms[MAX_DOMS];
static int narrdoms;
static const char* mailproc;
static const char* queue;
static sds mess,todo,info,info2,local,remote,modmess,intd;

static char* buffer;
static size_t buflen;

static FOBJ
	F_todo,
	F_info,
	F_local,
	F_remote;

static void ignore(int i){}

static void init_buffer(){
	size_t psize = getpagesize();
	size_t bsize = 0;
	if(psize<1) abort();

	while( bsize < 1024 ) bsize += psize;

	buffer = mmap(NULL,bsize,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
	if(buffer==MAP_FAILED) abort();
	buflen = bsize;
}

static void init_doms(){
	sds temp;
	narrdoms = 0;
	const char* domains = getenv("DOMAINS");
	if(!domains)return;
	int i;
	while(narrdoms<MAX_DOMS){
		i = cstr_indexof(domains,',');
		if(i<0)break;
		if(i>0){
			temp = sdsnewlen(domains,i); failon(temp);
			sdstolower(temp);
			arrdoms[narrdoms++] = temp;
		}
		domains+=i+1;
	}
	if(*domains&&narrdoms<MAX_DOMS){
		temp = sdsnew(domains); failon(temp);
		sdstolower(temp);
		arrdoms[narrdoms++] = temp;
	}
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
	init_doms();
	mailproc = getenv("MAILPROC");
}

static void queue_process_init(const char* msgnam){
	// saves us several strlen calls
	sds MID = sdsnew(msgnam); failon(MID);
	mess    = sdsnew(queue); failon(mess);
	mess    = sdscatfmt(mess,"mess/%S",MID); failon(mess);
	todo    = sdsnew(queue); failon(todo);
	todo    = sdscatfmt(todo,"todo/%S",MID); failon(todo);
	info    = sdsnew(queue); failon(info);
	info    = sdscatfmt(info,"info/%S",MID); failon(info);
	info2   = sdsnew(queue); failon(info2);
	info2   = sdscatfmt(info2,"info2/%S",MID); failon(info2);
	local   = sdsnew(queue); failon(local);
	local   = sdscatfmt(local,"local/%S",MID); failon(local);
	remote  = sdsnew(queue); failon(remote);
	remote  = sdscatfmt(remote,"remote/%S",MID); failon(remote);
	modmess = sdsnew(queue); failon(modmess);
	modmess = sdscatfmt(modmess,"modmess/%S",MID); failon(modmess);
	intd    = sdsnew(queue); failon(intd);
	intd    = sdscatfmt(intd,"intd/%S",MID); failon(intd);
	sdsfree(MID);
}

static int queue_proc_recipient_is_local(sds rec){
	int i;
	sds r2 = sdsdup(rec); failon(r2);

	/*
	 * Extract the Domain part of the mail address.
	 */
	csds_pattm(r2,"%u@",'(');
	csds_pattm(r2,"%u>",')');
	sdstolower(r2);
	for(i=0;i<narrdoms;++i) {
		if(!sdscmp(r2,arrdoms[i])) { /* the domain matches */
			sdsfree(r2);
			return 1;
		}
	}
	sdsfree(r2);
	return 0;
}

static void queue_proc_recipient_add(sds rec){
	FOBJ f;
	if(queue_proc_recipient_is_local(rec)){
		if(!F_local) F_local = fopen(local,"w");
		f = F_local;
	}else{
		if(!F_remote) F_remote = fopen(remote,"w");
		f = F_remote;
	}
	if(!f)return;
	fprintf(f,"DONE:N %s\n",rec);
}
static void queue_pr_cleanup() {
	if(F_local)fclose(F_local);
	if(F_remote)fclose(F_remote);
}

#define breakon(x) if(!(x)) { queue_pr_cleanup(); return 0; }
#define FRET(x) { queue_pr_cleanup(); sdsfree(linebuf); return (x); }
#define FUBREAK FRET(0)


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
		queue_proc_recipient_add(linebuf);
		sdssetlen(linebuf,0);
	}
	FRET(1)
}
#undef FUBREAK
#undef FRET
#undef breakon

static void queue_mailproc(){
	int fdin,fdout,status;
	pid_t pid;
	struct stat statbuf;

	if(!mailproc)return;

	fdin = open(mess,O_RDONLY,0); failfd(fdin,"open(mess)");

	failfd(   fstat(fdin,&statbuf)     ,"fstat(mess)");

	if(statbuf.st_size > (1<<19)) { close(fdin); return; }

	fdout = open(modmess,O_WRONLY|O_CREAT|O_TRUNC,statbuf.st_mode); failfd(fdout,"open(modmess)");
	pid = fork();
	if(pid<0) abort();
	if(pid==0){
		dup2(fdin,0);
		dup2(fdout,1);
		close(fdin);
		close(fdout);
		execlp(mailproc,mailproc,NULL);
		abort();
	}
	close(fdin);
	close(fdout);
	if(waitpid(pid,&status,0)<1) abort();
	if(status) abort();
}

void queue_process(const char* msgnam){
	struct utimbuf buf;
	int Locker;
	queue_process_init(msgnam);
	Locker = open(mess,O_RDONLY|O_CLOEXEC,0); failfd(Locker,"open(mess)");
	if(flock(Locker,LOCK_EX)<0) abort();
	F_todo = fopen(todo,"r"); failon(F_todo);
	unlink(info);
	unlink(local);
	unlink(remote);
	unlink(modmess);
	F_info = fopen(info,"w"); failon(F_info);
	if(!queue_proc_todo()) abort();
	fclose(F_todo);
	queue_mailproc();
	unlink(intd);
	unlink(todo);
	buf.actime = buf.modtime = time(NULL);
	utime(info,&buf);
	ignore(link(info,info2));
}


