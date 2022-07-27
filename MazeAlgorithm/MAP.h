#include <iostream> //standard
#include <random> 
#include <windows.h> // 커서 관련
#include <conio.h> // 사용자 입력(getch)
#include <stack>
#include <queue>
#include <algorithm> // reverse함수
#include <stdlib.h>

#include <thread>
#include <time.h>
#include <chrono>

using namespace std;

const int MAPSIZE = 41; //홀수 값만 사용, 현재 스택을 너무 많이 사용하므로 확장에 주의.
const int DepthOfWater = 40; // 가중치

static int SLEEPTIME = 10; // 알고리즘 렌더링 속도를 조절한다(클수록 느리다)
static int Tile[MAPSIZE][MAPSIZE];

// 스레드 설정 값
static bool PLAY = false;
static bool RH = false;
static bool BFS = false;
static bool DIJK = false;
static bool Astar = false;

// 실행 측정 시간(시간 복잡도)
static std::chrono::nanoseconds rhTimeExc;
static std::chrono::nanoseconds bfsTimeExc;
static std::chrono::nanoseconds dijkTimeExc;
static std::chrono::nanoseconds astarTimeExc;

// 탐색 측정 시간
static double yourPlayTime = 0;
static double rhTime = 0;
static double bfsTime = 0;
static double dijkTime = 0;
static double astarTime = 0;

enum TileType // 타일 타입에 따라 출력될 요소들 [Render 메서드와 함께 동작]
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

enum Direction //Player 방향설정
{
	UP = 0, //반시계
	LEFT = 1,
	DOWN = 2,
	RIGHT = 3
};

static struct MyStruct //Player들의 현재 위치를 구현
{
	int PosY_PLAY = 0;
	int PosX_PLAY = 0;

	int PosY_RH = 0; //현위치(오른손)
	int PosX_RH = 0;

	int PosY_BFS = 0; //현위치(BFS)
	int PosX_BFS = 0;

	int PosY_DIJK = 0;
	int PosX_DIJK = 0; //현위치(DIJK)

	int PosY_Astar = 0;
	int PosX_Astar = 0;

	int DestY = 0; //목적지
	int DestX = 0;
}ps;

static struct PQNode // Astar 노드
{
	int F;
	int G;
	int Y;
	int X;
}pn;

struct compare // priority Queue 내림차순 정렬
{
	bool operator()(const PQNode& pn1, const PQNode& pn2) {
		return pn1.F > pn2.F;
	}
};

class Settings //커서 관련 클래스
{
public:
	//커서 숨기는 함수
	static void CursorView(char show)
	{
		HANDLE hConsole;
		CONSOLE_CURSOR_INFO ConsoleCursor;

		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		ConsoleCursor.bVisible = show;
		ConsoleCursor.dwSize = 1;

		SetConsoleCursorInfo(hConsole, &ConsoleCursor);
	}
	//커서 위치 초기화 함수
	static void gotoxy(int x, int y)
	{
		COORD Cur;
		Cur.X = x;
		Cur.Y = y;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cur);
	}
};

class Board //맵 렌더링 관련 클래스
{
public:
	int size = MAPSIZE;
	int intervalOfWater = 10; // 가중치 값이 설정된 물이 나올 확률 [1 / n]

	void GenerateByBinaryTree() // 이진트리
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
				if (x % 2 == 0 || y % 2 == 0) //노드가 아닌 좌표는 건너뛴다.
					continue;

				if (x == size - 2 && y == size - 2) //목적지에서는 길을 뚫지 않는다.
					continue;

				if (y == size - 2) //사이드에서는 일자
				{
					Tile[y][x + 1] = EMPTY;
					continue;
				}

				if (x == size - 2) //사이드에서는 일자
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
	void GenerateBySideWinder() // 사이드 와인더
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
				if (x % 2 == 0 || y % 2 == 0) //노드가 아닌 좌표는 건너뛴다.
					continue;

				if (x == size - 2 && y == size - 2) //목적지에서는 길을 뚫지 않는다.
					continue;

				if (y == size - 2) //사이드에서는 일자
				{
					Tile[y][x + 1] = EMPTY;
					continue;
				}

				if (x == size - 2) //사이드에서는 일자
				{
					Tile[y + 1][x] = EMPTY;
					continue;
				}

