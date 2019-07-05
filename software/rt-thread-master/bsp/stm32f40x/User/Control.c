/*
 * ControlCmd.c
 *
 *  Created on: 2019年3月20日
 *      Author: zengwangfa
 *      Notes:  方位角控制、深度控制
 */
#include <rtthread.h>
#include <elog.h>
#include <stdlib.h>
#include <math.h>

#include "Control.h"
#include "PID.h"
#include "RC_Data.h"

#include "focus.h"
#include "led.h"
#include "servo.h"
#include "PropellerControl.h"
#include "propeller.h"


float Yaw_Control = 0.0f;//Yaw—— 偏航控制 
float Yaw = 0.0f;
int16 Force1 = 0;
int16 Force2 = 0;


void Convert_RockerValue(Rocker_Type *rc) //获取摇杆值
{

		rc->X = ControlCmd.Move - 128; 			  //摇杆值变换：X轴摇杆值 -127 ~ +127
		rc->Y = ControlCmd.Translation- 128  ;//					  Y轴摇杆值 -127 ~ +127
		
		rc->Angle = Rad2Deg(atan2(rc->X,rc->Y));
		if(rc->Angle < 0){rc->Angle += 360;} //角度变换 
																				 /* 以极坐标定义 角度顺序 0~360°*/ 	
		rc->Force = sqrt(rc->X*rc->X+rc->Y*rc->Y);	//求合力斜边
		
		Force1 = (sqrt(2)/2)*(rc->X + rc->Y);
		Force2 = (sqrt(2)/2)*(rc->X - rc->Y);		
		
		//掌舵一号
		PropellerPower.leftUp =    PropellerDir.leftUp    * (-Force1)*2 + PropellerError.leftUp;
		PropellerPower.rightUp =   PropellerDir.rightUp   * ( Force2)*2 + PropellerError.rightUp;   
		PropellerPower.leftDown =  PropellerDir.leftDown  * (-Force2)*2 + PropellerError.leftDown ; 
		PropellerPower.rightDown = PropellerDir.rightDown * (-Force1)*2 + PropellerError.rightDown;
		
}







/**
  * @brief  highSpeed Devices_Control(高速设备控制)
  * @param  None
  * @retval None
  * @notice 
  */
void control_highSpeed_thread_entry(void *parameter)//高速控制线程
{
		
		rt_thread_mdelay(5000);//等待外部设备初始化成功
		while(1)
		{
				Control_Cmd_Get(&ControlCmd); //控制命令获取 所有上位控制命令都来自于此【Important】
				Convert_RockerValue(&Rocker);

				if(UNLOCK == ControlCmd.All_Lock){
						Focus_Zoom_Camera(&ControlCmd.Focus);//变焦聚焦摄像头控制
						Depth_Control(); //深度控制
				}

				rt_thread_mdelay(10);
		}

}

/**
  * @brief  lowSpeed Devices_Control(低速设备控制)
  * @param  None
  * @retval None
  * @notice 
  */
void control_lowSpeed_thread_entry(void *parameter)//低速控制线程
{

		rt_thread_mdelay(5000);//等待外部设备初始化成功
		
		while(1)
		{

				Light_Control(&ControlCmd.Light);  //探照灯控制
				Propeller_Control(); //推进器控制
			
				rt_thread_mdelay(30);
		}
}



int control_thread_init(void)
{
		rt_thread_t control_lowSpeed_tid;
		rt_thread_t control_highSpeed_tid;
		/*创建动态线程*/
    control_lowSpeed_tid = rt_thread_create("control_low",//线程名称
                    control_lowSpeed_thread_entry,				 //线程入口函数【entry】
                    RT_NULL,							   //线程入口函数参数【parameter】
                    2048,										 //线程栈大小，单位是字节【byte】
                    10,										 	 //线程优先级【priority】
                    10);										 //线程的时间片大小【tick】= 100ms

			/*创建动态线程*/
    control_highSpeed_tid = rt_thread_create("control_high",//线程名称
                    control_highSpeed_thread_entry,				 //线程入口函数【entry】
                    RT_NULL,							   //线程入口函数参数【parameter】
                    2048,										 //线程栈大小，单位是字节【byte】
                    10,										 	 //线程优先级【priority】
                    10);										 //线程的时间片大小【tick】= 100ms
	
    if (control_lowSpeed_tid != RT_NULL && control_highSpeed_tid != RT_NULL  ){
				rt_thread_startup(control_lowSpeed_tid);
				rt_thread_startup(control_highSpeed_tid);
				log_i("Control_Init()");
		}
		else {
				log_e("Control Error!");
		}
		return 0;
}
INIT_APP_EXPORT(control_thread_init);





