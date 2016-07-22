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
#include <string.h>


/* Our Library. */
#include <sdsec.h>
#include <str_hol.h>

#define failon(x) if(!(x))abort();

#ifdef DEBUG
#define should(x) if(!(x))abort();
#else
#define should(x)
#endif

#define MAX_TOS 128

static char lineBuffer[10000];

static sds path,info,local,mess,modmess;

static sds clientHelo,fromLine;

int lmtp_connect();

int fdLocal,fdInfo,fdMsg;

static struct dest{
	sds string;
	size_t off;
} dests[MAX_TOS];
static int ndests;

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

static void read_from(){
	sds from;
	ssize_t n;
	size_t i=0;
	n = read(fdInfo,lineBuffer,sizeof(lineBuffer));
	for(;;) {
		if(i>=n)break;
		if(lineBuffer[i]=='\n'){ i++; break; }
		i++;
	}
	failon(i>1);
	from = sdsnewlen(lineBuffer+1,i-1);failon(from);
	switch(*lineBuffer){
	case 'M':fromLine = sdsnew("MAIL FROM:");break;
	case 'S':fromLine = sdsnew("SEND FROM:");break;
	case 'O':fromLine = sdsnew("SOML FROM:");break;
	case 'A':fromLine = sdsnew("SAML FROM:");break;
	
	/*
	 * When in doubt, just mail.
	 */
	default: fromLine = sdsnew("MAIL FROM:");break;
	}
	failon(fromLine);
	sdstrim(from," \r\n\t");
	fromLine = sdscatsds(fromLine,from); failon(fromLine);
	fromLine = sdscat(fromLine,"\r\n"); failon(fromLine);
}

static void read_tos(){
	size_t pos=0,used=0,i;
	ssize_t n;
	sds current;
	ndests = 0;
	for(;;){
		if(used<sizeof(lineBuffer)){
			n = read(fdLocal,lineBuffer+used,sizeof(lineBuffer)-used);
			if(n>0)used+=n;
		}
		i = 0;
		for(;;) {
			if(i>=used)break;
			if(lineBuffer[i]=='\n'){ i++; break; }
			i++;
		}
		current = sdsnewlen(lineBuffer,i);
		if(current) {
			if((ndests<MAX_TOS) && !memcmp("DONE:N",current,6)){
				csds_pullfront(current,7);
				dests[ndests].string = current;
				dests[ndests].off = pos+5;
			}
		}
		if(used>i) memmove(lineBuffer,lineBuffer+i,used-i);
		used -= i;
		pos += i;
	}
}

void lmtp_init(){
	const char* queue = getenv("QUEUE"); failon(queue)
	const char* host = getenv("HOST");
	path = sdsnew(queue); failon(path);
	if(!csds_ends_with(path,'/')) {
		path = sdscat(path,"/");
		failon(path);
	}
	info = sdsdup(path); failon(info);
	info = sdscat(info,"info2/"); failon(info);
	local = sdsdup(path); failon(local);
	local = sdscat(local,"local/"); failon(local);
	mess = sdsdup(path); failon(mess);
	mess = sdscat(mess,"mess/"); failon(mess);
	modmess = sdsdup(path); failon(modmess);
	modmess = sdscat(modmess,"modmess/"); failon(modmess);
	
	clientHelo = sdsnew("LHLO "); failon(clientHelo);
	if(host)clientHelo = sdscat(clientHelo,host);
	else clientHelo = sdscat(clientHelo,"localhost");
	failon(clientHelo);
	clientHelo = sdscat(clientHelo,"\r\n"); failon(clientHelo);
}

#define CONSUME_250 \
	for(;;){\
		line = fgets(lineBuffer,sizeof(lineBuffer)-1,in);\
		if(memcmp(line,"250",3)) goto conn_error;\
		if(line[3]=='-')continue;\
		else break;\
	}
#define CONSUME \
	for(;;){\
		line = fgets(lineBuffer,sizeof(lineBuffer)-1,in);\
		if(line[3]=='-')continue;\
		else break;\
	}
#define CONSUME_354 \
	for(;;){\
		line = fgets(lineBuffer,sizeof(lineBuffer)-1,in);\
		if(memcmp(line,"354",3)) goto conn_error;\
		if(line[3]=='-')continue;\
		else break;\
	}

static void transmit(){
	sds rcptTo;
	size_t rcptTol;
	ssize_t n;
	int i,j,fd;
	const char * line;
	FILE * in;
	rcptTo = sdsnew("RCPT TO:"); failon(rcptTo);
	rcptTol = sdslen(rcptTo);
	fd = lmtp_connect();
	if(fd<0){ perror("connect"); abort(); }
	in = fdopen(fd,"r"); failon(in);
	
	/* Read Input line */
	fgets(lineBuffer,sizeof(lineBuffer)-1,in);
	/* Assume the incoming line to be correct, for now... */
	
	failon(writeAll(fd,clientHelo,sdslen(clientHelo))>=0);
	CONSUME_250
	
	failon(writeAll(fd,fromLine,sdslen(fromLine))>=0);
	CONSUME_250
	
	for(j=0;j<ndests;++j){
		sdssetlen(rcptTo,rcptTol);
		rcptTo = sdscatsds(rcptTo,dests[j].string); failon(rcptTo);
		rcptTo = sdscat(rcptTo,"\r\n"); failon(rcptTo);
		failon(writeAll(fd,fromLine,sdslen(fromLine))>=0);
		CONSUME_250
	}
	
	failon(writeAll(fd,"DATA\r\n",6)>=0);
	CONSUME_354
	
	for(;;){
		n = read(fdMsg,lineBuffer,sizeof(lineBuffer));
		if(n<1)break;
		write(fd,lineBuffer,n);
	}
	
	for(j=0;j<ndests;++j){
		CONSUME
		if(memcmp("250",line,3)){
			lseek(fdLocal,dests[j].off,SEEK_SET);
			write(fdLocal,"Y",1);
		}
	}
	
conn_error:
	writeAll(fd,"QUIT\r\n",6);
	read(fd,lineBuffer,sizeof(lineBuffer));
	fclose(in);
	close(fd);
}

void lmtp_process(const char* msgnam){
	info = sdscat(info,msgnam); failon(info);
	local = sdscat(local,msgnam); failon(local);
	mess = sdscat(mess,msgnam); failon(mess);
	modmess = sdscat(mess,msgnam); failon(modmess);
	
	fdLocal = open(local,O_RDWR,0); if(fdLocal<0)return;
	fdInfo = open(info,O_RDONLY,0); failon(fdInfo>=0);
	fdMsg = open(modmess,O_RDONLY,0);
		if(fdMsg<0) fdMsg = open(mess,O_RDONLY,0);
		failon(fdMsg>=0);
	read_from();
	read_tos();
	
}

