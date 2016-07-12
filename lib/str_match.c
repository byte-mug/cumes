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
#include <str_match.h>
#include <stdlib.h>

int csds_match(sds subj,const char* pat1,const char* pat2){
	size_t len = sdslen(subj);
	int i;
	if(pat2==NULL) pat2 = pat1;
	for(i=0;(i<len)&&(i>=0);++i,++pat1,++pat2){
		if(*pat1==0){
			sdsrange(subj,i,-1);
			return 1;
		}
		if(
			((*pat1)!=subj[i])  &&
			((*pat2)!=subj[i])
		) return 0;
		if(*pat2==0)--pat2; /* Avoid overflow. */
	}
	/* pattern 1 must end here, or no match.*/
	if(*pat1) return 0;

	/* The entire string matched, so set string size to 0. */
	sdssetlen(subj,0);
	return 1;
}

int csds_pattm(sds subj, const char* pattern,char action){
	int i=0;
	size_t len = sdslen(subj);
	char c;
	#define ISDIE ((i>=len)||(i<0))
	while(c = *pattern){
		if(ISDIE) return 0;
		switch(c){
		case '%':
			pattern++;
			switch(*pattern){
			case 'u':
				pattern++;
				c=*pattern;
				while(c!=subj[i]){
					i++;
					if(ISDIE) return 0;
				}
				i++;
				pattern++;
				break;
			default:
				if((*pattern)!=subj[i]) return 0;
				i++;
				pattern++;
			}
			break;
		default:
			if(c!=subj[i]) return 0;
			i++;
			pattern++;
		}
	}
	#undef ISDIE
	if(((i>len)||(i<0)))return 0;
	switch(action){
	case '[':sdsrange(subj,i?i-1:i,-1);break;
	case '(':sdsrange(subj,i,-1);break;
	case ')':sdsrange(subj,0,i>1?i-2:0);break;
	case ']':sdsrange(subj,0,i?i-1:0);break;
	case '!':return 1;
	case '^':return i+1;
	default:return 0;
	}
	return 1;
}

int cstr_indexof(const char* pattern,char num){
	int i;
	for(i=0;pattern[i];i++){
		if(pattern[i]==num)return i;
		
	}
	return -1;
}


