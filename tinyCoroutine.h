#pragma once

#define STACK_SIZE_MB 1         //协程共有栈的大小，以MB为单位
#define DEFAULT_MAX_COROUTINE 16

//协程调度器类
class scheduler;
//协程类
class coroutine;


/***
 *  @brief 在堆中创建一个协程调度器
 *  @param maxCo 该协程调度器能管理的最大协程数
 *  @return 创建好的协程调度器
***/
scheduler* openCo(int maxCo = DEFAULT_MAX_COROUTINE);

/***
 *  @brief 关闭一个协程调度器
 *  @param sche 关闭的协程调度器
***/
void closeCo(scheduler* sche);

/***
 *  @brief 创建一个协程，并注册到相应的协程调度器中
 *  @param sche 该协程属于的协程调度器
 *  @param func 该协程运行的函数
 *  @param ud 函数的参数
 *  @return 创建好的协程
***/
coroutine* newCo(scheduler* sche, coroutineFunction func, void* ud);

/***
 *  @brief 挂起调度器中正在运行的协程
 *  @param sche 相关调度器
***/
void yieldCo(scheduler* sche);

/***
 *  @brief 恢复调度器中的i号协程
 *  @param sche 相关调度器
 *  @param i 调度器中的i号协程
***/
void resumeCo(scheduler* sche, int i);

