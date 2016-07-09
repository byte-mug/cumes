#pragma once
#include <sds.h>

static int csds_ends_with(sds s,char e){
	size_t ln = sdslen(s);
	if(ln<1)return 0;
	return s[ln-1]==e;
}
