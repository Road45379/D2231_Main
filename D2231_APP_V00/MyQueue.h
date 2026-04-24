#ifndef MYQUEUE_H
#define MYQUEUE_H

#include <stdio.h>
#include <malloc.h>
#include <string.h>

typedef enum
{
    OK=0, //正确
    ERROR=1,   //出错
    QueueTRUE=2,  //为真
    FALSE=3   //为假
}status;

typedef int ElemType;   //宏定义队列的数据类型
#define MAX_SIZE 1024

typedef struct queue
{
 //   char *pBase;    //数组指针
    char pBase[MAX_SIZE][4096];
    ElemType front;      //队头索引
    ElemType rear;       //队尾索引
    int maxSize;    //当前分配的最大容量
}queue;

//创建空队列 queueCapacity-队列容量
status initQueue(volatile queue *PQueue);
//销毁队列
void destroyQueue(queue *PQueue);
//清空队列
void clearQueue(queue *PQueue);
//判断队列是否为空
status isEmpityQueue(volatile queue *PQueue);
//判断队列是否为满
status isFullQueue(queue *PQueue);
//获得队列长度
int getQueueLen(queue *PQueue);
//新元素入队 [先进先出原则：在队尾的位置插入] element-要插入元素
status enQueue(volatile queue *PQueue,char* element, int len);
//新元素出队,同时保存出队的元素 [先进先出原则：在队头的位置删除]
status deQueue(volatile queue *PQueue,char *pElement);
//遍历队列
void queueTraverse(volatile queue *PQueue);

#if 0
#define SaveLog
extern char LogBuf[0x4000];//16k
extern int indexs;
#endif

#endif //MYQUEUE_H
