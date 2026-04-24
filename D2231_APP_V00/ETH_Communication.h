/*
 * ETH_Recv.h
 *
 *  Created on: 2025-12-23
 *      Author: ThinkPad
 */

#ifndef ETH_RECV_H_
#define ETH_RECV_H_

void Set_Socket_ID(int val);
int Get_Socket_ID();
void *RecvPC_thread();
void* Eth_Send_pyhread();

#endif /* ETH_RECV_H_ */
