#pragma once

#define STACK_SIZE_MB 1         //协程共有栈的大小，以MB为单位
#define DEFAULT_MAX_COROUTINE 16

//协程调度器类
class scheduler;
//协程类
class coroutine;



scheduler* openCo(int maxCo = DEFAULT_MAX_COROUTINE);

void closeCo(scheduler* sche);

coroutine* newCo(coroutineFunction func, scheduler* sche, void* ud);

void yieldCo(scheduler* sche);

void resumeCo(scheduler* sche, int i);

