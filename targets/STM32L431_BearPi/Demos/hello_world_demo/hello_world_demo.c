/*----------------------------------------------------------------------------
 * Copyright (c) <2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/
/**
 *  DATE                AUTHOR      INSTRUCTION
 *  2019-07-23 10:00    yuhengP    The first version  
 *
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <osal.h>



/*===============================================================================
Copyright:
Version:
Author:
Date: 2017/11/3
Description:
    函数功能是将接收固定长度的字符串，并将接收后的字符串通过串口发送出去
    通过滴答定时器方式获取数据
revise Description:
===============================================================================*/
#include "usart.h"
#include "stm32l4xx.h"


#include <stdint.h>
#include <stddef.h>
#include <osal.h>
#include <link_misc.h>
#include <driver.h>
#include "sys/fcntl.h"
#include <iot_link_config.h>

#define USART3_TIMEOUT_Setting 800  //(ms)

u8 USART3_RX_BUF[30];
u16 USART3_RX_CNT=0;
u16 USART3_RX_TIMEOUT=0;       //接收状态标记
u8 my_string[37] = {0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x01,0x01,0x02,0x5A,0x23};
void Timer1CountInitial(void);

void USART3_Init(u32 baud)
{
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;    //声明一个结构体变量，用来初始化GPIO
    //使能串口的RCC时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE); //使能UART3所在GPIOB的时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    //串口使用的GPIO口配置
    // Configure USART3 Rx (PB.11) as input floating
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure USART3 Tx (PB.10) as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //配置串口
    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;


    // Configure USART3
    USART_Init(USART3, &USART_InitStructure);//配置串口3
    // Enable USART3 Receive interrupts 使能串口接收中断
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    // Enable the USART3
    USART_Cmd(USART3, ENABLE);//使能串口3

    //串口中断配置
    //Configure the NVIC Preemption Priority Bits
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // Enable the USART3 Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;        //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

}

void USART3_Sned_Char(u8 temp)
{
    USART_SendData(USART3,(u8)temp);
    while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);

}

void USART3_Sned_Char_Buff(u8 buf[],u32 len)
{
    u32 i;
    for(i=0;i<len;i++)
    USART3_Sned_Char(buf[i]);

}


void USART3_IRQHandler(void)                    //串口3中断服务程序
{
    u8 Res;
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        USART3_RX_TIMEOUT=0;
        USART3_RX_BUF[USART3_RX_CNT++] = USART_ReceiveData(USART3);    //读取接收到的数据
        printf(USART_ReceiveData(USART3));
    }
    //溢出-如果发生溢出需要先读SR,再读DR寄存器则可清除不断入中断的问题
    if(USART_GetFlagStatus(USART3,USART_FLAG_ORE) == SET)
    {
        USART_ReceiveData(USART3);
        USART_ClearFlag(USART3,USART_FLAG_ORE);
    }
    USART_ClearITPendingBit(USART3, USART_IT_RXNE);

}

//放到主函数的初始化中初始化
void Timer1CountInitial(void)
{
    //定时=36000/72000x2=0.001s=1ms;
        TIM_TimeBaseInitTypeDef    TIM_TimeBaseStructure;
        ///////////////////////////////////////////////////////////////
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

        TIM_TimeBaseStructure.TIM_Period = 100-1;//自动重装值（此时改为10ms）
        TIM_TimeBaseStructure.TIM_Prescaler = 7200-1;//时钟预分频
        TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//向上计数
        TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;        //时钟分频1
        TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
        TIM_TimeBaseInit(TIM1,&TIM_TimeBaseStructure);

        TIM_ClearFlag(TIM1,TIM_FLAG_Update);
        TIM_ITConfig(TIM1,TIM_IT_Update,ENABLE);
        TIM_Cmd(TIM1, ENABLE);
}
void TIM1_UP_IRQHandler(void)
{
    //TIM_TimeBaseStructure.TIM_Period = 100-1;//自动重装值（此时改为10ms）
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)
    {
        if(USART3_RX_TIMEOUT<USART3_TIMEOUT_Setting)
                USART3_RX_TIMEOUT++;
    }
    TIM_ClearITPendingBit(TIM1,TIM_IT_Update);
}



void app_hello_world_entry(void)
{
    Timer1CountInitial();
    Usart3_Init(115200);//串口1波特率设置为9600
    while(1)
    {
        if(USART3_RX_TIMEOUT==USART3_TIMEOUT_Setting)
        {
            USART3_RX_TIMEOUT=0;
            USART3_Sned_Char_Buff( my_string,USART3_RX_CNT);//将接收到的数据发送出去
            USART3_RX_CNT=0;
        }

    }
}


int standard_app_demo_main()
{   
    osal_task_create("helloworld",app_hello_world_entry,NULL,0x400,NULL,2);
    return 0;
}





