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


