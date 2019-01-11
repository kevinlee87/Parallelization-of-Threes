#ifndef _AB_P_H_
#define _AB_P_H_
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <pthread.h>

#include "board.h"

#define POSINF 999999.0
#define NEGINF -9999.0

#define USE_SERIAL 0

extern int next[4][4];

int debug_log=0;

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

struct AB_pack{
	struct AB_node node;
	int start,depth;
	float alpha,beta;
	float res;
	unsigned long long traverse;
	unsigned long long simus;
};

pthread_t workers[12];
struct AB_pack packs[12];
int ready[12];

float AB_F(float,float,struct AB_node &,int,struct AB_pack *);
float AB_G(float,float,struct AB_node &,int,struct AB_pack *);
void *run(void *);
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

float AB_F(float alpha,float beta,struct AB_node &info,int depth,struct AB_pack *pack=NULL)
{
	if(pack)
		pack->traverse+=1;
	if(depth==0){
		if(pack)
			pack->simus+=1;
		//return small_simulation(info,0,20);
		return simulation(info,0,100);
	}
	float m=alpha;
	int flag=1;
	struct AB_node next_node;
	for(int i=0;i<4;i++){
		next_node=info;
		if(next_node.board.slide(i)==-1)
			continue;
		flag=0;
		next_node.last_move=i;
		float t=AB_G(m,beta,next_node,depth-1,pack);
		if(t>m) m=t;
		if(m>=beta){
			return m;
		}
	}
	if(flag){
		//return info.board.get_tuple(info.next,0);
		return info.board.count_score();
	}else{
		return m;
	}
}

float AB_G(float alpha,float beta,struct AB_node &info,int depth,struct AB_pack *pack=NULL)
{
	if(pack)
		pack->traverse+=1;
	if(depth==0){
		if(pack)
			pack->simus+=1;
		//return small_simulation(info,1,20);
		return simulation(info,1,100);
	}
	float m=beta;
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
			float t=AB_F(alpha,m,next_node,depth-1,pack);
			if(t<m) m=t;
			if(m<=alpha){
				return m;
			}
		}
	}
	return m;
}

void *run(void *package_v){
	struct AB_pack *p=(struct AB_pack *)package_v;
	if(p->start==0){
		p->res=AB_F(p->alpha,p->beta,p->node,p->depth,p);
	}else{
		p->res=AB_G(p->alpha,p->beta,p->node,p->depth,p);
	}
	if(debug_log)
		std::cout<<"Debug from threads: result="<<p->res<<"\n\n";
	pthread_exit(0);
}

float AB_F_p(float alpha,float beta,struct AB_node &info,int depth){
	traverse+=1;
	if(depth==0){
		simus+=1;
		//return small_simulation(info,0,20);
		return simulation(info,0,100);
	}
	float m=alpha;
	int flag=1;
	int it,it2;
	struct AB_node next_node;
	for(it=0;it<4;it++){
		next_node=info;
		if(next_node.board.slide(it)==-1)
			continue;
		flag=0;
		next_node.last_move=it;
		break;
	}
	if(flag){
		//return info.board.get_tuple(info.next,0);
		return info.board.count_score();
	}else{
		float t=AB_G_p(m,beta,next_node,depth-1);
		if(t>m) m=t;
		if(m>=beta){
			return m;
		}
		for(it2=it+1;it2<4;it2++){
			ready[it2]=0;
			packs[it2].node=info;
			if(packs[it2].node.board.slide(it2)==-1)
				continue;
			packs[it2].node.last_move=it2;
			packs[it2].alpha=m;
			packs[it2].beta=beta;
			packs[it2].depth=depth-1;
			packs[it2].start=1;
			packs[it2].traverse=0;
			packs[it2].simus=0;
			ready[it2]=1;
		}
		for(it2=it+1;it2<4;it2++){
			if(ready[it2]){
				pthread_create(workers+it2,NULL,run,packs+it2);
			}
		}
		if(debug_log){
			char _;
			std::cin>>_;
			for(it2=it+1;it2<4;it2++){
				if(ready[it2]){
					float res;
					res=AB_G(packs[it2].alpha,packs[it2].beta,packs[it2].node,packs[it2].depth);
					std::cout<<"Debug from master: res="<<res<<"\n\n";
					std::cin>>_;
				}
			}
		}
		for(it2=it+1;it2<4;it2++){
			if(ready[it2]){
				pthread_join(workers[it2],NULL);
				t=packs[it2].res;
				if(t>m&&m<beta) m=t;
				traverse+=packs[it2].traverse;
				simus+=packs[it2].simus;
			}
		}
		return m;
	}
}

float AB_G_p(float alpha,float beta,struct AB_node &info,int depth){
	traverse+=1;
	if(depth==0){
		simus+=1;
		//return small_simulation(info,1,20);
		return simulation(info,1,100);
	}
	float m=beta;
	int it=0,it2;
	struct AB_node next_node;
	for(;it<12;it++){
		int i=it/3,j=it%3;
		next_node=info;
		if(!(next_node.board.take_place(next[info.last_move][i],info.next))){
			continue;
		}
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
		break;
	}
	float t=AB_F_p(alpha,m,next_node,depth-1);
	if(t<m) m=t;
	if(m<=alpha){
		return m;
	}
	for(it2=it+1;it2<12;it2++){
		ready[it2]=0;
		int i=it2/3,j=it2%3;
		packs[it2].node=info;
		if(!(packs[it2].node.board.take_place(next[info.last_move][i],info.next))){
			continue;
		}
		int bit;
		for(bit=info.bagit%3;bit<3;bit++){
			if(info.bag[bit]==j+1) break;
		}
		if(bit==3){
			continue;
		}
		packs[it2].node.next=j+1;
		packs[it2].node.bag[bit]=packs[it2].node.bag[info.bagit];
		packs[it2].node.bag[info.bagit]=j+1;
		packs[it2].node.bagit=info.bagit+1;
		packs[it2].node.bagit%=3;
		packs[it2].alpha=alpha;
		packs[it2].beta=m;
		packs[it2].depth=depth-1;
		packs[it2].start=0;
		packs[it2].traverse=0;
		packs[it2].simus=0;
		ready[it2]=1;
	}
	for(it2=it+1;it2<12;it2++){
		if(ready[it2])
			pthread_create(workers+it2,NULL,run,packs+it2);
	}
	if(debug_log){
		char _;
		std::cin>>_;
		for(it2=it+1;it2<12;it2++){
			if(ready[it2]){
				float res;
				res=AB_G(packs[it2].alpha,packs[it2].beta,packs[it2].node,packs[it2].depth);
				std::cout<<"Debug from master: res="<<res<<"\n\n";
				std::cin>>_;
			}
		}
	}
	for(it2=it+1;it2<12;it2++){
		if(ready[it2]){
			pthread_join(workers[it2],NULL);
			t=packs[it2].res;
			if(t<m&&m>alpha) m=t;
			traverse+=packs[it2].traverse;
			simus+=packs[it2].simus;
		}
	}
	return m;
}

float ABsearch(float alpha,float beta,struct AB_node init,int start,int depth)
{
	if(USE_SERIAL){
		if(start==0){
			return AB_F(alpha,beta,init,depth);
		}else{
			return AB_G(alpha,beta,init,depth);
		}
	}else{
		if(start==0){
			return AB_F_p(alpha,beta,init,depth);
		}else{
			return AB_G_p(alpha,beta,init,depth);
		}
	}
}

#endif