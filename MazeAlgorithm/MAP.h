#include <iostream> //standard
#include <random> 
#include <windows.h> // Ŀ�� ����
#include <conio.h> // ����� �Է�(getch)
#include <stack>
#include <queue>
#include <algorithm> // reverse�Լ�
#include <stdlib.h>

#include <thread>
#include <time.h>
#include <chrono>

using namespace std;

const int MAPSIZE = 41; //Ȧ�� ���� ���, ���� ������ �ʹ� ���� ����ϹǷ� Ȯ�忡 ����.
const int DepthOfWater = 40; // ����ġ

static int SLEEPTIME = 10; // �˰��� ������ �ӵ��� �����Ѵ�(Ŭ���� ������)
static int Tile[MAPSIZE][MAPSIZE];

// ������ ���� ��
static bool PLAY = false;
static bool RH = false;
static bool BFS = false;
static bool DIJK = false;
static bool Astar = false;

// ���� ���� �ð�(�ð� ���⵵)
static std::chrono::nanoseconds rhTimeExc;
static std::chrono::nanoseconds bfsTimeExc;
static std::chrono::nanoseconds dijkTimeExc;
static std::chrono::nanoseconds astarTimeExc;

// Ž�� ���� �ð�
static double yourPlayTime = 0;
static double rhTime = 0;
static double bfsTime = 0;
static double dijkTime = 0;
static double astarTime = 0;

enum TileType // Ÿ�� Ÿ�Կ� ���� ��µ� ��ҵ� [Render �޼���� �Բ� ����]
{
	EMPTY = 0,
	WALL = 1,
	Water = 2,

	TilePLAY = 3,
	TileRH = 4,
	TileBFS = 5,
	TileDIJK = 6,
	TileAstar = 7,
	TileDEST = 8
};

enum Direction //Player ���⼳��
{
	UP = 0, //�ݽð�
	LEFT = 1,
	DOWN = 2,
	RIGHT = 3
};

static struct MyStruct //Player���� ���� ��ġ�� ����
{
	int PosY_PLAY = 0;
	int PosX_PLAY = 0;

	int PosY_RH = 0; //����ġ(������)
	int PosX_RH = 0;

	int PosY_BFS = 0; //����ġ(BFS)
	int PosX_BFS = 0;

	int PosY_DIJK = 0;
	int PosX_DIJK = 0; //����ġ(DIJK)

	int PosY_Astar = 0;
	int PosX_Astar = 0;

	int DestY = 0; //������
	int DestX = 0;
}ps;

static struct PQNode // Astar ���
{
	int F;
	int G;
	int Y;
	int X;
}pn;

struct compare // priority Queue �������� ����
{
	bool operator()(const PQNode& pn1, const PQNode& pn2) {
		return pn1.F > pn2.F;
	}
};

class Settings //Ŀ�� ���� Ŭ����
{
public:
	//Ŀ�� ����� �Լ�
	static void CursorView(char show)
	{
		HANDLE hConsole;
		CONSOLE_CURSOR_INFO ConsoleCursor;

		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		ConsoleCursor.bVisible = show;
		ConsoleCursor.dwSize = 1;

		SetConsoleCursorInfo(hConsole, &ConsoleCursor);
	}
	//Ŀ�� ��ġ �ʱ�ȭ �Լ�
	static void gotoxy(int x, int y)
	{
		COORD Cur;
		Cur.X = x;
		Cur.Y = y;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cur);
	}
};

class Board //�� ������ ���� Ŭ����
{
public:
	int size = MAPSIZE;
	int intervalOfWater = 10; // ����ġ ���� ������ ���� ���� Ȯ�� [1 / n]

