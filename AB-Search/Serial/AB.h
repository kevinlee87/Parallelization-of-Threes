#ifndef _AB_H_
#define _AB_H_

#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <utility>

#include "board.h"

#define POSINF 999999.0
#define NEGINF -9999.0

#define SIMULATE_P 0

extern int next[4][4];

unsigned long long traverse;
unsigned long long simus;

struct AB_node{
	//int type;
	Board board;
	int bag[3];
	int bagit;
	int next;
	int last_move;
};

float AB_F(float,float,struct AB_node &,int);
float AB_G(float,float,struct AB_node &,int);

float AB_F_p(float,float,struct AB_node &,int);
float AB_G_p(float,float,struct AB_node &,int);

void c_shuffle(int *start,int *end){
	int length=(end-start)/sizeof(int);
	for(int i=length;i>=2;i++){
		int it=rand()%length;
		int tmp=*(start+i-1);
		*(start+i-1)=*(start+it);
		*(start+it)=tmp;
	}
}

int simu_check=4;

float simulation(struct AB_node &info,int start,int round){
	int dir[4]={0,1,2,3};
	float avgscore=0;
	int bagback[3];
	struct AB_node current;
	int flag;
	for(int imp=0;imp<round;imp++){
		current=info;
		c_shuffle(current.bag+current.bagit,current.bag+3);
		if(start) goto CMove;
		PMove:
			c_shuffle(dir,dir+4);
			flag=1;
			for(int i=0;i<4;i++){
				if(current.board.slide(dir[i])!=-1){
					flag=0;
					current.last_move=dir[i];
					break;
				}
			}
			if(flag){
				avgscore+=((float)current.board.count_score())/round;
				continue;
			}
		
		CMove:
			c_shuffle(dir,dir+4);
			for(int i=0;i<4;i++){
				if(current.board.take_place(next[current.last_move][dir[i]],current.bag[current.bagit]))
					break;
			}
			current.bagit++;
			if(current.bagit==3){
				current.bagit=0;
				c_shuffle(current.bag,current.bag+3);
			}
			current.next=current.bag[current.bagit];
			goto PMove;
	}
	return avgscore;
}

float small_simulation(struct AB_node &info,int start,int round){
	int dir[4]={0,1,2,3};
	float avgscore=0;
	int bagback[3];
	struct AB_node current;
	int flag;
	int steps;
	for(int imp=0;imp<round;imp++){
		current=info;
		steps=20;
		c_shuffle(current.bag+current.bagit,current.bag+3);
		if(start) goto CMove;
		PMove:
			c_shuffle(dir,dir+4);
			flag=1;
			for(int i=0;i<4;i++){
				if(current.board.slide(dir[i])!=-1){
					flag=0;
					current.last_move=dir[i];
					break;
				}
			}
			steps--;
			if(flag||steps<=0){
				//avgscore+=((float)current.board.count_score())/round;
				avgscore+=(float)current.board.get_tuple(0,0)/round;
				continue;
			}
		
		CMove:
			c_shuffle(dir,dir+4);
			for(int i=0;i<4;i++){
				if(current.board.take_place(next[current.last_move][dir[i]],current.bag[current.bagit]))
					break;
			}
			steps--;
			current.bagit++;
			if(current.bagit==3){
				current.bagit=0;
				c_shuffle(current.bag,current.bag+3);
			}
			current.next=current.bag[current.bagit];
			goto PMove;
	}
	return avgscore;
}

float AB_F(float alpha,float beta,struct AB_node &info,int depth)
{
	traverse++;
	if(depth==0){
		simus++;
		return small_simulation(info,0,20);
		//return simulation(info,0,20);
	}
	float m=NEGINF;
	int flag=1;
	struct AB_node next_node;
	for(int i=0;i<4;i++){
		next_node=info;
		if(next_node.board.slide(i)==-1)
			continue;
		flag=0;
		next_node.last_move=i;
		float t=AB_G(m,beta,next_node,depth-1);
		if(t>m) m=t;
		if(m>=beta){
			return m;
		}
	}
	if(flag){
		return info.board.get_tuple(info.next,0);
		//return info.board.count_score();
	}else{
		return m;
	}
}