				if (rand() % 2 == 0)
				{
					Tile[y][x + 1] = EMPTY;
					count++;
				}
				else // 본 노드 포함 이전 노드들 중 하나를 선택
				{
					int randomIndex = rand() % (count + 1);
					Tile[y + 1][x - randomIndex * 2] = EMPTY;
					count = 0;
				}
			}
		}
	}
	void BackTracker() // DFS + 랜덤으로 뚫을 노드 선택
	{
		int Nodes[MAPSIZE][MAPSIZE];
		stack<int> historyX;
		stack<int> historyY;

		int trackerX = 1; //맵 뚫는친구
		int trackerY = 1;
		int counter = 0;
		//노드
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
		//첫번째 노드 방문
		Nodes[trackerY][trackerX] = WALL;
		historyY.push(trackerY);
		historyX.push(trackerX);
		counter--;
		//길뚫기
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
				//이전 노드로 돌아감
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
				//길 뚫기
				Tile[roadMakerY][roadMakerX] = EMPTY;

				//정해진 노드로 이동
				trackerX = nextXBox.at(nextIndex);
				trackerY = nextYBox.at(nextIndex);

				//탐색할 노드 대상에서 제외
				Nodes[trackerY][trackerX] = WALL;
				historyX.push(trackerX);
				historyY.push(trackerY);
				counter--;
			}
		}
	}
	void GenerateByBinaryTreeWithWater() // 이진 트리 + 가중치
	{
		srand((unsigned)time(NULL));
		GenerateByBinaryTree(); // 맵을 먼저 생성
		for (int y = 0;y < size;y++)
		{
			for (int x = 0;x < size;x++)
			{
				if (x == 0 || y == 0 || x == size - 1 || y == size - 1)
					continue; // 테두리
				else if (x == ps.PosX_PLAY && y == ps.PosY_PLAY)
					continue; // 출발점
				else if (x == ps.DestX && y == ps.DestY)
					continue; // 결승선

				if (rand() % intervalOfWater == 0)
					Tile[y][x] = Water; //그 외 나머지는 확률 적으로 가중치부여
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
			return "□";
		case WALL:
			return "▣";
		case Water:
			return "∬";
		case TilePLAY:
			return "ⓟ";
		case TileRH:
			return "ⓡ";
		case TileBFS:
			return "ⓑ";
		case TileDIJK:
			return "ⓓ";
		case TileAstar:
			return "ⓐ";
		case TileDEST:
			return "♬";
		default:
			return "□";
		}
	}
};

class Player //플레이어 관련 클래스
{
public:
	int size = MAPSIZE;

	int dir = LEFT; //현재 보는 방향

	vector<int> pointsX; //논리를 담는 동적 배열
	vector<int> pointsY;

	vector<int> pointsXForBFS; //논리를 담는 동적 배열
	vector<int> pointsYForBFS;

	vector<int> pointsXForDIJK; //논리를 담는 동적 배열
	vector<int> pointsYForDIJK;

	vector<int> pointsXForAstar; //논리를 담는 동적 배열
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

