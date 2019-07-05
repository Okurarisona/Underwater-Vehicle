#ifndef _FLASH_H_
#define _FLASH_H_

#define FLIGHT_PARAMETER_TABLE_NUM  60
#define PID_USE_NUM  	8

#include "DataType.h"

typedef struct
{
		uint8 No_0xFF[PID_USE_NUM];
}PID_Parameter_Flag;

typedef enum  // _e 表示枚举
{
		VEHICLE_MODE_e,              //工作模式
		DEBUG_TOOL_e,                //Debug工具
		
		ROBOTIC_ARM_MAX_VALUE_e,    //机械臂开启最大值  
		ROBOTIC_ARM_MIN_VALUE_e,   	//机械臂关闭值
		ROBOTIC_ARM_MED_VALUE_e, 		//机械臂当前值
		ROBOTIC_ARM_SPEED_e,   			//机械臂转动速度
	
		YUNTAI_MAX_VALUE_e,    	 //云台向上最大当前值
		YUNTAI_MIN_VALUE_e,	 		 //云台向下当前值	
		YUNTAI_MED_VALUE_e,   	 //云台中值
		YUNTAI_SPEED_e,  				 //云台转动速度

		COMPASS_OFFSET_ANGLE_e,    //指南针补偿角度
	


	
		PARAMEMER_MAX_NUMBER_e,    //最大参数值
	
}NORMAL_PARAMETER_TABLE;


typedef enum //FLASH 地址address枚举
{
		PID1_PARAMETER_KP,
		PID1_PARAMETER_KI,
		PID1_PARAMETER_KD,
		PID2_PARAMETER_KP,
		PID2_PARAMETER_KI,
		PID2_PARAMETER_KD,
		PID3_PARAMETER_KP,
		PID3_PARAMETER_KI,
		PID3_PARAMETER_KD,
		PID4_PARAMETER_KP,
		PID4_PARAMETER_KI,
		PID4_PARAMETER_KD,
		PID5_PARAMETER_KP,
		PID5_PARAMETER_KI,
		PID5_PARAMETER_KD,
		PID6_PARAMETER_KP,
		PID6_PARAMETER_KI,
		PID6_PARAMETER_KD,
		PID7_PARAMETER_KP,
		PID7_PARAMETER_KI,
		PID7_PARAMETER_KD,
		PID8_PARAMETER_KP,
		PID8_PARAMETER_KI,
		PID8_PARAMETER_KD,   //地址枚举
		
		
		PID9_PARAMETER_KP,
		PID9_PARAMETER_KI,
		PID9_PARAMETER_KD,   //地址枚举
		
				
		PID10_PARAMETER_KP,
		PID10_PARAMETER_KI,
		PID10_PARAMETER_KD,   //地址枚举
	

}IMPORTANT_PARAMETER_TABLE;

void Parameter_SelfCheck(uint32 *RealParameter,uint32 *TempParameter);
void Normal_Parameter_SelfCheck_With_Flash(void); //Flash参数自检 若为-1 或 0 则为 非正常数据 
/* FLASH 更新 普通值 */
void Flash_Update(void);

int Normal_Parameter_Init_With_Flash(void);
															 
void Save_PID_Parameter(void);			
															 
#endif



