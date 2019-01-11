#include <cstdlib>
#include <random>
#include <vector>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <pthread.h>
#include <ctime>
#include <iostream>
#include "board.h"
#include "move.h"

//#define PARALLEL_AB
#ifdef PARALLEL_AB
#include "AB_p.h"
#else
#include "AB.h"
#endif
#define forstate for(int i = 0 ; i < 16 ; i++) for(int j = 0 ; j < 4 ; j++) for(int k = 0 ; k < 4 ; k++)
std::default_random_engine engine;
std::uniform_int_distribution <int> dis(0,20);
std::vector<Move> record;
Index *Tuple[16][4][4];
int next[4][4] = {{12, 13, 14, 15}, {0, 4, 8, 12}, {0, 1, 2, 3}, {3, 7, 11, 15}};
int search_level, num_Tuple[16][4][4];
pthread_t thread[8];
float search(Board &board, int dir, int next_tile)
{
	int bag[3]={1,2,3},bag_index=0;
	struct AB_node search_base;
	search_base.board=board;
	for(int i=0;i<3;i++){
		search_base.bag[i]=bag[i];
	}
	search_base.bagit=bag_index;
	search_base.next=next_tile;
	search_base.last_move=dir;
	float tmp=ABsearch(NEGINF,POSINF,search_base,1,5-1);
#ifdef PARALLEL_AB
	if(debug_log) debug_log--;
#endif
	return tmp;
}
inline void Computer(Board &board, const int dir, int &next_tile, Move &p)
{
	static int bag[3] = {1, 2, 3}, bag_index;
	static int location[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	if(dir == 4)
	{
		board = Board();
		bag_index = 0;
		shuffle(bag, bag + 3, engine);
		shuffle(location, location + 16, engine);
		int cnt=0;
		while(cnt < 9)
		{
			board.take_place(location[cnt], bag[bag_index]);
			record.push_back(Move(location[cnt], bag[bag_index]));
			bag_index++;
			if(bag_index==3){
				shuffle(bag, bag + 3, engine);
				bag_index=0;
			}
			cnt++;
		}
		next_tile = bag[0];
		return ;
	}
	else
	{
		shuffle(next[dir], next[dir] + 4, engine);
		for(int i = 0 ; i < 4 ; i++) if(board.take_place(next[dir][i], next_tile)) {p = Move(next[dir][i], next_tile); break;}
		if(next_tile < 4) bag_index++;
		if(bag_index == 3)
		{
			bag_index = 0;
			shuffle(bag, bag + 3, engine);
		}
		next_tile = bag[bag_index];
	}
}
inline void Player(Board &board, const int next_tile, Move &p)
{
	int dir = -1;
	Board tmp;
	float res, maxvalue;
	for(int i = 0 ; i < 4 ; i++)
	{
		tmp = board;
		if(tmp.slide(i) != -1)
		{
			//decide the value
			res = search(tmp, i, next_tile);
			//compare the value
			if(dir == -1 || res > maxvalue)
			{
				dir = i;
				maxvalue = res;
			}
		}
	}
	if(dir == -1) {p = Move(); return ;}
	board.slide(dir);
	p = Move(dir);
	if(simu_check)
		simu_check--;
}
inline void input()
{
	unsigned num, index;
	unsigned long long cnt = 0;
	float value;
	std::ifstream fin("data", std::ios::binary);
	forstate
	{
		fin.read((char*)&num, sizeof(size_t));
		cnt += num;
		num_Tuple[i][j][k] = num;
		Tuple[i][j][k] = new Index[num];
		while(num--)
		{
			fin.read((char*)&index, sizeof(unsigned));
			fin.read((char*)&value, sizeof(float));
			Tuple[i][j][k][num].index = index;
			Tuple[i][j][k][num].value = value;
		}
	}
	fin.close();
	forstate std::sort(Tuple[i][j][k], Tuple[i][j][k] + num_Tuple[i][j][k]);
	printf("The number of Tuple : %llu\n", cnt);
}
time_t get_time()
{
	auto now = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}
int main(int n, char **argv)
{
	srand(time(NULL));
	//std::ios_base::sync_with_stdio(0);
	//std::cin.tie(0);
	int num_game = atoi(argv[1]), next_tile, dir;
	if(n > 2) search_level = atoi(argv[2]);
	else search_level = 0;
	engine.seed(time(NULL));
	//std::printf("Start Reading\n");
	//input();
	//std::printf("Reading Complete.\n");
	Board board;
	Move p;
	time_t time_start, time_end;
	int step_cnt;
	float average_step=0.0;
	float average_score=0.0;
	std::printf("Start Playing.\n");
	std::ofstream fout("result.txt");
	traverse=0;
	simus=0;
	for(int i = 0 ; i < num_game ; i++)
	{
		step_cnt=0;
		record.clear();
		Computer(board, 4, next_tile, p);
		time_start = get_time();
		while(true)
		{
			step_cnt++;
			Player(board, next_tile, p);
			if(p.slide() == -1) break;
			record.push_back(p);
			Computer(board, p.slide(), next_tile, p);
			record.push_back(p);
		}
		time_end = get_time();
		average_step+=(float)step_cnt/num_game;
		average_score+=(float)board.count_score()/num_game;
		fout << "0516310:N-Tuple@" << time_start << "|";
		for(int j = 0 ; j < record.size() ; j++) fout << record[j];
		fout << "|N-Tuple@" << time_end << "\n";
	}
	printf("Complete playing.\n");
	printf("Total nodes traversed: %llu\n",traverse);
	printf("Total simulation round elapsed: %llu\n",simus);
	printf("Average game length: %.3f\n",average_step);
	//printf("Average score: %.3f\n",average_score);
	return 0;
}