	void GenerateByBinaryTree() // ����Ʈ��
	{
		for (int y = 0;y < size;y++)
		{
			for (int x = 0;x < size;x++)
			{
				if (x % 2 == 0 || y % 2 == 0)
					Tile[y][x] = WALL;
				else
					Tile[y][x] = EMPTY;
			}
		}

		srand((unsigned)time(NULL));
		for (int y = 0;y < size;y++)
		{
			for (int x = 0;x < size;x++)
			{
				if (x % 2 == 0 || y % 2 == 0) //��尡 �ƴ� ��ǥ�� �ǳʶڴ�.
					continue;

				if (x == size - 2 && y == size - 2) //������������ ���� ���� �ʴ´�.
					continue;

				if (y == size - 2) //���̵忡���� ����
				{
					Tile[y][x + 1] = EMPTY;
					continue;
				}

				if (x == size - 2) //���̵忡���� ����
				{
					Tile[y + 1][x] = EMPTY;
					continue;
				}

				if (rand() % 2 == 0)
				{
					Tile[y][x + 1] = EMPTY;
				}
				else
				{
					Tile[y + 1][x] = EMPTY;
				}
			}
		}
	}
	void GenerateBySideWinder() // ���̵� ���δ�
	{
		for (int y = 0;y < size;y++)
		{
			for (int x = 0;x < size;x++)
			{
				if (x % 2 == 0 || y % 2 == 0)
					Tile[y][x] = WALL;
				else
					Tile[y][x] = EMPTY;
			}
		}

		srand((unsigned)time(NULL));

		for (int y = 0;y < size;y++)
		{
			int count = 0;
			for (int x = 0;x < size;x++)
			{
				if (x % 2 == 0 || y % 2 == 0) //��尡 �ƴ� ��ǥ�� �ǳʶڴ�.
					continue;

				if (x == size - 2 && y == size - 2) //������������ ���� ���� �ʴ´�.
					continue;

				if (y == size - 2) //���̵忡���� ����
				{
					Tile[y][x + 1] = EMPTY;
					continue;
				}

				if (x == size - 2) //���̵忡���� ����
				{
					Tile[y + 1][x] = EMPTY;
					continue;
				}

				if (rand() % 2 == 0)
				{
					Tile[y][x + 1] = EMPTY;
					count++;
				}
				else // �� ��� ���� ���� ���� �� �ϳ��� ����
				{
					int randomIndex = rand() % (count + 1);
					Tile[y + 1][x - randomIndex * 2] = EMPTY;
					count = 0;
				}
			}
		}
	}
	void BackTracker() // DFS + �������� ���� ��� ����
	{
		int Nodes[MAPSIZE][MAPSIZE];
		stack<int> historyX;
		stack<int> historyY;

		int trackerX = 1; //�� �մ�ģ��
		int trackerY = 1;
		int counter = 0;
		//���
		for (int y = 0;y < size;y++)
		{
			for (int x = 0;x < size;x++)
			{
				if (x % 2 == 0 || y % 2 == 0)
				{
					Tile[y][x] = WALL;
					Nodes[y][x] = WALL;
				}
				else
				{
					Tile[y][x] = EMPTY;
					Nodes[y][x] = EMPTY;
					counter++;
				}
			}
		}

		srand((unsigned)time(NULL));
		//ù��° ��� �湮
		Nodes[trackerY][trackerX] = WALL;
		historyY.push(trackerY);
		historyX.push(trackerX);
		counter--;
		//��ձ�
		while (counter != 0)
		{
			vector<int> nextYBox;
			vector<int> nextXBox;

			if (trackerX + 2 >= 0 && trackerX + 2 < size)
			{
				if (Nodes[trackerY][trackerX + 2] == EMPTY)
				{
					nextXBox.push_back(trackerX + 2);
					nextYBox.push_back(trackerY);
				}
			}

			if (trackerY + 2 >= 0 && trackerY + 2 < size)
			{
				if (Nodes[trackerY + 2][trackerX] == EMPTY)
				{
					nextXBox.push_back(trackerX);
					nextYBox.push_back(trackerY + 2);
				}
			}

			if (trackerX - 2 >= 0 && trackerY - 2 < size)
			{
				if (Nodes[trackerY][trackerX - 2] == EMPTY)
				{
					nextXBox.push_back(trackerX - 2);
					nextYBox.push_back(trackerY);
				}
			}

			if (trackerY - 2 >= 0 && trackerY - 2 < size)
			{
				if (Nodes[trackerY - 2][trackerX] == EMPTY)
				{
					nextXBox.push_back(trackerX);
					nextYBox.push_back(trackerY - 2);
				}
			}

			if (nextXBox.size() == 0 && nextYBox.size() == 0)
			{
				//���� ���� ���ư�
				trackerX = historyX.top();
				historyX.pop();
				trackerY = historyY.top();
				historyY.pop();
			}

			if (nextXBox.size() != 0 && nextYBox.size() != 0)
			{

				int nextIndex = rand() % nextXBox.size();

				int roadMakerY = (trackerY + nextYBox.at(nextIndex)) / 2;
				int roadMakerX = (trackerX + nextXBox.at(nextIndex)) / 2;
				//�� �ձ�
				Tile[roadMakerY][roadMakerX] = EMPTY;

				//������ ���� �̵�
				trackerX = nextXBox.at(nextIndex);
				trackerY = nextYBox.at(nextIndex);

				//Ž���� ��� ��󿡼� ����
				Nodes[trackerY][trackerX] = WALL;
				historyX.push(trackerX);
				historyY.push(trackerY);
				counter--;
			}
		}
	}
	void GenerateByBinaryTreeWithWater() // ���� Ʈ�� + ����ġ
	{
		srand((unsigned)time(NULL));
		GenerateByBinaryTree(); // ���� ���� ����
		for (int y = 0;y < size;y++)
		{
			for (int x = 0;x < size;x++)
			{
				if (x == 0 || y == 0 || x == size - 1 || y == size - 1)
					continue; // �׵θ�
				else if (x == ps.PosX_PLAY && y == ps.PosY_PLAY)
					continue; // �����
				else if (x == ps.DestX && y == ps.DestY)
					continue; // ��¼�

				if (rand() % intervalOfWater == 0)
					Tile[y][x] = Water; //�� �� �������� Ȯ�� ������ ����ġ�ο�
			}
		}
	}
	void GenerateBySideWinderWithWater()
	{
		srand((unsigned)time(NULL));
		GenerateBySideWinder();
		for (int y = 0;y < size;y++)
		{
			for (int x = 0;x < size;x++)
			{
				if (x == 0 || y == 0 || x == size - 1 || y == size - 1)
					continue;
				else if (x == ps.PosX_PLAY && y == ps.PosY_PLAY)
					continue;
				else if (x == ps.DestX && y == ps.DestY)
					continue;

				if (rand() % intervalOfWater == 0)
					Tile[y][x] = Water;
			}
		}
	}
	void BackTrackerWithWater()
	{
		srand((unsigned)time(NULL));
		BackTracker();
		for (int y = 0;y < size;y++)
		{
			for (int x = 0;x < size;x++)
			{
				if (x == 0 || y == 0 || x == size - 1 || y == size - 1)
					continue;
				else if (x == ps.PosX_PLAY && y == ps.PosY_PLAY)
					continue;
				else if (x == ps.DestX && y == ps.DestY)
					continue;

				if (rand() % intervalOfWater == 0)
					Tile[y][x] = Water;
			}
		}
	}

