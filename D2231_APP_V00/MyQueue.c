#include "Myqueue.h"
#include "ASCII_Transition.h"

#ifdef SaveLog
char LogBuf[0x4000];//16k
int indexs;
#endif
/* 队列: 只允许在表的一端(队尾rear)进行插入操作，而在另一端(队头front)进行删除操作的线性表
 * 插入操作简称为入队  删除操作简称为出队   队列具有先进先出的特点
 */

/*=====队列的入队、出队示意图========
 *
 *  出队 ----------------- 入队
 *   <--- a1,a2,a3,...,an <---
 *      -----------------
 *
 *================================*/

//初始化队列 queueCapacity-队列容量
status initQueue(volatile queue *PQueue)
{
    //给数组指针分配内存
   /* PQueue->pBase = (char *)malloc(queueCapacity);
    if(!PQueue->pBase)
    {
        printf("malloc error!\n");
        return ERROR;
    }*/

    PQueue->front = 0; //最开始创建时，队头索引为0
    PQueue->rear = 0; //最开始创建时，队尾索引为0
    PQueue->maxSize = MAX_SIZE;

    return OK;
}

//销毁队列
void destroyQueue(queue *PQueue)
{
    free(PQueue);  //释放队列数组指针指向的内存
    PQueue = NULL;    //队列数组指针重新指向NULL,避免成为野指针
}

//清空队列
void clearQueue(queue *PQueue)
{
    PQueue->front = 0; //队头索引清0
    PQueue->rear = 0; //队尾索引清0
}

//判断队列是否为空
status isEmpityQueue(volatile queue *PQueue)
{
    if( PQueue->front == PQueue->rear )  //队头==队尾，说明为空
        return QueueTRUE;

    return FALSE;
}

/*
 *在循环队列中,“队满”和“队空”的条件有可能是相同的，都是front==rear，
 *这种情况下，无法区别是“队满”还是“队空”。
 *针对这个问题，有3种可能的处理方法：
 *（1）另设一个标志以区别是“队满”还是“队空”。（即入队/出队前检查是否“队满”/“队空”）
 *（2）设一个计数器，此时甚至还可以省去一个指针。
 *（3）少用一个元素空间，即约定队头指针在队尾指针的下一位置时就作为“队满”的标志，
 *即“队满”条件为：(PQueue->rear+1)%MAX_SIZE == PQueue->front。
 *  【这里采用了第3种处理方法】
 */
//判断队列是否为满
status isFullQueue(queue *PQueue)
{
    if( (PQueue->rear+1)%PQueue->maxSize == PQueue->front )  //队列满
        return QueueTRUE;

    return FALSE;
}

//获得队列长度
int getQueueLen(queue *PQueue)
{
    //正常情况下，队列长度为队尾队头指针之差，但如果首尾指针跨容量最大值时，要%
    return (PQueue->rear - PQueue->front + PQueue->maxSize)%PQueue->maxSize;
}

//新元素入队 [先进先出原则：在队尾的位置插入] element-要插入元素
status enQueue(volatile queue *PQueue, char* element, int len)
{
	if(isFullQueue((queue *)PQueue)==QueueTRUE)
	{
		printf("The queue is full!\n");
#ifdef SaveLog
		indexs += sprintf(&LogBuf[indexs], "The queue is full!\n");
#endif
		return FALSE;
	}
	// printf("enQueue : %s", element);
	//向队列中添加新元素
	//PQueue->pBase[PQueue->rear][] = element;
	//strcpy((char *)PQueue->pBase[PQueue->rear], element);
	char lens[8];
	sprintf(lens, "%04X", len);
	memcpy((char *)PQueue->pBase[PQueue->rear], lens, 4);
	// *(int *)PQueue->pBase[PQueue->rear]=len;
	memcpy((char *)PQueue->pBase[PQueue->rear] + 4, element, len);
	PQueue->rear = (PQueue->rear+1) % PQueue->maxSize; //将rear赋予新的合适的值
	return QueueTRUE;
}

//新元素出队,同时保存出队的元素 [先进先出原则：在队头的位置删除]
status deQueue(volatile queue *PQueue,char *pElement)
{
    //如果队列为空,则返回false
    if(isEmpityQueue((queue *)PQueue)==QueueTRUE)
    {
       // printf("The queue is empty,deQueue error!\n");
        return FALSE;
    }
   // char lens[8] = "0";
  //  memcpy(lens, (char *)PQueue->pBase[PQueue->front], 4);
    int len = PackWord((char *)PQueue->pBase[PQueue->front]);
   // int len =*(int *)PQueue->pBase[PQueue->rear];
    memcpy(pElement, (char *)PQueue->pBase[PQueue->front] + 4, len);
    //pElement = PQueue->pBase[PQueue->front];       //先进先出
    //strcpy(pElement,(char *)PQueue->pBase[PQueue->front]);
    memset((char *)PQueue->pBase[PQueue->front], 0, sizeof(PQueue->pBase[PQueue->front]));
    PQueue->front = (PQueue->front+1) % PQueue->maxSize; //移到下一位置
   // printf("deQueue : %s", pElement);
    return QueueTRUE;
}

//遍历队列
void queueTraverse(volatile queue *PQueue)
{
    int i = PQueue->front;           //从头开始遍历
    while(i != PQueue->rear)     //如果没有到达rear位置，就循环
    {
        printf("%s", (PQueue->pBase[i]));
#ifdef SaveLog
		indexs += sprintf(&LogBuf[indexs], "%s", (PQueue->pBase[i]));
#endif
        i = (i+1) % PQueue->maxSize;              //移到下一位置
    }
}

//根据索引出队
int outQuene(volatile queue *PQueue, char *pElement, int index)
{
    //如果队列为空,则返回false
    if(isEmpityQueue((queue *)PQueue)==QueueTRUE)
    {
       // printf("The queue is empty,deQueue error!\n");
        return FALSE;
    }
    char lens[8] = "0";
    memcpy(lens, (char *)PQueue->pBase[index], 4);
    int len = PackWord(lens);
    memcpy(pElement, (char *)PQueue->pBase[index] + 4, len);
    while(index != PQueue->front)
    {
    	memset(lens, 0, sizeof(lens));
    	memcpy(lens, (char *)PQueue->pBase[index], 4);
    	memcpy((char *)PQueue->pBase[index], (char *)PQueue->pBase[index + 1], PackWord(lens));
    	PQueue->front = (PQueue->front + 1) % PQueue->maxSize;
    }
    return OK;
}



