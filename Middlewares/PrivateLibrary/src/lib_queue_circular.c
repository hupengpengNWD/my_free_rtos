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
	
#include "lib_queue_circular.h"
/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void lib_queue_circular_destroy(st_queue_ptr queue) {
	
	if(NULL == queue) {
		return;
	}
	
	MYQUEUE_API_LOCK(queue);
	//free(queue->buffer);
	MYQUEUE_API_UNLOCK(queue);
	MYQUEUE_API_DELETELOCK(queue);
	
	//free(queue);
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
size_t  lib_queue_circular_used(st_queue_ptr queue) {
	
	if(NULL == queue) {
		return 0;
	}
	
	MYQUEUE_API_LOCK(queue);
	size_t num_in_queue = NUM_IN_QUEUE(queue);
	MYQUEUE_API_UNLOCK(queue);
	return num_in_queue;
}


/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
size_t lib_queue_circular_surplus(st_queue_ptr queue) {
	
	if(NULL == queue) {
		return 0;
	}
	
	MYQUEUE_API_LOCK(queue);
	size_t left_num_in_queue = LFET_NUM_IN_QUEUE(queue);
	MYQUEUE_API_UNLOCK(queue);
	return left_num_in_queue;
}


/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
size_t lib_queue_circular_capacity(st_queue_ptr queue) {
	
	return (NULL == queue)?0:(queue->len - 1);
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
bool lib_queue_circular_full(st_queue_ptr queue) {
	if(NULL == queue) {
		return false;
	}
	return (lib_queue_circular_used(queue)  == ((queue->len)-1));
}


/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
bool lib_queue_circular_empty(
	st_queue_ptr queue) {
		
	return (NULL==queue)?false:(0==lib_queue_circular_used(queue));
}


/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
static size_t lib_queue_circular_minimum(
	size_t a, 
	size_t b) {

	return a<b ? a : b;
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
static void* lib_queue_circular_copy(
	void *dst, 
	const void *src, 
	size_t n) {
	return memcpy(dst,src,n);
}


/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
bool lib_queue_circular_put(
	 st_queue_ptr queue, 
	 void* buf,
	 size_t num) {
	
	if((NULL==queue) ||
	   (NULL==queue->buffer) ||
	   (NULL==buf) || 
	   (0==num)) {
		return false;
	}
	
	bool rt = false;
	
	MYQUEUE_API_LOCK(queue);
	size_t left_num_in_queue = LFET_NUM_IN_QUEUE(queue);
	
	if (num <= left_num_in_queue) {
		
		size_t templen = lib_queue_circular_minimum((queue->len) - (queue->rear), num);
		
		if (templen > 0) {
			lib_queue_circular_copy((char *)(queue->buffer) + (queue->rear)*(queue->size), buf, templen*(queue->size));
		}
		
		if (num > templen) {
			lib_queue_circular_copy((char *)(queue->buffer), (char *)buf + templen*(queue->size), (num - templen)*(queue->size));
		}
		queue->rear = (queue->rear + num) % (queue->len);
		rt = true;
	}else {
		
	}
	
	MYQUEUE_API_UNLOCK(queue);
	return rt;
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
bool lib_queue_circular_get(
	 st_queue_ptr queue,
	 void *buf,
	 size_t num) {
	
	if((NULL==queue) || 
	   (NULL==queue->buffer) || 
	   (NULL==buf) || 
	   (0==num)) {

		return false;
	}
	
	bool rt = false;
	
	MYQUEUE_API_LOCK(queue);
	size_t num_in_queue = NUM_IN_QUEUE(queue);
	
	if (num <= num_in_queue) {
		
		size_t templen = lib_queue_circular_minimum((queue->len) - (queue->front), num);
		if (templen > 0) {
			lib_queue_circular_copy((char *)buf, (char *)(queue->buffer) + (queue->front)*(queue->size), templen*(queue->size));
		}
		if (num > templen) {
			lib_queue_circular_copy((char *)buf + templen*(queue->size), queue->buffer, (num - templen)*(queue->size));
		}
		queue->front = (queue->front + num) % (queue->len);
		rt = true;
	}else {
		
	}
	
	MYQUEUE_API_UNLOCK(queue);
	return rt;
}


/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
bool lib_queue_circular_peek(
	 st_queue_ptr queue,
	 void *buf, 
	 size_t num, 
	 size_t offset) {
	
	if((NULL==queue) || (NULL==queue->buffer) || (NULL==buf) || (0==num)) {
		return false;
	}
	
	bool rt = false;
	
	MYQUEUE_API_LOCK(queue);
	size_t num_in_queue = NUM_IN_QUEUE(queue);
	
	if ((offset + num) <= num_in_queue) {
	
		size_t temp_front = (queue->front) + offset;
		size_t templen = lib_queue_circular_minimum((queue->len) - temp_front, num);
		
		if (templen > 0) {
			lib_queue_circular_copy((char *)buf, (char *)(queue->buffer) + temp_front*(queue->size), templen*(queue->size));
		}

		if (num > templen) {
			lib_queue_circular_copy((char *)buf + templen*(queue->size), queue->buffer, (num - templen)*(queue->size));
		}
		rt = true;
	}else {

	}
	MYQUEUE_API_UNLOCK(queue);
	return rt;
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
bool lib_queue_circular_clear(st_queue_ptr queue,size_t num) {
	
	if((NULL==queue) || (0==num)) {
		return false;
	}
	
	bool rt = true;
	
	MYQUEUE_API_LOCK(queue);
	size_t num_in_queue = NUM_IN_QUEUE(queue);
	
	if(num <= num_in_queue) {
		queue->front = (queue->front + num) % (queue->len);
	}else {
		rt = false;
	}

	MYQUEUE_API_UNLOCK(queue);
	return rt;
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
bool lib_queue_circular_clears(st_queue_ptr queue) {
	
	if(NULL == queue) {
		
		return false;
	}
	
	MYQUEUE_API_LOCK(queue);
	queue->front = queue->rear;
	MYQUEUE_API_UNLOCK(queue);
	return true;
}


/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void lib_queue_circular_initialize(
     st_queue_ptr ptr, 
     size_t  queue_len, 
     size_t  item_size, 
     void    *buffer){
     
	
	ptr->size = item_size;
	ptr->len = queue_len; 
	ptr->buffer = buffer;
	ptr->front = ptr->rear = 0;    
 
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void lib_queue_circular_configure(st_queue_ptr ptr){

  	ptr->size = 0;
	ptr->len = 0; 
	ptr->buffer = 0;
	ptr->front = ptr->rear = 0; 
  
  	ptr->used = lib_queue_circular_used;
	ptr->surplus = lib_queue_circular_surplus;
	ptr->capacity = lib_queue_circular_capacity;

	ptr->put = lib_queue_circular_put;
	ptr->clear = lib_queue_circular_clear;
	ptr->clears = lib_queue_circular_clears;
	ptr->destroy = lib_queue_circular_destroy;

	ptr->empty = lib_queue_circular_empty;
	ptr->full = lib_queue_circular_full;
	ptr->get = lib_queue_circular_get;
	ptr->peek = lib_queue_circular_peek;
    	
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void lib_queue_circular_create(st_queue_ptr ptr){ 
	   
  ptr->configure = lib_queue_circular_configure;
  ptr->initialize = lib_queue_circular_initialize;
  
 
}
    