	void Render()
	{
		for (int y = 0;y < size;y++)
		{
			for (int x = 0;x < size;x++)
			{
				cout << GetTile(Tile[y][x], y, x);
			}
			cout << endl;
		}
	}

	string GetTile(int type, int y, int x)
	{
		if (y == ps.DestY && x == ps.DestX)
		{
			type = TileDEST;
		}
		if (y == ps.PosY_PLAY && x == ps.PosX_PLAY)
		{
			type = TilePLAY;
		}
		if (y == ps.PosY_RH && x == ps.PosX_RH)
		{
			type = TileRH;
		}
		if (y == ps.PosY_BFS && x == ps.PosX_BFS)
		{
			type = TileBFS;
		}
		if (y == ps.PosY_DIJK && x == ps.PosX_DIJK)
		{
			type = TileDIJK;
		}
		if (y == ps.PosY_Astar && x == ps.PosX_Astar)
		{
			type = TileAstar;
		}

		switch (type)
		{
		case EMPTY:
			return "��";
		case WALL:
			return "��";
		case Water:
			return "��";
		case TilePLAY:
			return "��";
		case TileRH:
			return "��";
		case TileBFS:
			return "��";
		case TileDIJK:
			return "��";
		case TileAstar:
			return "��";
		case TileDEST:
			return "��";
		default:
			return "��";
		}
	}
};

class Player //�÷��̾� ���� Ŭ����
{
public:
	int size = MAPSIZE;

	int dir = LEFT; //���� ���� ����

	vector<int> pointsX; //���� ��� ���� �迭
	vector<int> pointsY;

	vector<int> pointsXForBFS; //���� ��� ���� �迭
	vector<int> pointsYForBFS;

	vector<int> pointsXForDIJK; //���� ��� ���� �迭
	vector<int> pointsYForDIJK;

	vector<int> pointsXForAstar; //���� ��� ���� �迭
	vector<int> pointsYForAstar;

public:
	void Initialize(int posY = 1, int posX = 1)
	{
		ps.PosY_PLAY = posY;
		ps.PosX_PLAY = posX;

		ps.PosY_RH = posY;
		ps.PosX_RH = posX;

		ps.PosY_BFS = posY;
		ps.PosX_BFS = posX;

		ps.PosY_DIJK = posY;
		ps.PosX_DIJK = posX;

		ps.PosY_Astar = posY;
		ps.PosX_Astar = posX;

		ps.DestY = size - 2;
		ps.DestX = size - 2;

		RightHand();
		BFS();
		Dijikstra();
		Astar();
	}