void Angle_Control(void)
{
	
		if(Sensor.JY901.Euler.Yaw < 0) Yaw = (180+Sensor.JY901.Euler.Yaw) + 180;//角度补偿
		if(Sensor.JY901.Euler.Yaw > 0) Yaw = (float)Sensor.JY901.Euler.Yaw;            //角度补偿
		Total_Controller.Yaw_Angle_Control.Expect = (float)Yaw_Control;//偏航角速度环期望，直接来源于遥控器打杆量
		Total_Controller.Yaw_Angle_Control.FeedBack = (float)Yaw;//偏航角反馈
	
		PID_Control_Yaw(&Total_Controller.Yaw_Angle_Control);//偏航角度控制
	

		//偏航角速度环期望，来源于偏航角度控制器输出
		//Total_Controller.Yaw_Gyro_Control.Expect = Total_Controller.Yaw_Angle_Control.Control_OutPut;
}



void Depth_Control(void)
{
		
		Total_Controller.High_Position_Control.Expect = (float)Expect_Depth; //期望深度由遥控器给定
		Total_Controller.High_Position_Control.FeedBack = (float)Sensor.Depth;  //当前深度反馈
		PID_Control(&Total_Controller.High_Position_Control);//高度位置控制器
	
		robot_upDown(Total_Controller.High_Position_Control.Control_OutPut);		//竖直推进器控制
}



void Gyro_Control(void)//角速度环
{

//  	偏航角前馈控制
//  	Total_Controller.Yaw_Gyro_Control.FeedBack=Yaw_Gyro;


//		PID_Control_Div_LPF(&Total_Controller.Yaw_Gyro_Control);
//		Yaw_Gyro_Control_Expect_Delta=1000*(Total_Controller.Yaw_Gyro_Control.Expect-Last_Yaw_Gyro_Control_Expect)
//			/Total_Controller.Yaw_Gyro_Control.PID_Controller_Dt.Time_Delta;
//		//**************************偏航角前馈控制**********************************
//		Total_Controller.Yaw_Gyro_Control.Control_OutPut+=Yaw_Feedforward_Kp*Total_Controller.Yaw_Gyro_Control.Expect
//			+Yaw_Feedforward_Kd*Yaw_Gyro_Control_Expect_Delta;//偏航角前馈控制
//		Total_Controller.Yaw_Gyro_Control.Control_OutPut=constrain_float(Total_Controller.Yaw_Gyro_Control.Control_OutPut,
//																																		 -Total_Controller.Yaw_Gyro_Control.Control_OutPut_Limit,
//																																		 Total_Controller.Yaw_Gyro_Control.Control_OutPut_Limit);
//		Last_Yaw_Gyro_Control_Expect=Total_Controller.Yaw_Gyro_Control.Expect;
//		

}

/*【深度 】期望yaw MSH方法 */
static int depth(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        rt_kprintf("Error! Proper Usage: RoboticArm_openvalue_set 1600");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1])<100){
				Expect_Depth = atoi(argv[1]);
		}
		else {
				log_e("Error! The  value is out of range!");
		}

		
_exit:
    return result;
}
MSH_CMD_EXPORT(depth,ag: depth 10);



/*【机械臂】舵机 期望yaw MSH方法 */
static int yaw(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        rt_kprintf("Error! Proper Usage: RoboticArm_openvalue_set 1600");
				result = -RT_ERROR;
        goto _exit;
    }
		Yaw_Control = atoi(argv[1]); //ASCII to Integer
		
_exit:
    return result;
}
MSH_CMD_EXPORT(yaw,ag: yaw 100);

