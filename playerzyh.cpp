#include "AI.h"
#include "Constants.h"
#include <vector>
#include <cmath>
#include <string>
#include <chrono>


//为假则play()调用期间游戏状态更新阻塞，为真则只保证当前游戏状态不会被状态更新函数与GameApi的方法同时访问
extern const bool asynchronous = true;

#define PI 3.1415926
#define start_time  1618993950538ll;
#define degree PI/120;
GameApi* temp;


#include <random>
#include <iostream>
#include "API.h"
#include "API.cpp"
using namespace GameApi;


void message();
void getMes();
inline int getTime();
static int isWalls[50][50];
static int walls[50][50];

/* 请于 VS2019 项目属性中开启 C++17 标准：/std:c++17 */

extern const THUAI4::JobType playerJob = THUAI4::JobType::Job6; //选手职业，选手 !!必须!! 定义此变量来选择职业

namespace
{
	[[maybe_unused]] std::uniform_real_distribution<double> direction(0, 2 * 3.1415926);
	[[maybe_unused]] std::default_random_engine e{std::random_device{}()};
}



void AI::play(GameApi &g)
{
	//获取周边视野和队友信息
	auto GUID = g.GetPlayerGUIDs();
	auto COLOR = g.GetSelfTeamColor();
	auto self = g.GetSelfInfo();
	auto bulletsur = g.GetBullets();
	auto propsur = g.GetProps();
	auto playersur = g.GetCharacters();
	auto wallsur = g.GetWalls();

	int det = 0;
	double P[4][3], B[80][4]; int E = 0, j = 0, b = 0;
	temp = &g;
	std::vector <std::shared_ptr<const THUAI4::Prop>> vp = g.GetProps();
	std::vector <std::shared_ptr<const THUAI4::Character>> ve = g.GetCharacters();
	std::vector <std::shared_ptr<const THUAI4::Wall>> vw = g.GetWalls();
	std::vector <std::shared_ptr<const THUAI4::Bullet>> vb = g.GetBullets();
	auto self = g.GetSelfInfo();
	for (int i = 0; i < (int)vw.size(); i++)
	{
		if (isWalls[(vw[i]->x) / 1000][(vw[i]->y) / 1000] != 1)
		{
			walls[(vw[i]->x) / 1000][(vw[i]->y) / 1000] = 1;
		}
	}
	for (int i = 0; i < (int)ve.size(); i++)
	{
		if (ve[i]->teamID != self->teamID)
		{
			P[j][0] = (double)ve[i]->jobType; P[j][1] = atan2((ve[i]->y - self->y), (ve[i]->x - self->x));
			P[j][2] = sqrt(pow((ve[i]->y - self->y), 2) + pow((ve[i]->x - self->x), 2));
			E++;
		}
	}
	for (int i = 0; i < (int)vb.size(); i++)
	{
		if (vb[i]->teamID != self->teamID)
		{
			if (sqrt(pow(vb[i]->x - self->x, 2) + pow(vb[i]->y - self->y, 2)) <= 2400)
			{
				B[b][0] = vb[i]->moveSpeed;
				B[b][1] = vb[i]->x;
				B[b][2] = vb[i]->y;
				B[b][3] = vb[i]->facingDirection;
				b++;
			}
		}
	}
	 
	//清除运动状态
	while (self->isMoving == 1);

	//变量设定
	int flag = 0;
	double e = 0;
	double t = 0;
	double d;


	//场地检查,寻找TEAM COLOR
	double xc, yc;

	for (int i = 0; (i < 50) && flag == 0; i++)
	{
		for (int j = 0; (j < 50) && flag == 0; j++)
		{
			if (g.GetCellColor(i, j) == COLOR)
				xc = i * 1000;
			yc = j * 1000;
			flag = 1;
			e = atan((yc - self->y) / (xc - self->x));
			d = sqrt((yc - self->y) * (yc - self->y) + (xc - self->x) * (xc - self->x));
		}
	}


	//捡道具
	double xp, yp;
	int i = 0;
	if (propsur.size() != 0)
	{
		int k = propsur.size();
		for (i = 0; (i < k)&&flag==0; i++)
		{
			if ((unsigned int)propsur[i]->propType==6|| (unsigned int)propsur[i]->propType == 7)
			{
				flag = 2;
			}
		}

		if (flag == 2)
		{
			xp = propsur[i]->x;
			yp = propsur[i]->y;
			e = atan((yp - self->y) / (xp - self->x));
			d = sqrt((yp - self->y) * (yp - self->y) + (xp - self->x) * (xp - self->x));
		}
	}


	//投掷子弹
	
	double xf, yf;
	double xw, yw;
    if (self->bulletNum)
	{
		//从队友处获得墙体信息
			
		
		
		//如果没有墙体信息，是否能攻击对方人员
		for(i = 0; i < playersur.size(); i++)
			if (playersur[i]->teamID != self->teamID)
			{
				 xf = playersur[i]->x + playersur[i]->moveSpeed * 25 * cos(playersur[i]->facingDirection);
				 yf = playersur[i]->y + playersur[i]->moveSpeed * 25 * sin(playersur[i]->facingDirection);

				double e = atan((yf - self->y) / (xf - self->x));

				g.Attack(100, direction(e));
			}

	}
	

	//通讯系统
	message();
	getMes();

	//运动指令
	t = d / self->moveSpeed;
	if (t >= 50)
		t = 50;

	switch (flag)
	{
	case 1: g.MovePlayer(t, direction(e)); break;
	case 2:	g.MovePlayer(t, direction(e)); break;
		break;
	}
}

