
/**
  ******************************************************************************
  * @file:    
  * @author:  hupengepng
  * @date:    
  * @email:   hupengpengHPP@outlook.com
  ******************************************************************************
  * @attention
  * 
  ******************************************************************************
  */
  
#ifndef LIB_SRTING_H
#define LIB_SRTING_H

#ifdef __cplusplus
 extern "C" {
#endif
     
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>  
#include <math.h>	


#define SECOND_OF_DAY   86400
#define BYTE_TO_BIN "%c %c %c %c %c %c %c %c"
#define TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')
  

#define CSTD_SETBIT(x,y)  x|=(1<<y)  	      /* ��1 */
#define CSTD_CLRBIT(x,y)  x&=~(1<<y)		    /* ��0 */
#define CSTD_REVBIT(x,y)  x^=(1<<y)			    /* ȡ�� */
#define CSTD_GETBIT(x,y)  ((x) >> (y)&1)		/* ȡֵ */


#define CSTD_MAX(a, b)		((a) > (b) ? (a) : (b))
#define CSTD_MIN(a, b)		((a) > (b) ? (b) : (a))
#define CSTD_ABS(a, b)		(((a) > (b)) ? ((a) - (b)) : ((b) - (a)))
#define CSTD_SIZE(n)      (sizeof(n)/sizeof(n[0]))

#define CSTD_ERRR(conf, ret)     do                \
																	{                \
																			if (conf)    \
																			{            \
																					ret;     \
																			}            \
																	} while(0) 
																	
																			
typedef enum
{
    CRC_USA     = 0x8005,
    CRC_CCITI   = 0x1021,
    CRC_RTU     = 0xA001,     
}CRCStandard_e;


 

#pragma pack (1)
typedef struct date_time{
	
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t min;
    uint16_t sec;
    
}st_date_time, *st_date_time_ptr;

extern st_date_time g_system_time_current;
extern volatile uint32_t g_timestamp_start; 
extern volatile uint32_t g_timestamp_current; 
//extern volatile uint32_t g_timestamp_tick;
 
void lib_srting_StringTransformDigit(char* ,void* , uint8_t );
char* lib_srting_digit_transform_string(void* num, char* str, uint8_t );
uint8_t lib_srting_Crc8Itcu(void*, uint16_t );					
uint8_t lib_srting_Crc8Bq76920(uint8_t*, uint8_t ,uint8_t );
uint8_t lib_srting_BccCheck(uint8_t* p, uint16_t );
uint8_t  lib_srting_SignedTOString(char *, int16_t );
int16_t  lib_srting_StringToSigned(char *); 
uint8_t lib_srting_crc_sht31( uint8_t * p_buffer, uint16_t buf_size );
uint16_t lib_crc_modbus(uint8_t* puchMsg, uint8_t usDataLen);
void lib_crc_dgus_update(unsigned short* ctx, const unsigned char* data, size_t len);
void lib_crc_dgus_final(unsigned short* ctx, unsigned char* md);
uint8_t lib_crc_dgus_cmp(uint16_t, unsigned char*);
float lib_convert_float_endian(float value);
int16_t convert_int16_t_endian(int16_t  value);
void lib_crc_insert_string(char*, char* ,int );
int lib_is_float_negative(float num);

uint32_t lib_fml_stamp_to_time(uint32_t,st_date_time* );
uint32_t lib_fml_time_to_stamp(st_date_time);
uint16_t lib_fml_leap_year(uint16_t);
uint8_t lib_find_closest_element(const float* arr, uint8_t size, float target);
uint8_t  lib_find_character_indexes(const char* str, char ch);

#pragma pack ()
#ifdef __cplusplus
}
#endif

#endif
