#include <cstdlib>
#include <random>
#include <vector>
#include <chrono>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include "mpi.h"
#include "board.h"
#include "move.h"
#define forstate for(int i = 0 ; i < 16 ; i++) for(int j = 0 ; j < 4 ; j++) for(int k = 0 ; k < 4 ; k++)
std::default_random_engine engine;
std::uniform_int_distribution <int> dis(0,3);
std::vector<Move> record;
int next[4][4] = {{12, 13, 14, 15}, {0, 4, 8, 12}, {0, 1, 2, 3}, {3, 7, 11, 15}};
int rank, size;
pthread_t thread[11];
class Node
{
public:
    Board board;
    unsigned long long score, time;
    int num_next, dir;
    Node **next, *parent;
    Node() 
    {
        score = time = 0;
        num_next = 0;
        parent = NULL;
        next = NULL;
        dir = 4;
    }
};
time_t get_time()
{
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}
Node* select(Node *s)
{
    Node *ret = NULL;
    double value;
    if(!s || !s->num_next) return s;
    for(int i = 0 ; i < s->num_next ; i++)
    {
        if(!s->next[i]) continue;
        if(!ret || s->next[i]->score / (double)s->next[i]->time > value)
        {
            ret = s->next[i];
            value = s->next[i]->score / (double)s->next[i]->time;
        }
    }
    return select(ret);
}
void expand(Node *s)
{
    Board board;
    if(s->dir == 4)
    {
        s->num_next = 4;
        s->next = new Node*[4];
        for(int i = 0 ; i < 4 ; i++) s->next[i] = NULL;
        for(int i = 0 ; i < 4 ; i++)
        {
            board = s->board;
            if(board.slide(i) == -1) continue;
            s->next[i] = new Node;
            s->next[i]->dir = i;
            s->next[i]->board = board;
            s->next[i]->parent = s;
        }
    }
    else
    {
        s->num_next = 12;
        s->next = new Node*[12];
        for(int i = 0 ; i < 12 ; i++) s->next[i] = NULL;
        for(int i = 0 ; i < 4 ; i++)
        {
            if(!s->board.take_place(next[s->dir][i], 0)) continue;
            for(int j = 1 ; j <= 3 ; j++)
            {
                s->next[i+j-1] = new Node;
                s->next[i+j-1]->board = s->board;
                s->next[i+j-1]->board.take_place(next[s->dir][i], j);
                s->next[i+j-1]->dir = 4;
                s->next[i+j-1]->parent = s;
            }   
        }
    }
}
unsigned long long run(Board board, int dir, bool bag[3], int rank)
{
    Board tmp;
    int move[4];
	//if(rank == 1)	std::printf("%d:slide:%d\n",rank,dir);
	//std::cout << board << "\n";
    if(dir == 4)
    {
        dir = -1;
        move[0] = 0;
        move[1] = 1;
        move[2] = 2;
        move[3] = 3;
        std::shuffle(move, move + 4, engine);
        for(int i = 0 ; i < 4 ; i++)
        {
            tmp = board;
            if(tmp.slide(move[i]) != -1) {dir = move[i]; break;}
        }
        if(dir == -1) return 100 * std::log(board.count_score());
        board.slide(dir);
        return run(board, dir, bag, rank);
    }
    else
    {
        if(!bag[0] && !bag[1] && !bag[2]) bag[0] = bag[1] = bag[2] = 1;
        int place = -1, s, num = 0;
        move[0] = next[dir][0];
        move[1] = next[dir][1];
        move[2] = next[dir][2];
        move[3] = next[dir][3];
        std::shuffle(move, move + 4, engine);
        for(int i = 0 ; i < 4 ; i++)
            if(board.take_place(move[i], 0)) {place = move[i]; break;}
        while(1)
        {
            s = dis(engine);
            if(!s) continue;
            if(!bag[s-1]) continue;
            bag[s-1] = 0;
            board.take_place(place, s);
            break;
        }
        return run(board, 4, bag, rank);
    }
}