void message()
{
	auto self = temp->GetSelfInfo(); std::vector <std::shared_ptr<const THUAI4::Character>> ve = temp->GetCharacters();
	char sn; int a = 0, b = 0, d, t; char s[2][3],  ej[4], ed[4][2], ev[4][2], wl[4][2], ej[4], ti[3], c = '0';
	std::string ms, E = "MAYDAY";
	s[0][0] = (int)((self->x) / 65536);
	s[0][1] = (int)(((self->x) % 65536) / 256);
	s[0][2] = (int)((self->x) % 256);
	s[1][0] = (int)((self->y) / 65536);
	s[1][1] = (int)(((self->y) % 65536) / 256);
	s[1][2] = (int)((self->y) % 256);

	for (int i = 0; i < 1000; i++)
	{
		for (int j = 0; j < 1000; i++)
		{
			while (b < 4)
			{
				if (walls[i][j] == 1)
				{
					wl[b][0] = (int)(i / 1000);
					wl[b][1] = (int)(j / 1000);
					b++;
				}
			}
		}
		if (b >= 4)
		{
			break;
		}
	}
	for (int i = a; i < 4; i++)
	{
		ej[a] = 0;

		for (int j = 0; j < 2; j++)
		{
			ed[i][j] = 0;
			ev[i][j] = 0;
		}
	}
	for (int i = b; i < 4; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			wl[i][j] = 0;
		}
	}
	t = getTime(); sn = (int)((unsigned char)self->jobType);
	ti[0] = (int)(t / 65536); ti[1] = (int)((t % 65536) / 256); ti[2] = (int)(t % 256);
	ms = sn;
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			ms.append(1, s[i][j]);
		}
	}
	for (int i = 0; i < 4; i++)
	{
		ms.append(1, ej[i]);
	
		for (int j = 0; j < 2; j++)
		{
			ms.append(1, ed[i][j]);
		}
		for (int j = 0; j < 2; j++)
		{
			ms.append(1, ev[i][j]);
		}
	}
	for (int i = 0; i < 3; i++)
	{
		ms.append(1, ti[i]);
	}
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			ms.append(1, wl[i][j]);
		}
	}
	if (self->hp == 0)
	{
		for (int i = 0; i < 6; i++)
		{
			ms.append(1, c);
		}
	}
	else if (a > 1)
	{
		ms.append(E);
	}
	for (int i = 0; i < 4; i++)
	{
		temp->Send(i, ms);
	}
}
void getMes()
{
	auto self = temp->GetSelfInfo();
	int a, b, c, d, f, g; double e;
	for (int i = 0; i < 4; i++)
	{
		std::string msg;
		if (GameApi::TryGetMessage(& msg))
		{
			if (msg[0] == ((int)self->jobType))
			{
				continue;
			}
			else
			{
				a = msg[1] * 65536 + msg[2] * 256 + msg[3];
				b = msg[4] * 65536 + msg[5] * 256 + msg[6];
				c = a - self->x; d = b - self->y;
				e = atan2(d, c);
				temp->Attack(50,e);
				for (int j = 47;j < 55; j + 2)
				{
					f = (int)msg[j]; g = (int)msg[j + 1];
					if (isWalls[f][g] != 1)
					{
						isWalls[f][g] = 1;
						walls[f][g] = 0;
					}
				}
			}
		}
		else
		{
			break;
		}
	}
}

inline int getTime() {
	return (1ll * std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::system_clock::now().time_since_epoch()).count()) - start_time;
}
