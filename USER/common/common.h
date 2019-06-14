#ifndef __COMMON_H
#define __COMMON_H

#include "fsl_debug_console.h"

#define __ASSERT_PARAM



#define DBUG_Printf 	DbgConsole_Printf		// MY_RTT_printf
#define DBUG_Put_hex 	my_printhex	


//====== ERR  =========//
#define ERR_printf(res)		 PRINTF("[%d][ERR][%s] Res: %d\n",__LINE__,__func__,res)


#define __DBUG_DEV_SPI_SD

#ifdef __DBUG_DEV_SPI_SD
#define sd_printf		DBUG_Printf
#define sd_puthex		DBUG_Put_hex
#else
#define sd_printf(...)
#define sd_puthex(...)	
#endif






		

/* exact-width signed integer types */
typedef signed           char s8;
typedef signed short     int s16;
typedef signed           int s32;


/* exact-width unsigned integer types */
typedef unsigned          char u8;
typedef unsigned short    int u16;
typedef unsigned          int u32;
typedef unsigned long long	  u64;

#define BIT(x)  			(1<<x)
#define STE_BIT(x,b)  do{x |=  BIT(b);}while(0)
#define CLR_BIT(x,b)  do{x &= ~BIT(b);}while(0)
#define REV_BIT(x,b)  do{x ^=  BIT(b);}while(0)
#define GET_BIT(x,b)  ((x>>b)&0x1)


#define N_BIT1(x)			(BIT(x)-1)
#define BITS_SET(x,s,n,v)	do{x= (x&(~(N_BIT1(n)<<s)))|(v<<s);}while(0)



#ifdef __ASSERT_PARAM
void assert_fail(u8* file, u32 line);
#define ASSERT(expr) ((expr) ? (void)0U : assert_fail((u8 *)__FILE__, __LINE__))
#else
#define ASSERT(expr) ((void)0U)	 
#endif



void sys_delay_us(u32 us);
void print_code_version(void);

void my_memcpy(char *tar, const char *src, u32 len);

int my_printhex(const char *buf, unsigned int len);


#endif




