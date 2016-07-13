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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>

/* Our library */
#include <sds.h>
#include <str_hol.h>

#define failon(x) if(!(x))abort()

static time_t mtime;
static sds todof;
static sds mesg;
static size_t mesgl;
static const char* program;

static void sig_exit()
{
	int status;
	while(wait(&status)<=0);
}

void clock_init(const char* progfile){
	program = progfile;
	const char * queue = getenv("QUEUE"); failon(queue);
	todof = sdsnew(queue); failon(todof);
	if(!csds_ends_with(todof,'/')) todof = sdscat(todof,"/todo");
	else todof = sdscat(todof,"todo");
	failon(todof);
	mesg = sdsdup(todof); failon(mesg);
	mesg = sdscat(mesg,"/"); failon(mesg);
	mesgl = sdslen(mesg);
}

static void clock_check(){
	struct dirent * ent;
	struct stat buf;
	DIR* dir = opendir(todof); failon(dir);
	sds temp;
	pid_t pid;
	while(ent = readdir(dir)){
		sdssetlen(mesg,mesgl);
		temp = sdscat(mesg,ent->d_name); if(!temp) continue;
		mesg = temp;
		if(stat(mesg,&buf)<0)continue;
		if(!S_ISREG(buf.st_mode))continue;
		if(mtime>=buf.st_mtime)continue;
		//printf("woe, we found a file: %s\n",mesg);
		pid = fork();
		if(pid<0) { perror("fork"); abort(); }
		if(pid==0) {
			closedir(dir);
			execlp(program,program,"-p",ent->d_name,NULL);
			perror("execlp");
			abort();
		}
	}
	closedir(dir);
}

void clock_loop(){
	mtime = 0;
	struct stat buf;
	for(;;){
		if(stat(todof,&buf)<0) abort();
		if(mtime>=buf.st_mtime) {
			usleep(1000);
			continue;
		}
		clock_check();
		mtime = buf.st_mtime;
	}
}



