#include "init.h"
#include <rthw.h>
#include <string.h>
/*---------------------- Constant / Macro Definitions -----------------------*/

#define JY901_UART_NAME       "uart2"
#define DEBUG_UART_NAME       "uart3"

#define Query_JY901_data 0     /* "1"为调试查询  "0"为正常读取 */

#if Query_JY901_data
char recv_buffer[128]; 				//串口2接收数据缓冲变量,
unsigned char recv_data_p=0x00;  //  串口2接收数据指针
#endif

/*----------------------- Variable Declarations -----------------------------*/
/* ALL_init 事件控制块. */
extern struct rt_event init_event;

rt_device_t debug_uart_device;	

rt_device_t uart2_device;	
struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT; /* 配置参数 */
static struct rt_semaphore rx_sem;/* 用于接收消息的信号量 */

u8 gyroscope_save_array[5] 		={0xFF,0xAA,0x00,0x00,0x00};	 //0x00-设置保存  0x01-恢复出厂设置并保存
u8 gyroscope_package_array[5] ={0xFF,0xAA,0x02,0x1F,0x00};	 //设置回传的数据包【0x1F 0x00 为 <时间> <加速度> <角速度> <角度> <磁场>】
u8 gyroscope_rate_array[5] 		={0xFF,0xAA,0x03,0x06,0x00};	 //传输速率 0x05-5Hz  0x06-10Hz(默认)  0x07-20Hz
u8 gyroscope_led_array[5] 		={0xFF,0xAA,0x1B,0x00,0x00}; 	 //倒数第二位 0x00-开启LED  0x01-关闭LED   
u8 gyroscope_baud_array[5] 		={0xFF,0xAA,0x04,0x02,0x00}; 	 //


/*----------------------- Function Implement --------------------------------*/

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

static void gyroscope_thread_entry(void *parameter)
{
    unsigned char ch;

		while (1)
		{
				/* 从串口读取一个字节的数据，没有读取到则等待接收信号量 */
				while (rt_device_read(uart2_device, 0, &ch, 1) != 1)
				{
						/* 阻塞等待接收信号量，等到信号量后再次读取数据 */
						rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
				}
#if Query_JY901_data //在线调试 查询模式
				
				recv_buffer[recv_data_p] = ch;
				recv_data_p++;
				if(recv_data_p>127)recv_data_p = 0;
		
#else 
				CopeSerial2Data(ch); //正常传输模式

#endif

		}
	
}

/* 设置 九轴模块 保存配置 */
void gyroscope_save(void)
{
			rt_device_write(uart2_device, 0, gyroscope_save_array, 5);  //进入加速度校准
			LOG_H("JY901 Save successed!");
}
MSH_CMD_EXPORT(gyroscope_save,gyroscope_save);


/* 设置 九轴模块 加速度校准 */
void gyroscope_Acc_calibration_enter(void)
{
			u8 Acc_calibration_enter[5]={0xFF,0xAA,0x01,0x01,0x00};
			rt_device_write(uart2_device, 0, Acc_calibration_enter, 5);   //ON LED
			LOG_H("Acc_calibrationing... ");
			rt_thread_mdelay(500);
			LOG_H("calibration OK, Next -> [gyroscope_save]");
}
MSH_CMD_EXPORT(gyroscope_Acc_calibration_enter,gyroscope_Acc_calibration_enter);

/* 设置 九轴模块 磁场 校准 */
void gyroscope_Mag_calibration_enter(void)
{
			u8 Mag_calibration_enter[5]={0xFF,0xAA,0x01,0x02,0x00};
			rt_device_write(uart2_device, 0, Mag_calibration_enter, 5);   //进入磁场校准
			LOG_H("Mag_calibrationing... ");
			rt_thread_mdelay(2000);
			LOG_H("After completing the rotation of the three axes... ");
			LOG_H("Nest -> [gyroscope_Mag_calibration_exit] ");

}
MSH_CMD_EXPORT(gyroscope_Mag_calibration_enter,gyroscope_Mag_calibration_enter);


