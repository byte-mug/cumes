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


int csds_match(sds subj,const char* pat1,const char* pat2){
	size_t i,len = sdslen(subj);
	if(pat2==NULL) pat2 = pat1;
	for(i=0;i<len;++i,++pat1,++pat2){
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


