
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
  
#ifndef LIB_QUEUE_CIRCULAR_H
#define LIB_QUEUE_CIRCULAR_H

#ifdef __cplusplus
 extern "C" {
#endif
     
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>



#define	MYQUEUE_API_CREATELOCK(v)
#define MYQUEUE_API_DELETELOCK(v)
#define	MYQUEUE_API_LOCK(v)
#define MYQUEUE_API_UNLOCK(v)


//#define debug_i(format,...)				printf(format"\n",##__VA_ARGS__)
#define NUM_IN_QUEUE(v)					((((v)->front) <= ((v)->rear))?(((v)->rear)-((v)->front)):(((v)->len)-((v)->front)+((v)->rear)))
#define LFET_NUM_IN_QUEUE(v)			((((v)->front) <= ((v)->rear))?((((v)->len)-1)-(((v)->rear)-((v)->front))):(((v)->front)-((v)->rear)-1))


/*队列句柄*/
#pragma pack (1)

typedef struct LibQueue{
	
	void     *buffer;		/*数据缓冲区*/
	size_t   len;    		/*队列长度*/
	size_t   size;   		/*单个数据大小(单位 字节)*/
	size_t   front;  		/*数据头,指向下一个空闲存放地址*/
	size_t   rear;   		/*数据尾，指向第一个数据*/


  void  (*initialize)(struct LibQueue*, size_t  queue_len, size_t  item_size, void *buffer); 
  void  (*configure)(struct LibQueue*); 
  
	void (*destroy)(struct LibQueue*);
	size_t (*capacity)(struct LibQueue*);
	size_t (*used)(struct LibQueue*);
	size_t (*surplus)(struct LibQueue*);
	
	bool (*full)(struct LibQueue*);
	bool (*empty)(struct LibQueue*);
	
	bool (*put)(struct LibQueue* ,void* ,size_t);
	bool (*get)(struct LibQueue* ,void* ,size_t );
	bool (*peek)(struct LibQueue* ,void*,size_t ,size_t);
	
	bool (*clear)(struct LibQueue* ,size_t);
	bool (*clears)(struct LibQueue* );
	
}st_queue, *st_queue_ptr;
#pragma pack ()


void lib_queue_circular_create(st_queue_ptr );

#ifdef __cplusplus
}
#endif

#endif