	void RightHand() // ������ ���� Ž��
	{
		std::chrono::system_clock::time_point StartTime = std::chrono::system_clock::now();

		int frontY[] = { -1, 0, 1, 0 };
		int frontX[] = { 0, -1, 0, 1 };
		int rightY[] = { 0, -1, 0, 1 };
		int rightX[] = { 1, 0, -1, 0 };

		pointsY.push_back(ps.PosY_RH);
		pointsX.push_back(ps.PosX_RH);

		while (ps.PosY_RH != ps.DestY || ps.PosX_RH != ps.DestX)
		{
			if (Tile[ps.PosY_RH + rightY[dir]][ps.PosX_RH + rightX[dir]] == EMPTY
				|| Tile[ps.PosY_RH + rightY[dir]][ps.PosX_RH + rightX[dir]] == Water)
			{
				dir = (dir - 1 + 4) % 4;

				ps.PosX_RH += frontX[dir];
				ps.PosY_RH += frontY[dir];
				pointsY.push_back(ps.PosY_RH);
				pointsX.push_back(ps.PosX_RH);
				if (Tile[ps.PosY_RH][ps.PosX_RH] == Water)
				{
					for (int i = 0;i < DepthOfWater;i++)
					{
						pointsY.push_back(ps.PosY_RH);
						pointsX.push_back(ps.PosX_RH);
					}
				}
			}
			else if (Tile[ps.PosY_RH + frontY[dir]][ps.PosX_RH + frontX[dir]] == EMPTY
				|| Tile[ps.PosY_RH + rightY[dir]][ps.PosX_RH + rightX[dir]] == Water)
			{
				ps.PosX_RH += frontX[dir];
				ps.PosY_RH += frontY[dir];
				pointsY.push_back(ps.PosY_RH);
				pointsX.push_back(ps.PosX_RH);
				if (Tile[ps.PosY_RH][ps.PosX_RH] == Water)
				{
					for (int i = 0;i < DepthOfWater;i++)
					{
						pointsY.push_back(ps.PosY_RH);
						pointsX.push_back(ps.PosX_RH);
					}
				}
			}
			else
			{
				dir = (dir + 1 + 4) % 4;
			}
		}
		ps.PosY_RH = 1;
		ps.PosX_RH = 1;
		std::chrono::system_clock::time_point EndTime = std::chrono::system_clock::now();
		rhTimeExc = EndTime - StartTime;
	}
	int UserMove() //����� ��Ʈ��
	{
		char c = _getch();

		switch (c)
		{
		case 'w': //��
			if (Tile[ps.PosY_PLAY - 1][ps.PosX_PLAY] == EMPTY)
				ps.PosY_PLAY -= 1;
			else if (Tile[ps.PosY_PLAY - 1][ps.PosX_PLAY] == Water)
			{
				for (int i = 0;i < DepthOfWater;i++)
					Sleep(SLEEPTIME);
				ps.PosY_PLAY -= 1;
			}
			break;
		case 's': //��
			if (Tile[ps.PosY_PLAY + 1][ps.PosX_PLAY] == EMPTY)
				ps.PosY_PLAY += 1;
			else if (Tile[ps.PosY_PLAY + 1][ps.PosX_PLAY] == Water)
			{
				for (int i = 0;i < DepthOfWater;i++)
					Sleep(SLEEPTIME);
				ps.PosY_PLAY += 1;
			}
			break;
		case 'a': //��
			if (Tile[ps.PosY_PLAY][ps.PosX_PLAY - 1] == EMPTY)
				ps.PosX_PLAY -= 1;
			else if (Tile[ps.PosY_PLAY][ps.PosX_PLAY - 1] == Water)
			{
				for (int i = 0;i < DepthOfWater;i++)
					Sleep(SLEEPTIME);
				ps.PosX_PLAY -= 1;
			}
			break;
		case 'd': //��
			if (Tile[ps.PosY_PLAY][ps.PosX_PLAY + 1] == EMPTY)
				ps.PosX_PLAY += 1;
			else if (Tile[ps.PosY_PLAY][ps.PosX_PLAY + 1] == Water)
			{
				for (int i = 0;i < DepthOfWater;i++)
					Sleep(SLEEPTIME);
				ps.PosX_PLAY += 1;
			}
			break;
		}

		if (ps.PosY_PLAY == ps.DestY && ps.PosX_PLAY == ps.DestX)
		{
			return 1234;
		}
	}
	void BFS()
		// Breadth First Search : �ʺ� �켱Ž��
	{
		std::chrono::system_clock::time_point StartTime = std::chrono::system_clock::now();

		int deltaY[4] = { -1,0,1,0 };
		int deltaX[4] = { 0,-1,0,1 };

		bool found[MAPSIZE][MAPSIZE] = { {0,}, };

		int parentY[MAPSIZE][MAPSIZE];
		int parentX[MAPSIZE][MAPSIZE];

		queue<int> qY;
		queue<int> qX;

		qY.push(ps.PosY_BFS);
		qX.push(ps.PosX_BFS);

		found[ps.PosY_BFS][ps.PosX_BFS] = true;

		parentY[ps.PosY_BFS][ps.PosX_BFS] = ps.PosY_BFS;
		parentX[ps.PosY_BFS][ps.PosX_BFS] = ps.PosX_BFS;

		while (qY.size() > 0)
		{
			int nowY = qY.front();
			qY.pop();
			int nowX = qX.front();
			qX.pop();

			for (int i = 0;i < 4;i++)
			{
				int nextY = nowY + deltaY[i];
				int nextX = nowX + deltaX[i];
				if (Tile[nextY][nextX] == WALL)
					continue;
				if (found[nextY][nextX])
					continue;

				qY.push(nextY);
				qX.push(nextX);
				found[nextY][nextX] = true;

				parentY[nextY][nextX] = nowY;
				parentX[nextY][nextX] = nowX;
			}
		}
		std::chrono::system_clock::time_point EndTime = std::chrono::system_clock::now();
		bfsTimeExc = EndTime - StartTime;

		int y = ps.DestY;
		int x = ps.DestX;
		while (parentY[y][x] != y || parentX[y][x] != x)
		{
			if (Tile[y][x] == Water)
			{
				for (int i = 0;i < DepthOfWater;i++)
				{
					pointsYForBFS.push_back(y);
					pointsXForBFS.push_back(x);
				}
			}
			else
			{
				pointsYForBFS.push_back(y);
				pointsXForBFS.push_back(x);
			}

			int posiY = parentY[y][x];
			int posiX = parentX[y][x];
			y = posiY;
			x = posiX;
		}

		pointsYForBFS.push_back(y);
		pointsXForBFS.push_back(x);

		reverse(pointsYForBFS.begin(), pointsYForBFS.end());
		reverse(pointsXForBFS.begin(), pointsXForBFS.end());
	}
	void Dijikstra()
		// Dijikstra : ����ġ�� ����Ͽ� ��� ��忡 ���� �ִܰŸ��� Ž��
		// ����� ���� ������, ������ Ȯ���ϴ�.
	{
		std::chrono::system_clock::time_point StartTime = std::chrono::system_clock::now();

		int deltaY[] = { -1,0,1,0 };
		int deltaX[] = { 0,-1,0,1 };

		bool visited[MAPSIZE][MAPSIZE];
		int distance[MAPSIZE][MAPSIZE];

		int parentY[MAPSIZE][MAPSIZE];
		int parentX[MAPSIZE][MAPSIZE];

		for (int i = 0;i < MAPSIZE;i++)
		{
			for (int j = 0;j < MAPSIZE;j++)
				distance[i][j] = INT_MAX;
		}

		distance[ps.PosY_DIJK][ps.PosX_DIJK] = 0;
		parentY[ps.PosY_DIJK][ps.PosX_DIJK] = ps.PosY_DIJK;
		parentX[ps.PosY_DIJK][ps.PosX_DIJK] = ps.PosX_DIJK;

		while (true)
		{
			int closest = INT_MAX;
			int nowY = -1, nowX = -1;
			for (int i = 0;i < MAPSIZE;i++)
			{
				for (int j = 0;j < MAPSIZE;j++)
				{
					if (visited[i][j] == true)
						continue;
					if (distance[i][j] == INT_MAX || distance[i][j] >= closest)
						continue;
					closest = distance[i][j];
					nowY = i;
					nowX = j;
				}
			}

			if (nowY == -1 && nowX == -1)
				break;

			visited[nowY][nowX] = true;

			for (int i = 0;i < 4;i++)
			{
				int nextY = nowY + deltaY[i];
				int nextX = nowX + deltaX[i];
				int nextDist = 0;

				if (Tile[nextY][nextX] == WALL)
					continue;
				else if (visited[nextY][nextX] == true)
					continue;

				if (Tile[nextY][nextX] == EMPTY)
				{
					nextDist = distance[nowY][nowX] + 1;
				}
				else if (Tile[nextY][nextX] == Water)
				{
					nextDist = distance[nowY][nowX] + DepthOfWater;
				}

				if (nextDist < distance[nextY][nextX])
				{

					distance[nextY][nextX] = nextDist;
					parentY[nextY][nextX] = nowY;
					parentX[nextY][nextX] = nowX;
				}
			}
		}
		std::chrono::system_clock::time_point EndTime = std::chrono::system_clock::now();
		dijkTimeExc = EndTime - StartTime;
		int y = ps.DestY;
		int x = ps.DestX;
		while (parentY[y][x] != y || parentX[y][x] != x)
		{
			if (Tile[y][x] == Water)
			{
				for (int i = 0;i < DepthOfWater;i++)
				{
					pointsYForDIJK.push_back(y);
					pointsXForDIJK.push_back(x);
				}
			}
			else
			{
				pointsYForDIJK.push_back(y);
				pointsXForDIJK.push_back(x);
			}

			int posiY = parentY[y][x];
			int posiX = parentX[y][x];
			y = posiY;
			x = posiX;
		}

		pointsYForDIJK.push_back(y);
		pointsXForDIJK.push_back(x);

		reverse(pointsYForDIJK.begin(), pointsYForDIJK.end());
		reverse(pointsXForDIJK.begin(), pointsXForDIJK.end());
	}
	void Astar()
	{
		// Astar �˰��� : ������� ����� ������ �켱 Ž��
		// ���� ���� ������� ȿ������ Ž���� ����.

		std::chrono::system_clock::time_point StartTime = std::chrono::system_clock::now();

		int deltaY[] = { -1,0,1,0 };
		int deltaX[] = { 0,-1,0,1 };
		int cost[] = { 10,10,10,10 };

		bool closed[MAPSIZE][MAPSIZE] = { {0,}, };
		int open[MAPSIZE][MAPSIZE];

		for (int y = 0;y < MAPSIZE;y++)
			for (int x = 0;x < MAPSIZE;x++)
				open[y][x] = INT_MAX;

		int parentY[MAPSIZE][MAPSIZE];
		int parentX[MAPSIZE][MAPSIZE];

		//���� �ű��
		//F = G + H
		//F = ��������(������������)
		//G = ���������� �ش� ��ǥ���� �̵��ϴµ� ��� ���
		//H = ���� �������� ���������� �󸶳� ������� : �޸���ƽ ������

		priority_queue<PQNode, vector<PQNode>, compare> pq;

		open[ps.PosY_Astar][ps.PosX_Astar]
			= 10 * (abs(ps.DestY - ps.PosY_Astar) + abs(ps.DestX - ps.PosX_Astar));

		PQNode pq1; //push pop�� O(logN)�̴�.
		pq1.F = open[ps.PosY_Astar][ps.PosX_Astar];
		pq1.G = 0;
		pq1.Y = ps.PosY_Astar;
		pq1.X = ps.PosX_Astar;

		pq.push(pq1);

		parentY[ps.PosY_Astar][ps.PosX_Astar] = ps.PosY_Astar;
		parentX[ps.PosY_Astar][ps.PosX_Astar] = ps.PosX_Astar;

		while (pq.size() > 0)
		{
			PQNode node = pq.top();
			pq.pop();

			if (closed[node.Y][node.X] == true)
				continue;

			closed[node.Y][node.X] = true;

			if (node.Y == ps.DestY && node.X == ps.DestX)
			{
				break;
			}

			for (int i = 0;i < sizeof(deltaY) / sizeof(deltaY[0]);i++)
			{
				int nextY = node.Y + deltaY[i];
				int nextX = node.X + deltaX[i];

				if (nextX < 0 || nextX >= size || nextY < 0 || nextY >= size)
					continue;
				if (Tile[nextY][nextX] == WALL)
					continue;
				if (closed[nextY][nextX])
					continue;

				int g = node.G + cost[i];

				if (Tile[nextY][nextX] == Water)
				{
					for (int x = 0;x < DepthOfWater - 1;x++)
						g += cost[i];
				}

				int h = 10 * (abs(ps.DestY - ps.PosY_Astar) + abs(ps.DestX - ps.PosX_Astar));

				if (open[nextY][nextX] < g + h)
					continue;

				open[nextY][nextX] = g + h;

				PQNode newPQ;
				newPQ.F = g + h;
				newPQ.G = g;
				newPQ.Y = nextY;
				newPQ.X = nextX;

				pq.push(newPQ);
				parentY[nextY][nextX] = node.Y;
				parentX[nextY][nextX] = node.X;
			}
		}
		std::chrono::system_clock::time_point EndTime = std::chrono::system_clock::now();
		astarTimeExc = EndTime - StartTime;
		int y = ps.DestY;
		int x = ps.DestX;
		while (parentY[y][x] != y || parentX[y][x] != x)
		{
			if (Tile[y][x] == Water)
			{
				for (int i = 0;i < DepthOfWater;i++)
				{
					pointsYForAstar.push_back(y);
					pointsXForAstar.push_back(x);
				}
			}
			else
			{
				pointsYForAstar.push_back(y);
				pointsXForAstar.push_back(x);
			}

			int posiY = parentY[y][x];
			int posiX = parentX[y][x];
			y = posiY;
			x = posiX;
		}

		pointsYForAstar.push_back(y);
		pointsXForAstar.push_back(x);

		reverse(pointsYForAstar.begin(), pointsYForAstar.end());
		reverse(pointsXForAstar.begin(), pointsXForAstar.end());
	}

public:
	unsigned int lastIndex = 0;
	unsigned int lastIndexForBFS = 0;
	unsigned int lastIndexForDIJK = 0;
	unsigned int lastIndexForAstar = 0;