	void RightHand() // 오른손 집고 탐색
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
	int UserMove() //사용자 컨트롤
	{
		char c = _getch();

		switch (c)
		{
		case 'w': //상
			if (Tile[ps.PosY_PLAY - 1][ps.PosX_PLAY] == EMPTY)
				ps.PosY_PLAY -= 1;
			else if (Tile[ps.PosY_PLAY - 1][ps.PosX_PLAY] == Water)
			{
				for (int i = 0;i < DepthOfWater;i++)
					Sleep(SLEEPTIME);
				ps.PosY_PLAY -= 1;
			}
			break;
		case 's': //하
			if (Tile[ps.PosY_PLAY + 1][ps.PosX_PLAY] == EMPTY)
				ps.PosY_PLAY += 1;
			else if (Tile[ps.PosY_PLAY + 1][ps.PosX_PLAY] == Water)
			{
				for (int i = 0;i < DepthOfWater;i++)
					Sleep(SLEEPTIME);
				ps.PosY_PLAY += 1;
			}
			break;
		case 'a': //좌
			if (Tile[ps.PosY_PLAY][ps.PosX_PLAY - 1] == EMPTY)
				ps.PosX_PLAY -= 1;
			else if (Tile[ps.PosY_PLAY][ps.PosX_PLAY - 1] == Water)
			{
				for (int i = 0;i < DepthOfWater;i++)
					Sleep(SLEEPTIME);
				ps.PosX_PLAY -= 1;
			}
			break;
		case 'd': //우
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
		// Breadth First Search : 너비 우선탐색
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
		// Dijikstra : 가중치를 고려하여 모든 노드에 대해 최단거리를 탐색
		// 비용이 많이 들지만, 성능은 확실하다.
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
		// Astar 알고리즘 : 결승점과 가까운 방향을 우선 탐색
		// 비교적 적은 비용으로 효율적인 탐색이 가능.

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

		//점수 매기기
		//F = G + H
		//F = 최종점수(작을수록좋음)
		//G = 시작점에서 해당 좌표까지 이동하는데 드는 비용
		//H = 현재 지점에서 목적지에서 얼마나 가까운지 : 휴리스틱 추정값

		priority_queue<PQNode, vector<PQNode>, compare> pq;

		open[ps.PosY_Astar][ps.PosX_Astar]
			= 10 * (abs(ps.DestY - ps.PosY_Astar) + abs(ps.DestX - ps.PosX_Astar));

		PQNode pq1; //push pop이 O(logN)이다.
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

	int Update() //우향법 렌더링
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

	int UpdateForBFS() //BFS 렌더링
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

	int UpdateForDIJK() //다익스트라 렌더링
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

	int UpdateForAstar() //A* 렌더링
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

// 실행(Execute)을 위한 객체 선언
static Board board;
static Player player;

class Interface // 인터페이스와 동작을 구현합니다.
{
public:
	static void Execute() //인터페이스 동작
	{
		while (true)
		{
			int user_input;
			cout << "=========================================" << endl;
			cout << "	플레이 할 모드를 선택하세요" << endl;
			cout << "=========================================" << endl;
			cout << "1. 무한 실행" << endl;
			cout << "2. 성능 측정" << endl;
			cout << "3. 백업용 후기" << endl;
			cout << "=========================================" << endl;
			rewind(stdin);
			scanf_s("%d", &user_input);

			if (user_input == 1) //도착지점에 가면 자동으로 재실행한다.
			{
				int boardData = -1;
				system("cls");
				cout << "=========================================" << endl;
				cout << "	개인 모드 - 원하는 탐색 알고리즘 선택" << endl;
				cout << "=========================================" << endl;
				cout << "1. PLAYER" << endl;
				cout << "2. RIGHT HAND" << endl;
				cout << "3. BFS" << endl;
				cout << "4. DIJIKSTRA" << endl;
				cout << "5. Astar" << endl;
				cout << "6. 처음으로" << endl;

				rewind(stdin);
				scanf_s("%d", &user_input);
				if (user_input == 6)
				{
					system("cls");
					continue;
				}

				boardData = choiceMAP();
			rebuildPoint: // 무한 실행을 위한 goto문
				mapSwitcher(boardData);
				player.Initialize(1, 1); //초기화

				Settings::gotoxy(0, 0);
				switch (user_input)
				{
				case 1: // 플레이어
					while (true)
					{
						Settings::CursorView(0); // 커서를 안보이게 설정
						board.Render(); //렌더링
						if (player.UserMove() == 1234)
							goto rebuildPoint; // 목적지 도착시 다시 반복
						Settings::gotoxy(0, 0);
					}
					break;
				case 2: // 오른손
					while (true)
					{
						Settings::CursorView(0); // 커서를 안보이게 설정
						if (player.Update() == 1234) //동적 배열에 넣어둔 값을 꺼낸다
						{
							Settings::gotoxy(0, 0); //커서 위치 초기화
							board.Render(); //렌더링
							goto rebuildPoint; // 목적지 도착시 다시 반복
						}
						Settings::gotoxy(0, 0);
						board.Render(); // 랜더링
						Sleep(SLEEPTIME); // 실행속도 조절
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
			else if (user_input == 2) // 원하는 알고리즘을 스레드 돌려서 시간측정
			{
				while (true)
				{
					system("cls");
					cout << "===================================================" << endl;
					cout << "	   성능 측정 - 원하는 알고리즘 선택" << endl;
					cout << "===================================================" << endl;
					cout << "   동작을 원하는 알고리즘들을 true로 만들어주세요." << endl;
					cout << "===================================================" << endl;
					printf("1. 수동 : %s\n", PLAY ? "true" : "false");
					printf("2. RightHand : %s\n", RH ? "true" : "false");
					printf("3. BFS : %s\n", BFS ? "true" : "false");
					printf("4. Dijikstra : %s\n", DIJK ? "true" : "false");
					printf("5. Astar : %s\n", Astar ? "true" : "false");
					printf("6. 선택완료(True값의 알고리즘만 실행)\n");
					printf("7. 처음으로\n");
					cout << "=========================================" << endl;
					rewind(stdin);
					scanf_s("%d", &user_input);

					switch (user_input) //알고리즘 복수 선택
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
						mapSwitcher(choiceMAP()); // 맵선택
						player.Initialize(1, 1); // 플레이어 위치 초기화
						system("cls"); // 화면 지우기
						extendThread(); // 스레드 확장,실행,결과물 출력
						exit(1); // 프로그램 종료
						break;
					default:
						break;
					}

					if (user_input == 7) // 처음으로
					{
						system("cls");
						break;
					}
				}
			}
			else if (user_input == 3)
			{
				cout << "사용한 자료구조 : 배열,동적 배열[vector],큐[queue],우선순위 큐[priorityQueue]" << endl;
				cout << endl;
				cout << "사용한 알고리즘 : 트리,DFS,BFS,DIJIKSTRA,A* ..." << endl;
				cout << endl;
				cout << "한계점 :" << endl;
				cout << "1. 모든 동작이 스택에서 돌아가게 하여, 성능이 별로 좋지않다.(지금 힙 쓰는법을 모름)" << endl;
				cout << "2. 모든 요소들을 객체화 시켜서 깔끔하게 만들고 싶었는데, 그러질 못했다." << endl;
				cout << "포인터를 잘 이용하면 만들수는 있을듯" << endl;
				cout << endl;
				cout << "후기 : " << endl;
				cout << "다익스트라랑 Astar시간복잡도 차이를 보고싶었다." << endl;
				cout << "맵 사이즈가 작을때는 astar가 더 느린데 커지면 커질수록, 훨씬 더 성능이 좋아진다." << endl;
				cout << "실제 25 * 25인경우 다익스트라가 더 빠르지만, 41 * 41 찍어보면 성능이 더 좋다는것을 볼 수있다." << endl;
				cout << endl;
				cout << "Board 클래스에 선언된 intervalOfWater," << endl;
				cout << "상수로 선언된 MAPSIZE, DepthOfWater의 값을 조절하며,각 탐색 알고리즘의 성능을 확인해 보길 바람." << endl;

				cout << "[아무키나 눌러주세요..]" << endl;
				_getch();
			}
			// 이상한 값 선택하면 continue
			system("cls");
		}
	}

	static int choiceMAP() //맵 선택시 정수형으로 리턴
	{
		while (true)
		{
			int user_input;

			system("cls");
			cout << "==============================================" << endl;
			cout << "	개인 모드 - 원하는 맵 생성 알고리즘 선택" << endl;
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

	static void mapSwitcher(int userInput) //choiceMap에서 선택한 값을 넣기
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

	static void threadRender() // 모든 스레드들의 렌더링을 관리한다
	{
		while (true)
		{
			Settings::CursorView(0);
			Settings::gotoxy(0, 0);
			board.Render();
			if (PLAY == false && RH == false && BFS == false && DIJK == false && Astar == false)
				//모든 스레드 동작을 마치면 종료
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
		clock_t start = clock(); //시간측정
		while (true)
		{
			if (player.UpdateForBFS() == 1234) //도착시 1234 리턴
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
		vector<thread> threadList; //스레드 구현

		if (true) // true값에 따라 스레드 확장 및 실행
		{
			threadList.emplace_back(&threadRender); // 스레드 확장
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
		/*여기에 알고리즘 계속 추가 하면됩니다*/

		for (auto& thread : threadList) //스레드가 끝나면 알아서 리턴
		{
			thread.join();
		}

		Settings::gotoxy(0, MAPSIZE + 1);
		cout << "===========================================================" << endl;
		printf("          [길찾기 수행 시간]\n");
		cout << "===========================================================" << endl;
		printf("%20s : %7.3lf second\n", "수동조작 경과시간", yourPlayTime);
		printf("%20s : %7.3lf second\n", "RightHand 경과시간", rhTime);
		printf("%20s : %7.3lf second\n", "BFS 경과시간", bfsTime);
		printf("%20s : %7.3lf second\n", "DIJKSTRA 경과시간", dijkTime);
		printf("%20s : %7.3lf second\n", "Astar 경과시간", astarTime);
		cout << "===========================================================" << endl;
		printf("          [알고리즘 실행 시간]\n");
		cout << "===========================================================" << endl;
		printf("%20s : \t", "RightHand 경과시간");
		cout << rhTimeExc.count() << " ns" << endl;
		printf("%20s : \t", "BFS 경과시간");
		cout << bfsTimeExc.count() << " ns" << endl;
		printf("%20s : \t", "DIJKSTRA 경과시간");
		cout << dijkTimeExc.count() << " ns" << endl;
		printf("%20s : \t", "Astar 경과시간");
		cout << astarTimeExc.count() << " ns" << endl;
		cout << "===========================================================" << endl;
	}
};