void simulate(Node *s)
{
	unsigned long long part_result[30];
	Board tmp_board;
    bool bag[3];
	for(int i = 0 ; i < s->num_next ; i++)
	{
		if(s->next[i])
		{
			for(s->next[i]->time = 0 ; s->next[i]->time < 1000 ; s->next[i]->time++)
			{
				bag[0] = bag[1] = bag[2] = 1;
				s->next[i]->score += run(s->next[i]->board, s->next[i]->dir, bag, rank);
			}
		}
	}
}
void update(Node *s)
{
    s->score = s->time = 0;
    for(int i = 0 ; i < s->num_next ; i++)
    {
        if(!s->next[i]) continue;
        s->score += s->next[i]->score;
        s->time += s->next[i]->time;
    }
    if(s->parent) update(s->parent);
}
void del(Node *s)
{
    for(int i = 0 ; i < s->num_next ; i++) if(s->next[i]) del(s->next[i]);
    delete(s);
}
int MCTS(Board &board)
{
    int dir = -1;
    double value;
    Node *cur, *head = new Node;
    head->board = board;
    head->dir = 4;
    time_t start = get_time();
    while(get_time() - start < 1000)
    {
        cur = select(head);
		//std::printf("select finished\n");
        if(!cur) {std::cout << "NULL\n"; break;}
		expand(cur);
		//std::printf("expand finished\n");
        simulate(cur);
		//std::printf("simulation finished\n");
        update(cur);
		//std::printf("update finished\n");
    }
	for(int i = 0 ; i < 4 ; i++)
	{
		if(!head->next[i]) continue;
		if(dir == -1 || head->next[i]->score / (double)head->next[i]->time > value)
		{
			dir = i;
			value = head->next[i]->score / (double)head->next[i]->time;
		}
	}
    del(head);
	return dir;
}
void Computer(Board &board, const int dir, Move &p)
{
    static int bag[3] = {1, 2, 3}, bag_index;
    static int location[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    if(dir == 4)
    {
        bag_index = 0;
        shuffle(bag, bag + 3, engine);
        shuffle(location, location + 16, engine);
        for(int i = 0 ; i < 9 ; i++)
        {
            board.take_place(location[i], bag[i % 3]);
            record.push_back(Move(location[i], bag[i % 3]));
        }
        bag_index = 3;
    }
    else
    {
        if(bag_index == 3)
        {
            bag_index = 0;
            shuffle(bag, bag + 3, engine);
        }
        shuffle(next[dir], next[dir] + 4, engine);
        for(int i = 0 ; i < 4 ; i++) if(board.take_place(next[dir][i], bag[bag_index])) {p = Move(next[dir][i], bag[bag_index]); break;}
        bag_index++;
    }
}
void Player(Board &board, Move &p)
{
	MPI_Status status;
	int part_result, tag = 0;
	int dir_result[4];
    int dir = MCTS(board);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	if(size > 1){
		for(int i = 0;i < size;i++){
			if(i == rank)	continue;
			MPI_Send(&dir,1,MPI_INT,i,tag,MPI_COMM_WORLD);
		}
		for(int i = 0;i < 4;i++){
			dir_result[i] = 0;
		}
		for(int i = 0;i < size;i++){
			if(i == rank)	continue;
			MPI_Recv(&part_result,1,MPI_INT,i,tag,MPI_COMM_WORLD,&status);
			if(part_result == -1 && i == 0){
				dir = -1;
				break;
			}
			dir_result[part_result]++;
		}
		if(dir != -1){
			int max = 1;
			dir_result[dir] = 1;
			for(int i = 0;i < 4;i++){
				if(max < dir_result[i]){
					dir = i;
					max = dir_result[i];
				}
			}
		}
	}
    if(dir == -1) {
		p = Move();
		return ;
	}
	board.slide(dir);
    p = Move(dir);
}
/*inline void input()
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
}*/
int main(int n, char **argv)
{
    //std::ios_base::sync_with_stdio(0);
    //std::cin.tie(0);
    int num_game = atoi(argv[1]), dir;
    engine.seed(time(NULL));
    std::printf("Start Reading\n");
    //input();
    std::printf("Reading Complete.\n");
    Board board;
    Move p;
    time_t time_start, time_end;
    std::printf("Start Playing.\n");
    std::ofstream fout("result.txt");
	MPI_Init(NULL,NULL);
	rank = 0;
    for(int i = 0 ; i < num_game ; i++)
    {
        record.clear();
        board = Board();
        Computer(board, 4, p);
        time_start = get_time();
        while(true)
        {
            Player(board, p);
            if(rank == 0)	std::cout << board << "\n";
            if(p.slide() == -1) break;
			record.push_back(p);
			Computer(board, p.slide(), p);
			record.push_back(p);
        }
        time_end = get_time();
        if(rank == 0){
			fout << "0516310:N-Tuple@" << time_start << "|";
			for(int j = 0 ; j < record.size() ; j++) fout << record[j];
			fout << "|N-Tuple@" << time_end << "\n";
		}
    }
    printf("Complete playing.\n");
	MPI_Finalize();
    return 0;
}