float AB_G(float alpha,float beta,struct AB_node &info,int depth)
{
	traverse++;
	if(depth==0){
		simus++;
		return small_simulation(info,1,20);
		//return simulation(info,1,20);
	}
	float m=POSINF;
	struct AB_node next_node;
	for(int i=0;i<4;i++){
		next_node=info;
		if(!(next_node.board.take_place(next[info.last_move][i],info.next))){
			continue;
		}
		for(int j=0;j<3;j++){
			int bit;
			for(bit=info.bagit%3;bit<3;bit++){
				if(info.bag[bit]==j+1) break;
			}
			if(bit==3){
				continue;
			}
			next_node.next=j+1;
			next_node.bag[bit]=next_node.bag[info.bagit];
			next_node.bag[info.bagit]=j+1;
			next_node.bagit=info.bagit+1;
			next_node.bagit%=3;
			float t=AB_F(alpha,m,next_node,depth-1);
			if(t<m) m=t;
			if(m<=alpha){
				return m;
			}
		}
	}
	return m;
}

float AB_F_p(float alpha,float beta,struct AB_node &info,int depth)
{
	traverse++;
	if(depth==0){
		simus++;
		return small_simulation(info,0,20);
		//return simulation(info,0,20);
	}
	float m=NEGINF,mtmp;
	int flag=1,wflag=1;
	struct AB_node next_node;
	for(int i=0;i<4;i++){
		next_node=info;
		if(next_node.board.slide(i)==-1)
			continue;
		flag=0;
		next_node.last_move=i;
		if(wflag){
			float t=AB_G_p(m,beta,next_node,depth-1);
			if(t>m) m=t;
			if(m>=beta){
				return m;
			}
			mtmp=m;
			wflag=0;
		}else{
			float t=AB_G(mtmp,beta,next_node,depth-1);
			if(t>m) m=t;
		}
	}
	if(flag){
		return info.board.get_tuple(info.next,0);
		//return info.board.count_score();
	}else{
		return m;
	}
}

float AB_G_p(float alpha,float beta,struct AB_node &info,int depth)
{
	traverse++;
	if(depth==0){
		simus++;
		return small_simulation(info,1,20);
		//return simulation(info,1,20);
	}
	float m=POSINF,mtmp;
	int wflag=1;
	struct AB_node next_node;
	for(int i=0;i<4;i++){
		next_node=info;
		if(!(next_node.board.take_place(next[info.last_move][i],info.next))){
			continue;
		}
		for(int j=0;j<3;j++){
			int bit;
			for(bit=info.bagit%3;bit<3;bit++){
				if(info.bag[bit]==j+1) break;
			}
			if(bit==3){
				continue;
			}
			next_node.next=j+1;
			next_node.bag[bit]=next_node.bag[info.bagit];
			next_node.bag[info.bagit]=j+1;
			next_node.bagit=info.bagit+1;
			next_node.bagit%=3;
			if(wflag){
				float t=AB_F_p(alpha,m,next_node,depth-1);
				if(t<m) m=t;
				if(m<=alpha){
					return m;
				}
			}else{
				float t=AB_F(alpha,mtmp,next_node,depth-1);
				if(t<m) m=t;
			}
		}
	}
	return m;
}

float ABsearch(float alpha,float beta,struct AB_node init,int start,int depth)
{
	if(SIMULATE_P){
		if(start==0){
			return AB_F_p(alpha,beta,init,depth);
		}else{
			return AB_G_p(alpha,beta,init,depth);
		}
	}else{
		if(start==0){
			return AB_F(alpha,beta,init,depth);
		}else{
			return AB_G(alpha,beta,init,depth);
		}
	}
}

#endif