	int Update() //����� ������
	{
		if (lastIndex >= pointsX.size() || lastIndex >= pointsY.size())
		{
			lastIndex = 0;
			pointsY.clear();
			pointsX.clear();
			return 1234;
		}
		ps.PosY_RH = pointsY[lastIndex];
		ps.PosX_RH = pointsX[lastIndex];
		lastIndex++;
		return 1;
	}

	int UpdateForBFS() //BFS ������
	{
		if (lastIndexForBFS >= pointsYForBFS.size() || lastIndexForBFS >= pointsXForBFS.size())
		{
			lastIndexForBFS = 0;
			pointsXForBFS.clear();
			pointsYForBFS.clear();
			return 1234;
		}
		ps.PosY_BFS = pointsYForBFS[lastIndexForBFS];
		ps.PosX_BFS = pointsXForBFS[lastIndexForBFS];
		lastIndexForBFS++;
		return 1;
	}

	int UpdateForDIJK() //���ͽ�Ʈ�� ������
	{
		if (lastIndexForDIJK >= pointsYForDIJK.size() || lastIndexForDIJK >= pointsXForDIJK.size())
		{
			lastIndexForDIJK = 0;
			pointsXForDIJK.clear();
			pointsYForDIJK.clear();
			return 1234;
		}
		ps.PosY_DIJK = pointsYForDIJK[lastIndexForDIJK];
		ps.PosX_DIJK = pointsXForDIJK[lastIndexForDIJK];
		lastIndexForDIJK++;
		return 1;
	}

