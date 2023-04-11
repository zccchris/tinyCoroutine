#include "tinyCoroutine.h"
#include "unistd.h"
#include "ucontext.h"
#include <vector>
#include <cassert>
#include <iostream>
#include <cstring>

#define DEAD 0
#define NEW 1
#define RUNNING 2
#define SUSPEND 3

#define STACK_SIZE (1024*1024)*STACK_SIZE_MB


using coroutineFunction = void(*)(scheduler*, void* ud);

class coroutine{
public:
    coroutine(coroutineFunction func, scheduler* sche, void* ud)
        : _func(func)
        , _sche(sche)
        , _ud(ud)
        , status(NEW)
        , st(nullptr){
    }

    ~coroutine(){
        free(_ud);
        free(st);
    }

    //�洢��ǰЭ��ʹ�õ�ջ��Ϣ
    void saveStack(char* base){
        char dummy = 0; //�ο��Ʒ�coroutine˼��
        this->stackSize = base - &dummy;
        assert(stackSize <= STACK_SIZE);
        free(this->st);
        this->st = (char*)malloc(stackSize);
        memcpy(this->st, &dummy, stackSize);
    }


    void coFunc(){
        this->_func(this->_sche, this->_ud);
    }


public:
    scheduler* _sche;       //Э�������ĵ�����
    coroutineFunction _func;//Э�̵ĺ���ָ��
    void* _ud;              //Э�̵ĺ�������
    ucontext_t _ctx;        //Э�̵���������Ϣ
    int status;             //Э�̵�״̬
    char* st;               //Э�̵�ջ
    ptrdiff_t stackSize;          //Э��ջ�Ĵ�С
};


class scheduler{
public:
    scheduler(int maxCo)
        : _maxCo(maxCo)
        , _curCo(0)
        , _running(-1){

        _co.resize(_maxCo, nullptr);
    }
    ~scheduler() = default;

public:
    ucontext_t _main;
    char _stack[STACK_SIZE];
    int _maxCo;
    int _curCo;
    int _running;
    std::vector<coroutine*> _co;
};

//����һ��Э�̵�����
scheduler* openCo(int maxCo = DEFAULT_MAX_COROUTINE){
    scheduler* sche = new scheduler(maxCo);
    return sche;
}

//ɾ��һ��Э�̵�����
void closeCo(scheduler* sche){
    for(auto co : sche->_co){
        if(co) delete co;
    }
    delete sche;
}

//����һ���µ�Э��
coroutine* newCo(coroutineFunction func, scheduler* sche, void* ud){
    if(sche->_curCo >= sche->_maxCo) {
        std::cout<<"maxCo is reached\n";
        assert(sche->_curCo < sche->_maxCo);
    }
    coroutine* co = new coroutine(func, sche, ud);

    //���sche��_co��ָ��null��coroutineָ��
    for(int i = 0; i < sche->_curCo; i++){
        if(sche->_co[i] == nullptr){
            sche->_co[i] = co;
            return co;
        }
    }
    //���sche��_co������coroutineָ�붼��Ϊnullptr
    sche->_co.push_back(co);
    return co;
}

//����һ��Э��
void yieldCo(scheduler* sche){
    int runningCoId = sche->_running;
    assert(runningCoId >= 0);
    coroutine* curCo = sche->_co[runningCoId];
    curCo->saveStack(sche->_stack + STACK_SIZE);
    curCo->status = SUSPEND;
    sche->_running = -1;
    //����swapcontext�л���mainִ��
    swapcontext(&(curCo->_ctx), &(sche->_main));
}

//��װЭ�̺��������й��̣�������ɺ�ر�Э��
void funcExecu(uint32_t low32, uint32_t high32){
    uintptr_t ptr = ((uintptr_t)low32 | (uintptr_t(high32)));
    scheduler* sche = (scheduler*) ptr;
    int runningID = sche->_running;
    coroutine* curCo = sche->_co[runningID];
    curCo->status = RUNNING;
    curCo->coFunc();
    //������ɺ������Э��
    sche->_co[runningID] = nullptr;
    delete curCo;
    return;

}
//�ָ�sche�������е�i��Э��
void resumeCo(scheduler* sche, int i){
    coroutine* curCo = sche->_co[i];
    if(!curCo) return;
    switch(curCo->status){
        //�մ���->����
        case NEW:
            sche->_running = i;
            curCo->status = RUNNING;
            uintptr_t ptr = (uintptr_t)sche;
            getcontext(&curCo->_ctx); //makecontextǰ�����ʼ��ucontext
            uintptr_t ptr = (uintptr_t)sche;
            makecontext(&curCo->_ctx, (void(*)()) funcExecu, (uint32_t)ptr, (uint32_t)ptr>>32);   //����curCo�̵߳ĵ��ú���
            curCo->_ctx.uc_stack.ss_sp = sche->_stack;  //
            curCo->_ctx.uc_stack.ss_size = STACK_SIZE;  //
            curCo->_ctx.uc_link = &sche->_main;
            swapcontext(&sche->_main, &curCo->_ctx); //��Ϊ��NEW����ǰ���е�Ϊmain

            break;
        //����->����
        case SUSPEND:
            memcpy(sche->_stack + STACK_SIZE - curCo->stackSize, curCo->st, curCo->stackSize);
            sche->_running = i;
            curCo->status = RUNNING;
            swapcontext(&sche->_main, &curCo->_ctx);
            break;
        default:
            assert(0);
    }
}
