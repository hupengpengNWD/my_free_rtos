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
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lib_finite_state_machine.h"


uint8_t lib_FsmEvnQueueStore[fsmSum][sizeof(st_evn)*LIB_QUEUE_CIRCULAR_LENGTH];
uint8_t lib_FsmEvnQueueStore2[fsmSum][sizeof(st_evn)*LIB_QUEUE_CIRCULAR_LENGTH];
st_queue lib_FsmEvnQueue[fsmSum];
st_queue lib_FsmEvnQueue2[fsmSum];

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
static void lib_finite_state_machine_transit(
            st_fsm_ptr ptr, 
            uint8_t state){

	ptr->curState = state;
}


/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void lib_finite_state_machine_poll(
     st_fsm_ptr ptr, 
     st_evn e){ 
  
    //void(*actionFunc)(st_fsm*, st_evn) = NULL; 
    void(*actionFunc)(void*, st_evn) = NULL;         
		uint8_t nextState = 0;
		uint8_t flag = 0;
    

    /* ����һά�� */
	for (uint8_t i = 0; i < ptr->transSize; i++)	{

		if (e.event_type == ptr->transTable[i].trgEvent && 
				ptr->curState == ptr->transTable[i].curState){
		
			flag = 1;
			actionFunc = ptr->transTable[i].actionFunc;
			nextState = ptr->transTable[i].nextState;
			break;
		}
	}
	
    /*-------------------------------------------------*/
	if (flag){
		
		if (actionFunc != NULL) {
            actionFunc(ptr->UserArgu, e); /* ִ����Ӧ���� */
        }            
		lib_finite_state_machine_transit(ptr, nextState); /* ״̬Ǩ�� */
	}
  
}


/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void lib_finite_state_machine_initialize(st_fsm_ptr ptr ){
     
     ptr->UserArgu = ptr;
     ptr->evn_course_queue->initialize(
		 ptr->evn_course_queue, 
	   LIB_QUEUE_CIRCULAR_LENGTH, 
	   sizeof(st_evn), 
		 lib_FsmEvnQueueStore[ptr->fsm_id]);
		 
     ptr->evn_trigger_queue->initialize(
		 ptr->evn_trigger_queue, 
		 LIB_QUEUE_CIRCULAR_LENGTH, 
		 sizeof(st_evn), 
		 lib_FsmEvnQueueStore2[ptr->fsm_id]);

}	
                    
/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
void lib_finite_state_machine_configure(st_fsm_ptr  ptr){

		 ptr->initialize = lib_finite_state_machine_initialize;
     ptr->poll = lib_finite_state_machine_poll;
     ptr->transit = lib_finite_state_machine_transit;
  
     ptr->evn_course_queue->configure(ptr->evn_course_queue);
     ptr->evn_trigger_queue->configure(ptr->evn_trigger_queue);
    
}

/**
  * @name     	
  * @brief	    
  * @param		
  * @return		
  * @remark  		
  */
uint8_t lib_finite_state_machine_create(st_fsm_ptr ptr){

    if(ptr->fsm_id >= fsmSum){
        return 1;
    }

    ptr->configure = lib_finite_state_machine_configure;             
    ptr->evn_course_queue = &lib_FsmEvnQueue[ptr->fsm_id];
    ptr->evn_trigger_queue = &lib_FsmEvnQueue2[ptr->fsm_id];
    
    lib_queue_circular_create(ptr->evn_course_queue);
    lib_queue_circular_create(ptr->evn_trigger_queue); 

    return 0;
}