	int UpdateForAstar() //A* ������
	{
		if (lastIndexForAstar >= pointsYForAstar.size() || lastIndexForAstar >= pointsXForAstar.size())
		{
			lastIndexForAstar = 0;
			pointsXForAstar.clear();
			pointsYForAstar.clear();
			return 1234;
		}
		ps.PosY_Astar = pointsYForAstar[lastIndexForAstar];
		ps.PosX_Astar = pointsXForAstar[lastIndexForAstar];
		lastIndexForAstar++;
		return 1;
	}
};

// ����(Execute)�� ���� ��ü ����
static Board board;
static Player player;

class Interface // �������̽��� ������ �����մϴ�.
{
public:
	static void Execute() //�������̽� ����
	{
		while (true)
		{
			int user_input;
			cout << "=========================================" << endl;
			cout << "	�÷��� �� ��带 �����ϼ���" << endl;
			cout << "=========================================" << endl;
			cout << "1. ���� ����" << endl;
			cout << "2. ���� ����" << endl;
			cout << "3. ����� �ı�" << endl;
			cout << "=========================================" << endl;
			rewind(stdin);
			scanf_s("%d", &user_input);

			if (user_input == 1) //���������� ���� �ڵ����� ������Ѵ�.
			{
				int boardData = -1;
				system("cls");
				cout << "=========================================" << endl;
				cout << "	���� ��� - ���ϴ� Ž�� �˰��� ����" << endl;
				cout << "=========================================" << endl;
				cout << "1. PLAYER" << endl;
				cout << "2. RIGHT HAND" << endl;
				cout << "3. BFS" << endl;
				cout << "4. DIJIKSTRA" << endl;
				cout << "5. Astar" << endl;
				cout << "6. ó������" << endl;

				rewind(stdin);
				scanf_s("%d", &user_input);
				if (user_input == 6)
				{
					system("cls");
					continue;
				}

				boardData = choiceMAP();
			rebuildPoint: // ���� ������ ���� goto��
				mapSwitcher(boardData);
				player.Initialize(1, 1); //�ʱ�ȭ

				Settings::gotoxy(0, 0);
				switch (user_input)
				{
				case 1: // �÷��̾�
					while (true)
					{
						Settings::CursorView(0); // Ŀ���� �Ⱥ��̰� ����
						board.Render(); //������
						if (player.UserMove() == 1234)
							goto rebuildPoint; // ������ ������ �ٽ� �ݺ�
						Settings::gotoxy(0, 0);
					}
					break;
				case 2: // ������
					while (true)
					{
						Settings::CursorView(0); // Ŀ���� �Ⱥ��̰� ����
						if (player.Update() == 1234) //���� �迭�� �־�� ���� ������
						{
							Settings::gotoxy(0, 0); //Ŀ�� ��ġ �ʱ�ȭ
							board.Render(); //������
							goto rebuildPoint; // ������ ������ �ٽ� �ݺ�
						}
						Settings::gotoxy(0, 0);
						board.Render(); // ������
						Sleep(SLEEPTIME); // ����ӵ� ����
					}
					break;
				case 3: //BFS
					while (true)
					{
						Settings::CursorView(0);
						if (player.UpdateForBFS() == 1234)
						{
							Settings::gotoxy(0, 0);
							board.Render();
							goto rebuildPoint;
						}
						Settings::gotoxy(0, 0);
						board.Render();
						Sleep(SLEEPTIME);
					}
					break;
				case 4:
					while (true)
					{
						Settings::CursorView(0);
						if (player.UpdateForDIJK() == 1234)
						{
							Settings::gotoxy(0, 0);
							board.Render();
							goto rebuildPoint;
						}
						Settings::gotoxy(0, 0);
						board.Render();
						Sleep(SLEEPTIME);
					}
					break;
				case 5: //Astar
					while (true)
					{
						Settings::CursorView(0);
						if (player.UpdateForAstar() == 1234)
						{
							Settings::gotoxy(0, 0);
							board.Render();
							goto rebuildPoint;
						}
						Settings::gotoxy(0, 0);
						board.Render();
						Sleep(SLEEPTIME);
					}
					break;
				default:
					break;
				}
			}
			else if (user_input == 2) // ���ϴ� �˰����� ������ ������ �ð�����
			{
				while (true)
				{
					system("cls");
					cout << "===================================================" << endl;
					cout << "	   ���� ���� - ���ϴ� �˰��� ����" << endl;
					cout << "===================================================" << endl;
					cout << "   ������ ���ϴ� �˰������ true�� ������ּ���." << endl;
					cout << "===================================================" << endl;
					printf("1. ���� : %s\n", PLAY ? "true" : "false");
					printf("2. RightHand : %s\n", RH ? "true" : "false");
					printf("3. BFS : %s\n", BFS ? "true" : "false");
					printf("4. Dijikstra : %s\n", DIJK ? "true" : "false");
					printf("5. Astar : %s\n", Astar ? "true" : "false");
					printf("6. ���ÿϷ�(True���� �˰��� ����)\n");
					printf("7. ó������\n");
					cout << "=========================================" << endl;
					rewind(stdin);
					scanf_s("%d", &user_input);

					switch (user_input) //�˰��� ���� ����
					{
					case 1:
						if (PLAY == false)
							PLAY = true;
						else
							PLAY = false;
						break;
					case 2:
						if (RH == false)
							RH = true;
						else
							RH = false;
						break;
					case 3:
						if (BFS == false)
							BFS = true;
						else
							BFS = false;
						break;
					case 4:
						if (DIJK == false)
							DIJK = true;
						else
							DIJK = false;
						break;
					case 5:
						if (Astar == false)
							Astar = true;
						else
							Astar = false;
						break;
					case 6:
						mapSwitcher(choiceMAP()); // �ʼ���
						player.Initialize(1, 1); // �÷��̾� ��ġ �ʱ�ȭ
						system("cls"); // ȭ�� �����
						extendThread(); // ������ Ȯ��,����,����� ���
						exit(1); // ���α׷� ����
						break;
					default:
						break;
					}

					if (user_input == 7) // ó������
					{
						system("cls");
						break;
					}
				}
			}
			else if (user_input == 3)
			{
				cout << "����� �ڷᱸ�� : �迭,���� �迭[vector],ť[queue],�켱���� ť[priorityQueue]" << endl;
				cout << endl;
				cout << "����� �˰��� : Ʈ��,DFS,BFS,DIJIKSTRA,A* ..." << endl;
				cout << endl;
				cout << "�Ѱ��� :" << endl;
				cout << "1. ��� ������ ���ÿ��� ���ư��� �Ͽ�, ������ ���� �����ʴ�.(���� �� ���¹��� ��)" << endl;
				cout << "2. ��� ��ҵ��� ��üȭ ���Ѽ� ����ϰ� ����� �;��µ�, �׷��� ���ߴ�." << endl;
				cout << "�����͸� �� �̿��ϸ� ������� ������" << endl;
				cout << endl;
				cout << "�ı� : " << endl;
				cout << "���ͽ�Ʈ��� Astar�ð����⵵ ���̸� ����;���." << endl;
				cout << "�� ����� �������� astar�� �� ������ Ŀ���� Ŀ������, �ξ� �� ������ ��������." << endl;
				cout << "���� 25 * 25�ΰ�� ���ͽ�Ʈ�� �� ��������, 41 * 41 ���� ������ �� ���ٴ°��� �� ���ִ�." << endl;
				cout << endl;
				cout << "Board Ŭ������ ����� intervalOfWater," << endl;
				cout << "����� ����� MAPSIZE, DepthOfWater�� ���� �����ϸ�,�� Ž�� �˰����� ������ Ȯ���� ���� �ٶ�." << endl;

				cout << "[�ƹ�Ű�� �����ּ���..]" << endl;
				_getch();
			}
			// �̻��� �� �����ϸ� continue
			system("cls");
		}
	}

