#pragma once
static char HEX[] = "0123456789ABCDEF";
class Move
{
private:
	int location, num, slide_dir;
public:
	Move():location(-1), slide_dir(-1){}
	Move(int loc, int n):location(loc), num(n){}
	Move(int dir):slide_dir(dir), location(-1){} 
	int slide() {return slide_dir;}
	friend std::ostream& operator<<(std::ostream &out, const Move &mv)
	{
		if(mv.location == -1)
		{
			out << "#";
			switch(mv.slide_dir)
			{
				case 0:
					out << "U";
					break;
				case 1:
					out << "R";
					break;
				case 2:
					out << "D";
					break;
				case 3:
					out << "L";
					break;
				default:
					out << "Wrong Movement\n";
			}
		}
		else  out << HEX[mv.location] << HEX[mv.num];
		return out;
	}
};
