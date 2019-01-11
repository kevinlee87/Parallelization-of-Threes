#ifndef _BOARD_H_
#define _BOARD_H_
#include <algorithm>
#include <iostream>
#include <cstring>
#include <functional>
#define for_i(x) for(int i = 0 ; i < (x) ; i++)
#define for_j(x) for(int j = 0 ; j < (x) ; j++)
static unsigned match[] = {3, 7, 11, 15, 2, 6, 10, 14, 1, 5, 9, 13, 0, 4, 8, 12};
class Index
{
public:
	unsigned index;
	float value;
	bool operator < (const Index &s) const {return index < s.index;}
	bool operator < (const unsigned &s) const {return index < s;}
};
extern Index *Tuple[16][4][4];
extern int num_Tuple[16][4][4];
class Board
{
private:
	unsigned board[16];
public:
	Board() {memset(board, 0, sizeof(board));}
	bool take_place(unsigned n, unsigned num) // put tile or return false
	{
		if(board[n]) return false;
		board[n] = num;
		return true;
	}
	unsigned maxtile()
	{
		unsigned ans = board[0];
		for_i(16) ans = std::max(ans, board[i]);
		return ans;
	}
	void rotate_clockwise()
	{
		unsigned temp[16];
		for_i(16) temp[i] = board[i];
		for_i(16) board[match[i]] = temp[i];
	}
	void rotate_counterclockwise()
	{
		unsigned temp[16];
		for_i(16) temp[i] = board[i];
		for_i(16) board[i] = temp[match[i]];
	}
	void reverse()
	{
		for_i(8) std::swap(board[i], board[15-i]);
	}
	void reflect()
	{
		for_i(8)
			if(i < 4) std::swap(board[i], board[i+12]);
			else std::swap(board[i], board[i+4]);
	}
	unsigned count_score()
	{
		unsigned score = 0;
		for_i(16) if(board[i] > 2) score += pow(3, board[i]-2);
		return score;
	}
	bool slide_left(unsigned &score)
	{
		bool change = 0;
		score = 0;
		for(int i = 0 ; i < 16 ; i += 4)
		{
			for(int j = 1 ; j < 4 ; j++)
			{
				if(!board[i+j]) continue;
				if(!board[i+j-1])
				{
					board[i+j-1] = board[i+j];
					board[i+j] = 0;
					change = 1;
				}
				else if(board[i+j] + board[i+j-1] == 3)
				{
					score++;
					board[i+j-1] = 3;
					board[i+j] = 0;
					change = 1;
				}
				else if(board[i+j] == board[i+j-1] && board[i+j] != 1 && board[i+j] != 2)
				{
					score += pow(3, board[i+j]-3);
					board[i+j-1]++;
					board[i+j] = 0;
					change = 1;
				}
			}
		}
		return change;
	}
	int slide(int dir)
	{
		bool canmove;
		unsigned score;
		if(!dir)
		{
			rotate_counterclockwise();
			canmove = slide_left(score);
			rotate_clockwise();
		}
		else if(dir == 1)
		{
			reverse();
			canmove = slide_left(score);
			reverse();
		}
		else if(dir == 2)
		{
			rotate_clockwise();
			canmove = slide_left(score);
			rotate_counterclockwise();
		}
		else if(dir == 3) canmove = slide_left(score);
		else return -1;
		if(canmove) return score;
		return -1;
	}
	void tuple_index(unsigned *index)
	{
		index[0] = (((((board[0] << 5) + board[1] << 5) + board[2] << 5) + board[3] << 5) + board[4] << 5) + board[5];
		index[1] = (((((board[4] << 5) + board[5] << 5) + board[6] << 5) + board[7] << 5) + board[8] << 5) + board[9];
		index[2] = (((((board[5] << 5) + board[6] << 5) + board[7] << 5) + board[9] << 5) + board[10] << 5) + board[11];
		index[3] = (((((board[9] << 5) + board[10] << 5) + board[11] << 5) + board[13] << 5) + board[14] << 5) + board[15];
	}
	float get_tuple(int next_tile,int dir)
	{
		//blank cnt
		int cnt=0;
		for(int i=0;i<16;i++){
			if(board[i]==0)
				cnt++;
		}
		return cnt;
	}
	friend std::ostream& operator<<(std::ostream &out, const Board board)
	{
		out << "-------\n";
		for_i(4)
		{
			for(int j = 0 ; j < 4 ; j++) out << board.board[(i<<2)+j] << " ";
			out << "\n-------\n";
		}
		out << "\n";
		return out;
	}
};

#endif