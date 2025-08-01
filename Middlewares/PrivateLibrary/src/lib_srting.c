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
  
#include "lib_srting.h"

st_date_time 		  g_system_time_current;
volatile uint32_t g_timestamp_current; 
volatile uint32_t g_timestamp_start; 

//volatile uint32_t g_timestamp_tick;

const uint16_t month_days_table[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

const  uint8_t auchCRCHi[] =
{
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

/* CRC��λ�ֽ�ֵ��*/
const  uint8_t auchCRCLo[] =
{
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint8_t  lib_find_character_indexes(const char* str, char ch) {
	uint8_t i = 0;
	uint8_t index = 0;

	while (str[i]) {
		if (str[i] == ch) {
			index = i;
//			break;/* �����ξ����׸�����Ŀ���ַ��ĸ��ƣ����ξ������һ�����ֵ�λ�� */
		}
		i++;
	}
	return index;
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint8_t lib_find_closest_element(const float* arr, uint8_t size, float target) {
	
	uint8_t closest_index = 0;
	float closest_diff = fabs(arr[0] - target);
	float diff = 0;

	for (uint8_t i = 1; i < size; i++) {
		 diff = fabs(arr[i] - target);
		if (diff < closest_diff) {
			closest_diff = diff;
			closest_index = i;
		}
	}

	return closest_index;
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint16_t lib_fml_leap_year(uint16_t year)
{
    return (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));
}


/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint32_t lib_fml_time_to_stamp(st_date_time date)
{
    static  uint32_t dax = 0;
    static  uint32_t day_count = 0;
    uint16_t leap_year_count = 0;
    uint16_t i;

    for (i = 1970; i < date.year; i++){
    
        if (lib_fml_leap_year(i)){
        
            leap_year_count++;
        }
    }

    day_count = leap_year_count * 366 + (date.year - 1970 - leap_year_count) * 365;

    for (i = 1; i < date.month; i++){
    
        if ((2 == i) && (lib_fml_leap_year(date.year))){
        
            day_count += 29;
        }
        else
        {
            day_count += month_days_table[i];
        }
    }
		
    day_count += (date.day - 1);

    dax = (uint32_t)(day_count * 86400) + (uint32_t)((uint32_t)date.hour * 3600) + (uint32_t)((uint32_t)date.min * 60) + (uint32_t)date.sec;

    /* ����ʱ�䲹�� */
    dax = dax - 8 * 60 * 60;

    return dax;
}


/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint32_t lib_fml_stamp_to_time(uint32_t timep, st_date_time* date)
{
    uint32_t days = 0;
    uint32_t rem = 0;

    /* ����ʱ�䲹�� */
    timep = timep + 8 * 60 * 60;

    days = (uint32_t)(timep / 86400);
    rem = (uint32_t)(timep % 86400);

    uint16_t year;
    for (year = 1970; ; ++year){
    
        uint16_t leap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
        uint16_t ydays = leap ? 366 : 365;
        if (days < ydays){
        
            break;
        }
        days -= ydays;
    }
    date->year = year;

    static const uint16_t days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    uint16_t month;
    for (month = 0; month < 12; month++){
    
        uint16_t mdays = days_in_month[month];
        if (month == 1 || ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)) {     
            mdays = 29;
        }
				
        if (days < mdays){
        
            break;
        }
        days -= mdays;
    }
		
    date->month = month;
    date->month += 1;

    date->day = days + 1;

    date->hour = rem / 3600;
    rem %= 3600;
    date->min = rem / 60;
    date->sec = rem % 60;

    return 0;
}



/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
int lib_is_float_negative(float num) {
    // ��������ת��Ϊuint32_t����
    uint32_t* num_ptr = (uint32_t*)&num;
    // ��ȡ�������Ķ����Ʊ�ʾ
    uint32_t bits = *num_ptr;
    // �жϷ���λ�Ƿ����ã�1��ʾ������0��ʾ������
    return bits >> 31 == 1;
}
/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void InvertUint16(unsigned short* dBuf, unsigned short* srcBuf)
{
    int i;
    unsigned short tmp[4];
    tmp[0] = 0;
    for (i = 0; i < 16; i++)
    {
        if (srcBuf[0] & (1 << i))
            tmp[0] |= 1 << (15 - i);
    }
    dBuf[0] = tmp[0];
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void InvertUint8(unsigned char* dBuf, unsigned char* srcBuf)
{
    int i;
    unsigned char tmp[4];
    tmp[0] = 0;
    for (i = 0; i < 8; i++)
    {
        if (srcBuf[0] & (1 << i))
            tmp[0] |= 1 << (7 - i);
    }
    dBuf[0] = tmp[0];

}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint16_t lib_swap_bytes(uint16_t value) {
    return ((value >> 8) & 0x00FF) | ((value << 8) & 0xFF00);
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void lib_crc_dgus_update(unsigned short* ctx, const unsigned char* data, size_t len)
{


    uint8_t uchCRCHi = ((*ctx) & 0xff00) >> 8;
    uint8_t uchCRCLo = ((*ctx) & 0x00ff);
    uint16_t tmp = 0;

    uint8_t uIndex;

    while (len--)
    {
        uIndex = uchCRCHi ^ *data++;
        uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex];
        uchCRCLo = auchCRCLo[uIndex];
    }

    tmp = ((uint16_t)uchCRCHi) << 8 | uchCRCLo; // MODBUS �涨��λ��ǰ
    (*ctx) = tmp;

}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint8_t lib_crc_dgus_cmp(uint16_t crc1, unsigned char* crc2){

		uint8_t high_byte = (uint8_t)(crc1>>8);
		uint8_t low_byte = (uint8_t)crc1;
		if(high_byte == crc2[0] && low_byte == crc2[1]){
			return 1;
		}else{
			return 0;
		}

}
/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void lib_crc_insert_string(char* dest, char* src, int index_pos) {

    int i = 0;
    int j = 0;

    for (i = strlen(dest); i >= index_pos; i--) {
        dest[i + strlen(src)] = dest[i];
    }


    for (j = 0; j < strlen(src); j++) {
        dest[index_pos + j] = src[j];
    }
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void lib_crc_dgus_final(unsigned short* ctx, unsigned char* md)
{

    *md++ = ((*ctx) & 0xFF00U) >> 8;
    *md++ = ((*ctx) & 0x00FFU);
}
/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint16_t lib_crc_modbus(uint8_t* puchMsg, uint8_t usDataLen)
{
    uint16_t wCRCin = 0xFFFF;
    uint16_t wCPoly = 0x8005;
    uint8_t wChar = 0;

    while (usDataLen--)
    {
        wChar = *(puchMsg++);
        InvertUint8(&wChar, &wChar);
        wCRCin ^= (wChar << 8);
        for (int i = 0; i < 8; i++)
        {
            if (wCRCin & 0x8000)
                wCRCin = (wCRCin << 1) ^ wCPoly;
            else
                wCRCin = wCRCin << 1;
        }
    }
    InvertUint16(&wCRCin, &wCRCin);
    return (wCRCin);
}


/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint16_t lib_crc_modbus2(uint8_t* puchMsg, uint8_t usDataLen)
{
    uint16_t wCRCin = 0xFFFF;
    uint16_t wCPoly = 0x8005;
    uint8_t wChar = 0;

    while (usDataLen--)
    {
        wChar = *(puchMsg++);
        InvertUint8(&wChar, &wChar);
        wCRCin ^= (wChar << 8);
        for (int i = 0; i < 8; i++)
        {
            if (wCRCin & 0x8000)
                wCRCin = (wCRCin << 1) ^ wCPoly;
            else
                wCRCin = wCRCin << 1;
        }
    }
    InvertUint16(&wCRCin, &wCRCin);
    return lib_swap_bytes(wCRCin);
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint8_t lib_srting_BccCheck(uint8_t* data, uint16_t length){

	uint8_t bcc = 0x00;

	for (uint16_t i = 0;i<length;){

		bcc ^= data[i++];
	}
	return bcc;
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint8_t lib_srting_crc_sht31( uint8_t * p_buffer, uint16_t buf_size )
{
    uint8_t crc = 0xff;
		uint8_t i=0;
    if(buf_size <= 0)
    {
        return crc;
    }
    while( buf_size-- )
    {
        for (  i = 0x80; i != 0; i /= 2 )
        {
            if ( (crc & 0x80) != 0)
            {
                crc *= 2;
                crc ^= 0x31; 
            }
            else
            {
                crc *= 2;
            }
 
            if ( (*p_buffer & i) != 0 )
            {
                crc ^= 0x31;
            }
        }
        p_buffer++;
    }
    return crc;
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint8_t lib_srting_Crc8Bq76920(uint8_t *ptr, uint8_t len,uint8_t key)
{
    uint8_t i;          
    uint8_t crc=0;      
    while(len--!=0)           
    {
        for(i=0x80; i!=0; i/=2)    
        {
            if((crc & 0x80) != 0)    
            {
                    crc *= 2;      
                    crc ^= key;    
            }
            else
                    crc *= 2;

            if((*ptr & i)!=0)
                    crc ^= key;
        }
        ptr++;
    }
    return(crc);
}
   
/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint8_t lib_srting_Crc8Itcu(void* d, uint16_t length)
{
    uint8_t i;
    uint8_t crc = 0;  
		uint8_t* data = (uint8_t*)d;
	 
    while(length--){
    
        crc ^= *data++;         
        for ( i = 0; i < 8; i++ ){
        
            if ( crc & 0x80 )
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc ^ 0x55;
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void lib_srting_StringTransformDigit(char* str,void* num, uint8_t type){

    char flag = '+';
    int Vint = 0;
    float Vfloat = 0;
    uint32_t Vu32 = 0;
		int16_t Vshort = 0;
    int r = 0;

    if (type == 1) {
        if (*str == '-'){
        
            ++str;
            flag = '-';
        }

        r = sscanf(str, "%d", &Vint);
        if (flag == '-'){
       
            Vint = -Vint;
        }
        *((int*)num) = Vint;
    }
    else if(type == 2) {
        if (*str == '-'){
        
            ++str;
            flag = '-';
        }

        r = sscanf(str, "%f", &Vfloat);
        if (flag == '-'){
        
            Vfloat = -Vfloat;
        }
        *((float*)num) = Vfloat;
    }
    else if (type == 3 || type == 4 || type == 5) {
        r = sscanf(str, "%u", &Vu32);
        *((uint32_t*)num) = Vu32;
    }else if(type == 6){
        if (*str == '-'){
        
            ++str;
            flag = '-';
        }

        r = sscanf(str, "%hd", &Vshort);
        if (flag == '-'){
       
            Vshort = -Vshort;
        }
        *((int16_t*)num) = Vshort;			
		}
		r = r;
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
char* lib_srting_digit_transform_string(void* num, char* str, uint8_t type){

	
    switch (type){
    
				case 12:
					sprintf(str, "%d", *((int*)num)); 
					break;
				case 16:
					sprintf(str, "%-7.2f", *((float*)num));     
					break;
				case 1:/* U8Type U16Type U32Type */
					sprintf(str, "%u", *((uint8_t*)num));
					break;
				case 3:
					sprintf(str, "%u", *((uint16_t*)num));
					break;
				case 5:	
					sprintf(str, "%u", *((uint32_t*)num));
					break;


    default:
        break;
    }
    

    return str;

}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void lib_srting_HeadTail(uint8_t *head, uint16_t len){

    uint8_t *p = head + len - 1;

    while (head < p)
    {
        *head ^= *p;
        *p ^= *head;
        *head++ ^= *p--;
    }
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
int16_t  lib_srting_StringToSigned(char *buf){

    uint8_t flag = 0;
    int16_t num = 0;

	if (*buf == '-')
	{
		flag = 1;
		buf++;
	}

	while ('0' <= *buf && *buf <= '9')
    {
        num *= 10;
        num += *buf++ - '0';
    }

    if (flag == 1)
    {
        num = (~num) + 1;
    }
	
    return num;
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint8_t  lib_srting_SignedTOString(char *buf, int16_t num){

    uint8_t flag = 0, i = 0;
    char *p = buf;

    if (num < 0)
    {
        *p++ = '-';
        num = ~((uint32_t)(num)) + 1;
        flag = 1;
    }
    
    do 
    {
        i++;
        *p++ = num%10 + '0';
        num /= 10;
    } while (num > 0);
    *p = '\0';
    lib_srting_HeadTail((uint8_t*)(buf +flag), i);
	
    return i + flag;
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
float lib_convert_float_endian(float value) {
    union {
        float f;
        uint8_t bytes[4];
    } original, converted;

    original.f = value;

    // ��С��ת��
    converted.bytes[0] = original.bytes[3];
    converted.bytes[1] = original.bytes[2];
    converted.bytes[2] = original.bytes[1];
    converted.bytes[3] = original.bytes[0];

    return converted.f;
}
 
/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
int16_t convert_int16_t_endian(int16_t  value) {
    union {
        int16_t f;
        uint8_t bytes[2];
    } original, converted;

    original.f = value;

    // ��С��ת��
    converted.bytes[0] = original.bytes[1];
    converted.bytes[1] = original.bytes[0];


    return converted.f;
}

 


