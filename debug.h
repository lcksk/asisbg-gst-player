/*
 * debug.h - console debug header
 *
 *  Created on: Mar 2, 2010
 *      Author: xpucmo
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdio.h>

#define DBG(n)	printf("=={ DEBUG POINT: %d }==\n", n);
#define D(n)	DBG(n)
#define DS(s)	printf("=={ DEBUG MSG: %s }==\n", s);
#define DX(x)	printf("=={ DEBUG VALUE: 0x%X }==\n", x);
#define DA(a,l)	do { \
	int _ARRAY_COUNTER_; \
	printf("=={ DEBUG ARRAY:"); \
	for(_ARRAY_COUNTER_ = 0; _ARRAY_COUNTER_ < l; _ARRAY_COUNTER_++) \
		printf(" %02X", a[_ARRAY_COUNTER_]); \
	printf(" }==\n"); \
	}while(0);

#define DAP(a,l,p)	do { \
	int _ARRAY_COUNTER_; \
	printf("=={ DEBUG <%d> ARRAY:", p); \
	for(_ARRAY_COUNTER_ = 0; _ARRAY_COUNTER_ < l; _ARRAY_COUNTER_++) \
		printf(" %02X", a[_ARRAY_COUNTER_]); \
	printf(" }==\n"); \
	}while(0);


#endif /* DEBUG_H_ */
