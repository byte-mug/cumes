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
#include <input.h>
#include <sds.h>
#include <str_match.h>

#include "smtp.h"

#define ZERO(s) memset(&(s),0,sizeof(s));

#define LN "\r\n"

#define MAX_TOS 128

char pre_head_type;
sds  pre_head;

sds pre_to[MAX_TOS];
int npre_to;

char hostbuf[10000];
const char* hostname;
sds helo_host;

static void smtp_welcome(){
	printf("220 %s SMTP cumes" LN,hostname);
}
static sds smtp_readline() {
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
static void smtp_clear_preattribs(){
	int i;
	for(i=0;i<npre_to;++i)
		sdsfree(pre_to[npre_to]);
	npre_to = 0;
}

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

static void smtp_handle_data(){
	IOV v;
	sds line = sdsempty();
	int fd,i,not_broken;
	const char prefix[2] = {pre_head_type,0};
	if(!line) goto dataPreError;
	if((!pre_head) || (npre_to<1)){
		printf("503 Bad sequence of commands" LN);
		sdsfree(line);
		return;
	}
	fd = queue_open_sink();
	if(fd<0) goto dataPreError;
	line = sdscatfmt(line,"%s%S\n",prefix,pre_head);
	if(!line) goto dataPreError;
	if(writeAll(fd,line,sdslen(line))<0) goto dataPreError;
	for(i=0;i<npre_to;++i){
		sdssetlen(line,0); line[0]=0;
		line = sdscatfmt(line,"+%S\n",pre_to[i]);
		if(!line) goto dataPreError;
		if(writeAll(fd,line,sdslen(line))<0) goto dataPreError;
	}
	if(writeAll(fd,";\n",2)<0) goto dataPreError;
	sdsfree(line);
	line = NULL;

	/*
	 * Start reading input.
	 */
	printf("354 End data with <CR><LF>.<CR><LF>" LN);
	not_broken = 1;
	v.more = 1;
	while(v.more) {
		v = input_readdot();
		if(!v.ptr) continue;
		if(not_broken)not_broken = writeAll(fd,v.ptr,v.size)==0;
	}
	close(fd);
	
	/*
	 * Respond to the client.
	 */
	if(not_broken) {
		printf("250 Ok" LN);
	}else{
		/*
		 * We are allowed to respond with one of the following status codes:
		 * 552, 554, 451 or 452.
		 * 
		 * Hence we don't know, what exactly went wrong, we respond with
		 * "554 Transaction failed"
		 */
		printf("554 Transaction failed" LN);
	}
	
	return;
dataPreError:
	printf("451 Requested action aborted: error in processing" LN);
	sdsfree(line);
	return;
}


void smtp_connection(){
	sds line = NULL;
	helo_host = NULL;
	pre_head_type = 0;
	pre_head = NULL;
	npre_to = 0;
	ZERO(pre_to);
	if(gethostname(hostbuf,sizeof(hostbuf)-1)>=0) hostname = hostbuf;
	else hostname = "localhost";
	smtp_welcome();
	for(;;) {
		if(line)sdsfree(line);
		line = smtp_readline();
		if(!line){
			printf("500 Internal Server Error" LN);
		}else if(csds_match(line,"HELO ","helo ")){
			sdstrim(line," \r\n\t");
			if(helo_host) sdsfree(helo_host);
			helo_host = line;
			line = NULL;
			printf("250 Hello" LN);
		}else if(csds_match(line,"MAIL FROM:","mail from:")){
			pre_head_type = 'M';
			goto checkFrom;
		}else if(csds_match(line,"SEND FROM:","send from:")){
			pre_head_type = 'S';
			goto checkFrom;
		}else if(csds_match(line,"SOML FROM:","soml from:")){
			pre_head_type = 'O';
			goto checkFrom;
		}else if(csds_match(line,"SAML FROM:","saml from:")){
			pre_head_type = 'A';
			goto checkFrom;
		}else if(csds_match(line,"RCPT TO:","rcpt to:")){
			if(npre_to >= MAX_TOS) goto onError;
			sdstrim(line," \r\n\t");
			pre_to[npre_to] = line;
			npre_to++;
			line = NULL;
			printf("250 Ok" LN);
		}else if(csds_match(line,"DATA","data")){
			smtp_handle_data();
		}else if(csds_match(line,"TURN","turn")){
			printf("502 Command not implemented" LN);
		}else if(csds_match(line,"NOOP","noop")){
			printf("250 Ok" LN);
		}else if(csds_match(line,"RSET","rset")){
			smtp_clear_preattribs();
		}else if(csds_match(line,"QUIT","quit")){
			printf("221 Bye" LN);
			return;
		}else{
			printf("500 Command unrecognized" LN);
		}
		continue;
		checkFrom:
		sdstrim(line," \r\n\t");
		if(pre_head) sdsfree(pre_head);
		pre_head = line;
		line = NULL;
		printf("250 Ok" LN);
		continue;
		onError:
		printf("500 Internal Server Error" LN);
	}
}