/* 退出 九轴模块 磁场校准 */
void gyroscope_Mag_calibration_exit(void)
{
			u8 Mag_calibration_exit[5]={0xFF,0xAA,0x01,0x00,0x00};       
			rt_device_write(uart2_device, 0, Mag_calibration_exit, 5);   //退出磁场校准
			rt_thread_mdelay(100);
			gyroscope_save();                                           //保配置
			LOG_H("Mag_calibration OK & Saved! ");
}
MSH_CMD_EXPORT(gyroscope_Mag_calibration_exit,gyroscope_Mag_calibration_exit);



/*  九轴模块  复位 */
void gyroscope_reset(void)
{
		gyroscope_save_array[3] = 0x01;
		rt_device_write(uart2_device, 0, gyroscope_save_array, 5);  //保存
		LOG_H("JY901 Reset!");
}
MSH_CMD_EXPORT(gyroscope_reset,gyroscope reset);


/* 开启 九轴模块 数据包 */
void gyroscope_package_open(void)
{
		gyroscope_save_array[3] = 0x00;
		rt_device_write(uart2_device, 0, gyroscope_package_array, 5);   //ON package 开启回传数据包
		rt_device_write(uart2_device, 0, gyroscope_save_array, 5);  //SAVE
		LOG_H("Open successed! JY901: 1.Time  2.Acc  3.Gyro  4.Angle  5.Mag OPEN!");
}
MSH_CMD_EXPORT(gyroscope_package_open,gyroscope package open);

/* 开启 九轴模块 LED */
static int gyroscope_led(int argc, char **argv)
{
	  int result = 0;
    if (argc != 2){
        LOG_E("Proper Usage: gyroscope_led on/off\n");
				result = -RT_ERROR;
        goto _exit;
    }

		if( !strcmp(argv[1],"on") ){
				gyroscope_led_array[3] = 0x00;
				LOG_H("Operation is successful! gyroscope_led on\n");
		}
		else if( !strncmp(argv[1],"off",3) ){
				gyroscope_led_array[3] = 0x01;
				LOG_H("Operation is successful! gyroscope_led off\n");
		}
		else {
				LOG_E("Error! Proper Usage: gyroscope_led on/off\n");goto _exit;
		}
		rt_device_write(uart2_device, 0, gyroscope_led_array, 5);   //ON LED
		rt_device_write(uart2_device, 0, gyroscope_save_array, 5);  //保存
		
_exit:
    return result;
}
MSH_CMD_EXPORT(gyroscope_led, gyroscope_led on/off);

/* 设置 九轴模块 波特率为9600 */
void gyroscope_baud_9600(void)
{
			rt_device_write(uart2_device, 0, gyroscope_baud_array, 5);   //ON LED
			rt_device_write(uart2_device, 0, gyroscope_save_array, 5);  //保存
			LOG_H("JY901 baud:9600 ");
}
MSH_CMD_EXPORT(gyroscope_baud_9600,Modify JY901 baud rate);



int uart_gyroscope(void)
{
	  rt_thread_t gyroscope_tid;
	  /* 查找系统中的串口设备 */
		uart2_device = rt_device_find(JY901_UART_NAME);
		debug_uart_device = rt_device_find(DEBUG_UART_NAME);
	
		LOG_I("gyroscope serial:  %s", uart2_device);
		LOG_I("debug serial:  %s", debug_uart_device);
		rt_device_open(debug_uart_device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX);
	
    if (uart2_device != RT_NULL){
			
					/* 以读写以及中断接打开串口设备 */
					rt_device_open(uart2_device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX);
					config.baud_rate = BAUD_RATE_9600;
					config.data_bits = DATA_BITS_8;
					config.stop_bits = STOP_BITS_1;
					config.parity = PARITY_NONE;
			
					/* 打开设备后才可修改串口配置参数 */
					rt_device_control(uart2_device, RT_DEVICE_CTRL_CONFIG, &config);
					rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
					/* 设置接收回调函数 */
					rt_device_set_rx_indicate(uart2_device, uart_input);
		}
    /* 创建 serial 线程 */
		gyroscope_tid = rt_thread_create("gyroscope",
																			gyroscope_thread_entry,
																			RT_NULL, 
																			1024, 
																			25,
																			10);
    /* 创建成功则启动线程 */
    if (gyroscope_tid != RT_NULL)
    {
        rt_thread_startup(gyroscope_tid);
				rt_event_send(&init_event, GYRO_EVENT); //发送事件  表示初始化完成
    }
		return 0;
}

INIT_APP_EXPORT(uart_gyroscope);





