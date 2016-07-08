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

#define ZERO(s) memset(&(s),0,sizeof(s));

#define LN "\r\n"

enum attrib_type{
	MAIL_FROM,
	RCPT_TO,
	SEND_FROM,
	SOML_FROM,
	SAML_FROM,
};
struct pre_attrib {
	enum attrib_type type;
	sds data;
};

#define MAX_PRE_ATTRIBS 128

struct pre_attrib pre_attribs[MAX_PRE_ATTRIBS];
int npre_attribs;

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
	for(i=0;i<npre_attribs;++i)
		sdsfree(pre_attribs[i].data);
}

void smtp_connection(){
	sds line = NULL;
	enum attrib_type preatttype;
	helo_host = NULL;
	npre_attribs = 0;
	ZERO(pre_attribs);
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
			helo_host = line;
			line = NULL;
			printf("250 Hello" LN);
		}else if(csds_match(line,"MAIL FROM:","mail from:")){
			preatttype = MAIL_FROM;
			goto onOk;
		}else if(csds_match(line,"RCPT TO:","rcpt to:")){
			preatttype = MAIL_FROM;
			goto onOk;
		}else if(csds_match(line,"SEND FROM:","send from:")){
			preatttype = MAIL_FROM;
			goto onOk;
		}else if(csds_match(line,"SOML FROM:","soml from:")){
			preatttype = MAIL_FROM;
			goto onOk;
		}else if(csds_match(line,"SAML FROM:","saml from:")){
			preatttype = MAIL_FROM;
			goto onOk;
		
		}else if(csds_match(line,"DATA","data")){
			printf("354 End data with <CR><LF>.<CR><LF>" LN);
			// TODO: implement.
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
		onOk:
		if(npre_attribs >= MAX_PRE_ATTRIBS) goto onError;
		sdstrim(line," \r\n\t");
		pre_attribs[npre_attribs] = (struct pre_attrib){preatttype,line};
		npre_attribs++;
		line = NULL;
		printf("250 Ok" LN);
		continue;
		onError:
		printf("500 Internal Server Error" LN);
	}
}


