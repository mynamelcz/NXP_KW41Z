#include "common.h"





void sys_delay_us(u32 us)
{
	while(us--);
}


void print_code_version(void)
{
	;
}


void my_memcpy(char *tar, const char *src, u32 len)
{
	ASSERT(tar);
	ASSERT(src);
	while(len--)
		*tar++ = *src++;
}

 
#ifdef __ASSERT_PARAM
void assert_fail(u8* file, u32 line)
{
	DBUG_Printf("[assert_fail] __file:%s ;__line:%d\n",file,line); 
	while(1);
}
#endif




static int uart_putchar(char c)
{
    PUTCHAR(c);               /// 需要自己添加
    return 0;
}

int my_puthex8(char c_arg)
{
    char *hex = "0123456789ABCDEF";
    uart_putchar(hex[(c_arg>>4)&0x0f]);
    uart_putchar(hex[(c_arg>>0)&0x0f]);
    return 0;
}
int my_put_hex(int i_arg)
{
    if(i_arg <= 0xff)
    {
        my_puthex8(i_arg);
    }
    else if(i_arg <= 0xffff)
    {
        my_puthex8((i_arg>> 8)&0xff);
        my_puthex8((i_arg>> 0)&0xff);
    }
    else
    {
        my_puthex8((i_arg>>24)&0xff);
        my_puthex8((i_arg>>16)&0xff);
        my_puthex8((i_arg>> 8)&0xff);
        my_puthex8((i_arg>> 0)&0xff);
    }
    return 0;
}
int my_printhex(const char *buf, unsigned int len)
{
    u8 i = 0;
    while(len--)
    {
       if(i++ == 16)
       {
           i = 1;
           uart_putchar('\n');
       }
       my_puthex8(*buf++);
       uart_putchar(' ');
    }
	uart_putchar('\n');
	return len;
}












