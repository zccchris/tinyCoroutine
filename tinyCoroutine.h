#pragma once

#define STACK_SIZE_MB 1         //Э�̹���ջ�Ĵ�С����MBΪ��λ
#define DEFAULT_MAX_COROUTINE 16

//Э�̵�������
class scheduler;
//Э����
class coroutine;


/***
 *  @brief �ڶ��д���һ��Э�̵�����
 *  @param maxCo ��Э�̵������ܹ�������Э����
 *  @return �����õ�Э�̵�����
***/
scheduler* openCo(int maxCo = DEFAULT_MAX_COROUTINE);

/***
 *  @brief �ر�һ��Э�̵�����
 *  @param sche �رյ�Э�̵�����
***/
void closeCo(scheduler* sche);

/***
 *  @brief ����һ��Э�̣���ע�ᵽ��Ӧ��Э�̵�������
 *  @param sche ��Э�����ڵ�Э�̵�����
 *  @param func ��Э�����еĺ���
 *  @param ud �����Ĳ���
 *  @return �����õ�Э��
***/
coroutine* newCo(scheduler* sche, coroutineFunction func, void* ud);

/***
 *  @brief ������������������е�Э��
 *  @param sche ��ص�����
***/
void yieldCo(scheduler* sche);

/***
 *  @brief �ָ��������е�i��Э��
 *  @param sche ��ص�����
 *  @param i �������е�i��Э��
***/
void resumeCo(scheduler* sche, int i);

