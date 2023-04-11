#pragma once

#define STACK_SIZE_MB 1         //Э�̹���ջ�Ĵ�С����MBΪ��λ
#define DEFAULT_MAX_COROUTINE 16

//Э�̵�������
class scheduler;
//Э����
class coroutine;



scheduler* openCo(int maxCo = DEFAULT_MAX_COROUTINE);

void closeCo(scheduler* sche);

coroutine* newCo(coroutineFunction func, scheduler* sche, void* ud);

void yieldCo(scheduler* sche);

void resumeCo(scheduler* sche, int i);

