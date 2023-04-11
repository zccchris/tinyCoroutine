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

    //存储当前协程使用的栈信息
    void saveStack(char* base){
        char dummy = 0; //参考云风coroutine思想
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
    scheduler* _sche;       //协程所属的调度器
    coroutineFunction _func;//协程的函数指针
    void* _ud;              //协程的函数参数
    ucontext_t _ctx;        //协程的上下文信息
    int status;             //协程的状态
    char* st;               //协程的栈
    ptrdiff_t stackSize;          //协程栈的大小
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

//创建一个协程调度器
scheduler* openCo(int maxCo = DEFAULT_MAX_COROUTINE){
    scheduler* sche = new scheduler(maxCo);
    return sche;
}

//删除一个协程调度器
void closeCo(scheduler* sche){
    for(auto co : sche->_co){
        if(co) delete co;
    }
    delete sche;
}

//创建一个新的协程
coroutine* newCo(coroutineFunction func, scheduler* sche, void* ud){
    if(sche->_curCo >= sche->_maxCo) {
        std::cout<<"maxCo is reached\n";
        assert(sche->_curCo < sche->_maxCo);
    }
    coroutine* co = new coroutine(func, sche, ud);

    //如果sche中_co有指向null的coroutine指针
    for(int i = 0; i < sche->_curCo; i++){
        if(sche->_co[i] == nullptr){
            sche->_co[i] = co;
            return co;
        }
    }
    //如果sche中_co中现有coroutine指针都不为nullptr
    sche->_co.push_back(co);
    return co;
}

//挂起一个协程
void yieldCo(scheduler* sche){
    int runningCoId = sche->_running;
    assert(runningCoId >= 0);
    coroutine* curCo = sche->_co[runningCoId];
    curCo->saveStack(sche->_stack + STACK_SIZE);
    curCo->status = SUSPEND;
    sche->_running = -1;
    //调用swapcontext切换到main执行
    swapcontext(&(curCo->_ctx), &(sche->_main));
}

//封装协程函数的运行过程，运行完成后关闭协程
void funcExecu(uint32_t low32, uint32_t high32){
    uintptr_t ptr = ((uintptr_t)low32 | (uintptr_t(high32)));
    scheduler* sche = (scheduler*) ptr;
    int runningID = sche->_running;
    coroutine* curCo = sche->_co[runningID];
    curCo->status = RUNNING;
    curCo->coFunc();
    //运行完成后，清理该协程
    sche->_co[runningID] = nullptr;
    delete curCo;
    return;

}
//恢复sche调度器中的i号协程
void resumeCo(scheduler* sche, int i){
    coroutine* curCo = sche->_co[i];
    if(!curCo) return;
    switch(curCo->status){
        //刚创建->运行
        case NEW:
            sche->_running = i;
            curCo->status = RUNNING;
            uintptr_t ptr = (uintptr_t)sche;
            getcontext(&curCo->_ctx); //makecontext前必须初始化ucontext
            uintptr_t ptr = (uintptr_t)sche;
            makecontext(&curCo->_ctx, (void(*)()) funcExecu, (uint32_t)ptr, (uint32_t)ptr>>32);   //设置curCo线程的调用函数
            curCo->_ctx.uc_stack.ss_sp = sche->_stack;  //
            curCo->_ctx.uc_stack.ss_size = STACK_SIZE;  //
            curCo->_ctx.uc_link = &sche->_main;
            swapcontext(&sche->_main, &curCo->_ctx); //因为是NEW，当前运行的为main

            break;
        //挂起->运行
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
