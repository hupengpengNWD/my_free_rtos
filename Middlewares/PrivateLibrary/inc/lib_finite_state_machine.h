
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

#ifndef LIB_FINITE_STATE_MACHINE_H
#define LIB_FINITE_STATE_MACHINE_H


#ifdef __cplusplus
 extern "C" {
#endif
   
#include <stdint.h>
#include "lib_queue_circular.h"

#define LIB_QUEUE_CIRCULAR_LENGTH 64

typedef enum{

    fsm0,
    fsm1,
    fsm2,
    fsm3,
 
    fsmSum
}et_fsm_id;

#pragma pack (1)

typedef struct evn{
    
    uint8_t event_type;          /* �¼����� */
    uint8_t EventUser;           /* �Զ������ */
    uint8_t event_data[6];       /* �¼����� */
 
}st_evn, st_evn_ptr ; 

struct fsm;/* ���������������� */

typedef struct table{
	
  uint8_t  curState;/* ��ǰ״̬ */
  uint8_t  trgEvent;/* �����¼� */
  //void     (*actionFunc)(struct fsm*, st_evn);/* �������� */
  void     (*actionFunc)(void*, st_evn);/* �������� */
  uint8_t  nextState; 

}st_fsm_state_transition_table, *st_fsm_state_transition_table_ptr ; 


typedef struct fsm{ 

  et_fsm_id	fsm_id;     		 			 /* ��� */
  const st_fsm_state_transition_table* transTable;  /* ״̬ת����(һά��) */
  uint8_t	transSize;   					   /* ״̬ת������С */
  uint8_t	curState;    		   		   /* ��ǰ״̬ */ 
  uint8_t	lastState;   			       /* ��ʷ״̬ */  
  st_queue_ptr	evn_course_queue;  /* �¼����� */
  st_queue_ptr	evn_trigger_queue; /* �¼����� */
  void*	UserArgu;    				 		   /* �û����� */  
	
  void  (*configure)(struct fsm*);
  void  (*initialize)(struct fsm*);     
  void (*poll)(struct fsm*, st_evn); 
  void (*transit)(struct fsm*, uint8_t);  
 
   
    
}st_fsm, *st_fsm_ptr;


#pragma pack ()

uint8_t lib_finite_state_machine_create(st_fsm_ptr ptr);
     

#ifdef __cplusplus
}
#endif

#endif
/*---End of File---*/

