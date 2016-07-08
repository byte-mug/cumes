#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/* Our Library. */
#include <input.h>
#include <sds.h>
#include <str_match.h>


#define LN "\r\n"

char hostbuf[10000];
const char* hostname;

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

void smtp_connection(){
	sds line = NULL;
	if(gethostname(hostbuf,sizeof(hostbuf)-1)>=0) hostname = hostbuf;
	else hostbuf = "localhost";
	for(;;) {
		line = smtp_readline();
		if(str
	}
}