	static int choiceMAP() //�� ���ý� ���������� ����
	{
		while (true)
		{
			int user_input;

			system("cls");
			cout << "==============================================" << endl;
			cout << "	���� ��� - ���ϴ� �� ���� �˰��� ����" << endl;
			cout << "==============================================" << endl;
			cout << "1. Binary Tree" << endl;
			cout << "2. SideWinder" << endl;
			cout << "3. BackTracker" << endl;
			cout << "4. Binary Tree With Water" << endl;
			cout << "5. SideWinder With Water" << endl;
			cout << "6. BackTracker With Water" << endl;

			rewind(stdin);
			scanf_s("%d", &user_input);

			switch (user_input)
			{
			case 1:
				return 1;
			case 2:
				return 2;
			case 3:
				return 3;
			case 4:
				return 4;
			case 5:
				return 5;
			case 6:
				return 6;
			default:
				break;
			}
		}
	}

	static void mapSwitcher(int userInput) //choiceMap���� ������ ���� �ֱ�
	{
		switch (userInput)
		{
		case 1:
			board.GenerateByBinaryTree();
			break;
		case 2:
			board.GenerateBySideWinder();
			break;
		case 3:
			board.BackTracker();
			break;
		case 4:
			board.GenerateByBinaryTreeWithWater();
			break;
		case 5:
			board.GenerateBySideWinderWithWater();
			break;
		case 6:
			board.BackTrackerWithWater();
			break;
		default:
			break;
		}
	}

	static void threadRender() // ��� ��������� �������� �����Ѵ�
	{
		while (true)
		{
			Settings::CursorView(0);
			Settings::gotoxy(0, 0);
			board.Render();
			if (PLAY == false && RH == false && BFS == false && DIJK == false && Astar == false)
				//��� ������ ������ ��ġ�� ����
				break;
		}
	}

	static void threadRightHand() //rightHand
	{
		clock_t start = clock();
		while (true)
		{
			if (player.Update() == 1234)
			{
				RH = false;
				break;
			}
			Sleep(SLEEPTIME);
		}
		clock_t end = clock();
		rhTime = double(end - start) / CLOCKS_PER_SEC;
	}

	static void threadUserMove() //userMove
	{
		clock_t start = clock();
		while (true)
		{
			if (player.UserMove() == 1234)
			{
				PLAY = false;
				break;
			}
		}
		clock_t end = clock();
		yourPlayTime = double(end - start) / CLOCKS_PER_SEC;
	}

	static void threadBFS() //BFS
	{
		clock_t start = clock(); //�ð�����
		while (true)
		{
			if (player.UpdateForBFS() == 1234) //������ 1234 ����
			{
				BFS = false;
				break;
			}
			Sleep(SLEEPTIME);
		}
		clock_t end = clock();
		bfsTime = double(end - start) / CLOCKS_PER_SEC;
	}

	static void threadDIJK() //DIJK
	{
		clock_t start = clock();
		while (true)
		{
			if (player.UpdateForDIJK() == 1234)
			{
				DIJK = false;
				break;
			}
			Sleep(SLEEPTIME);
		}
		clock_t end = clock();
		dijkTime = double(end - start) / CLOCKS_PER_SEC;
	}

	static void threadAstar() //Astar
	{
		clock_t start = clock();
		while (true)
		{
			if (player.UpdateForAstar() == 1234)
			{
				Astar = false;
				break;
			}
			Sleep(SLEEPTIME);
		}
		clock_t end = clock();
		astarTime = double(end - start) / CLOCKS_PER_SEC;
	}

	static void extendThread()
	{
		vector<thread> threadList; //������ ����

		if (true) // true���� ���� ������ Ȯ�� �� ����
		{
			threadList.emplace_back(&threadRender); // ������ Ȯ��
		}
		if (RH == true)
		{
			threadList.emplace_back(&threadRightHand);
		}
		if (PLAY == true)
		{
			threadList.emplace_back(&threadUserMove);
		}
		if (BFS == true)
		{
			threadList.emplace_back(&threadBFS);
		}
		if (DIJK == true)
		{
			threadList.emplace_back(&threadDIJK);
		}
		if (Astar == true)
		{
			threadList.emplace_back(&threadAstar);
		}
		/*���⿡ �˰��� ��� �߰� �ϸ�˴ϴ�*/

		for (auto& thread : threadList) //�����尡 ������ �˾Ƽ� ����
		{
			thread.join();
		}

		Settings::gotoxy(0, MAPSIZE + 1);
		cout << "===========================================================" << endl;
		printf("          [��ã�� ���� �ð�]\n");
		cout << "===========================================================" << endl;
		printf("%20s : %7.3lf second\n", "�������� ����ð�", yourPlayTime);
		printf("%20s : %7.3lf second\n", "RightHand ����ð�", rhTime);
		printf("%20s : %7.3lf second\n", "BFS ����ð�", bfsTime);
		printf("%20s : %7.3lf second\n", "DIJKSTRA ����ð�", dijkTime);
		printf("%20s : %7.3lf second\n", "Astar ����ð�", astarTime);
		cout << "===========================================================" << endl;
		printf("          [�˰��� ���� �ð�]\n");
		cout << "===========================================================" << endl;
		printf("%20s : \t", "RightHand ����ð�");
		cout << rhTimeExc.count() << " ns" << endl;
		printf("%20s : \t", "BFS ����ð�");
		cout << bfsTimeExc.count() << " ns" << endl;
		printf("%20s : \t", "DIJKSTRA ����ð�");
		cout << dijkTimeExc.count() << " ns" << endl;
		printf("%20s : \t", "Astar ����ð�");
		cout << astarTimeExc.count() << " ns" << endl;
		cout << "===========================================================" << endl;
	}
};