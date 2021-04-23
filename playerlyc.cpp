#include "AI.h"
#include "Constants.h"

//为假则play()调用期间游戏状态更新阻塞，为真则只保证当前游戏状态不会被状态更新函数与GameApi的方法同时访问
extern const bool asynchronous = true;

#include <random>
#include <iostream>

/* 请于 VS2019 项目属性中开启 C++17 标准：/std:c++17 */

extern const THUAI4::JobType playerJob = THUAI4::JobType::Job5; //选手职业，选手 !!必须!! 定义此变量来选择职业

namespace
{
	[[maybe_unused]] std::uniform_real_distribution<double> direction(0, 2 * 3.1415926);
	[[maybe_unused]] std::default_random_engine e{ std::random_device{}() };
}



#include <set>
#include <cstring>
#include <queue>
#include <chrono>
#include <algorithm>

using namespace std;

#define int long long
#define belongcell(belongcell__x) ((int)(belongcell__x/1000))
#define d_belongcell(belongcell__x) ((int)(belongcell__x/1000.00))
#define abs(abs__x) (((abs__x)>0)?(abs__x):(-abs__x))

const int start_time = 1618993950538ll;

inline int getTime() {
	return (1ll * std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::system_clock::now().time_since_epoch()).count()) - start_time;
}

const double pi = 3.1415926;
const int Massage_compress_time = 256;
const int Massage_compress_multiply[4] = { 1, 256, 256 * 256, 256 * 256 * 256 };
struct enemy_returning_node {
	int x, y; //敌人的坐标
	int job; //敌人的职业
	int speed; //敌人速度的大小
	double facingDirection; //敌人速度的方向
	bool operator < (const enemy_returning_node& u) const { //重载运算符以便set自动排序
		if (x != u.x)
			return x < u.x;
		return y < u.y;
	}
};
queue<shared_ptr<const THUAI4::Wall>> walls_info;
class Massage_Node {
public:
	string str;
private:
	void base_write(int first_pos, int length, int val); //进制转换后，将val写入[first_pos,first_pos+length-1]中
	void base_read(int first_pos, int length, int& val);//读取[first_pos,first_pos+length-1]的串，转化为十进制后写入val
	int base_read(int first_pos, int length); //读取[first_pos,first_pos+length-1]的串，转化为十进制后写入val
	void update_myinfo(shared_ptr<const THUAI4::Character> myinfo);
	void update_enemies(vector<shared_ptr<const THUAI4::Character>>* enemies, shared_ptr<const THUAI4::Character> myinfo);
	void update_time();
	void update_walls();
public:
	void update_all(shared_ptr<const THUAI4::Character> myinfo
		, vector<shared_ptr<const THUAI4::Character>>* enemies); //更新出通讯代码！
	Massage_Node() {
		str.clear();
	}
	void sendMassage(GameApi* api);
	void read_mate_job(int& mate_job); //需要准备返回值的储存位置
	int read_mate_job(); //直接返回队友职业
	void read_mate_location(int& matex, int& matey); //需要准备两个返回值的储存位置
	pair<int, int> read_mate_location(); //直接返回一个坐标
	void read_enemies(set<enemy_returning_node>* enemies); //准备一个set<enemy_returning_node>，然后连着读所有队友传来信息的enemies，以便去重。
	void read_walls(pair<int, int>* walls); //准备一个pair数组和组内四个可覆盖元素[a,a+1,a+2,a+3]，调用read_walls(walls+a)。
	void read_walls(vector<pair<int, int>>* walls); //准备一个pair组成的vector，新的墙将被丢在后面。
	bool getMassage(GameApi* api);
};
void Massage_Node::base_write(int first_pos, int length, int val) { //进制转换后，将val写入[first_pos,first_pos+length-1]中
	for (int i = 0; i < length; i++)
		str[first_pos + length - i - 1] = (char)((unsigned char)
			((val / Massage_compress_multiply[i]) % Massage_compress_time));
}
void Massage_Node::base_read(int first_pos, int length, int& val) { //读取[first_pos,first_pos+length-1]的串，转化为十进制后写入val
	val = 0;
	for (int i = 0; i < length; i++)
		val += str[first_pos + length - i - 1] * Massage_compress_multiply[i];
}
int Massage_Node::base_read(int first_pos, int length) { //读取[first_pos,first_pos+length-1]的串，转化为十进制后写入val
	int val = 0;
	for (int i = 0; i < length; i++)
		val += str[first_pos + length - i - 1] * Massage_compress_multiply[i];
	return val;
}
void Massage_Node::update_myinfo(shared_ptr<const THUAI4::Character> myinfo) {
	base_write(0, 1, (int)((unsigned char)(myinfo->jobType)));
	base_write(1, 3, myinfo->x);
	base_write(4, 3, myinfo->y);
}
void Massage_Node::update_enemies(vector<shared_ptr<const THUAI4::Character>>* enemies, shared_ptr<const THUAI4::Character> myinfo) {
	//from 7 to 42
	vector<shared_ptr<const THUAI4::Character>>::iterator ic;
	int i = 0;
	for (ic = enemies->begin(); ic != enemies->end(); i++, ic++) {
		if ((*ic)->teamID == myinfo->teamID) {
			i--;
			continue;
		}
		base_write(9 * i + 7, 2, (*ic)->x - myinfo->x + 10000); //用笛卡尔坐标表示相对位置
		base_write(9 * i + 9, 2, (*ic)->y - myinfo->y + 10000);
		base_write(9 * i + 11, 1, (int)((unsigned char)((*ic)->jobType)));
		if ((*ic)->isMoving) {
			base_write(9 * i + 12, 2, (int)(((*ic)->facingDirection) / (2 * pi) * 180.0 * 180.0));
			base_write(9 * i + 14, 2, (*ic)->moveSpeed);
		}
		else {
			base_write(9 * i + 12, 2, 0);
			base_write(9 * i + 14, 2, 0);
		}
	}
	while (i < 4) {
		base_write(9 * i + 7, 2, 0);
		base_write(9 * i + 9, 2, 0);
		base_write(9 * i + 11, 1, 0);
		base_write(9 * i + 12, 2, 0);
		base_write(9 * i + 14, 2, 0);
		i++;
	}
}
void Massage_Node::update_time() {
	//43~45
	base_write(43, 3, getTime());
}
void Massage_Node::update_walls() {
	//46~53
	for (int i = 0; i < 4 && (!walls_info.empty()); i++) {
		base_write(46 + i * 2, 1, belongcell(walls_info.front()->x));
		base_write(47 + i * 2, 1, belongcell(walls_info.front()->y));
		walls_info.pop();
	}
}
void Massage_Node::update_all(shared_ptr<const THUAI4::Character> myinfo
	, vector<shared_ptr<const THUAI4::Character>>* enemies) {
	update_myinfo(myinfo);
	update_enemies(enemies, myinfo);
	update_time();
	update_walls();
}
void Massage_Node::sendMassage(GameApi* api) {
	for (int i = 0; i < 4; i++)
		api->Send(i, str);
}
void Massage_Node::read_mate_job(int& mate_job) {
	//需要准备返回值的储存位置
	mate_job = base_read(0, 1);
}
int Massage_Node::read_mate_job() { //直接返回队友职业
	return base_read(0, 1);
}
void Massage_Node::read_mate_location(int& matex, int& matey) {
	//需要准备两个返回值的储存位置
	matex = base_read(1, 3);
	matey = base_read(4, 3);
}
pair<int, int> Massage_Node::read_mate_location() { //直接返回一个坐标
	return pair<int, int>(base_read(1, 3), base_read(4, 3));
}
void Massage_Node::read_enemies(set<enemy_returning_node>* enemies) {
	//准备一个set<enemy_returning_node>，然后连着读所有队友传来信息的enemies，以便去重。
	enemy_returning_node tmp;
	int matex, matey;
	read_mate_location(matex, matey);
	for (int i = 0; i < 4; i++) { //更新包含的四个敌人
		tmp.x = base_read(9 * i + 7, 2) - 10000 + matex;
		tmp.y = base_read(9 * i + 9, 2) - 10000 + matey;
		tmp.job = base_read(9 * i + 11, 1);
		if (tmp.x == 0 && tmp.y == 0 && tmp.job == 0) //更新完了
			break;
		tmp.facingDirection = ((double)(base_read(9 * i + 12, 2))) * 2 * pi / 180.0 / 180.0;
		tmp.speed = base_read(9 * i + 14, 2);
		enemies->insert(tmp);
	}
}
void Massage_Node::read_walls(pair<int, int>* walls) {
	//准备一个pair数组和组内四个可覆盖元素[a,a+1,a+2,a+3]，调用read_walls(walls+a)。
	for (int i = 0; i < 4; i++) {
		walls[i].first = base_read(46 + i * 2, 1);
		walls[i].second = base_read(47 + i * 2, 1);
	}
}
void Massage_Node::read_walls(vector<pair<int, int>>* walls) {
	for (int i = 0; i < 4; i++) {
		walls->push_back(pair<int, int>(base_read(46 + i * 2, 1), base_read(47 + i * 2, 1)));
	}
}

bool Massage_Node::getMassage(GameApi* api) {
	if (!api->MessageAvailable())
		return false;
	api->TryGetMessage(str);
	//自动过滤掉自己发出去的信息！
	shared_ptr<const THUAI4::Character> myinfo = api->GetSelfInfo();
	if (base_read(0, 1) == ((int)((unsigned char)(myinfo->jobType)))) { //通过判断职业是否相同来过滤自己的信息
		if (!api->MessageAvailable())
			return false;
		api->TryGetMessage(str);
	}
	//如果不希望过滤掉自己的信息，请删除这两条注释间的代码。
	return true;
}

inline bool cell_visible(int x, int y, shared_ptr<const THUAI4::Character> myinfo) {
	int divx = max(abs(((int)myinfo->x) - (x * 1000)), abs(((int)myinfo->x) - (x * 1000 + 999)));
	int divy = max(abs(((int)myinfo->y) - (y * 1000)), abs(((int)myinfo->y) - (x * 1000 + 999)));
	return (divx * divx) + (divy * divy) > 5000 * 5000;
}

struct enemy_node {
	shared_ptr<const THUAI4::Character> data;
	double val;
	enemy_node():val(1) {}
	enemy_node(shared_ptr<const THUAI4::Character> __data, double __val) : data(__data), val(__val) {}
	bool operator < (const enemy_node& u) const {
		return val < u.val;
	}
};

inline double getValue(shared_ptr<const THUAI4::Character> x, shared_ptr<const THUAI4::Character> myinfo, double distance) {
	//优先等级判断，确定我们的攻击目标

	//可以直接打
	double available = 1.0;
	if (abs(belongcell(x->y) - belongcell(myinfo->y)) == 2 && abs(belongcell(x->x) - belongcell(myinfo->x)) <= 2)
		available = 100000000000.00; //直接就可以命中！意不意外！

	if (myinfo->bulletNum == 0)
		available = -1; //呜呜诶没子弹了，逃命要紧！

	double jobval;
	switch ((int)((unsigned char)(x->jobType))) {
	case 0: case 5: jobval = 1; //可以考虑进攻
		break;
	case 1: case 4: jobval = 4; //很可以进攻
		break;
	case 3: jobval = 20; //小紫鱼？杀！
		break;
	case 2: jobval = 0.01; //几乎不要进攻
		break;
	case 6: jobval = 0;//打死不要进攻
	}

	double distval = 1000.0 / (1.0 * distance);

	double speedval;
	if (x->moveSpeed > myinfo->moveSpeed)
		speedval = -1; //追不上，会被人追！跑跑跑！
	else if (x->moveSpeed == myinfo->moveSpeed)
		speedval = 0; //没有速度优势，算了算了
	else {
		if (x->moveSpeed <= myinfo->moveSpeed / 2)
			speedval = 100; //可以啊！
		if (x->moveSpeed < myinfo->moveSpeed / 4)
			speedval = 1000; //妙哉！
	}

	if (available < 0 || speedval < 0)
		return -1;
	return available * speedval * distval * jobval;
}
int knowntime[55][55]; //最后一次被监视过的时间，游走方向判定时使用
int knownmap[55][55]; //未探明-(-1)，包含墙-32，完全空地-(color)

double dist[55][55]; //getdist使用，保存与视野内节点间的距离
double dist_without_restriction[55][55];
void getdist(pair<int, int> cur, bool first_run = true) {
	if (first_run)
		memset(dist, 0x3f, sizeof(dist)), dist[cur.first][cur.second] = 0;
	for (int i = -1; i <= 1; i++)
		for (int j = -1 + abs(i); abs(j) + abs(i) <= 1; j++)
			if (knownmap[cur.first + i][cur.second + j] != 32 &&
				dist[cur.first + i][cur.second + j] > dist[cur.first][cur.second] &&
				knowntime[cur.first + i][cur.second + j] == knowntime[cur.first][cur.second])
				dist[cur.first][cur.second] = dist[cur.first][cur.second] + 1.0,
				getdist(pair<int, int>(cur.first + i, cur.second + j), false);
}
void getdist_without_restriction(pair<int, int> cur, bool first_run = true) {
	if (first_run)
		memset(dist_without_restriction, 0x3f, sizeof(dist_without_restriction)),
		dist_without_restriction[cur.first][cur.second] = 0;
	for (int i = -1; i <= 1; i++)
		for (int j = -1 + abs(i); abs(j) + abs(i) <= 1; j++)
			if (knownmap[cur.first + i][cur.second + j] != 32 && knownmap[cur.first + i][cur.second + j] != -1 &&
				dist_without_restriction[cur.first + i][cur.second + j] > dist_without_restriction[cur.first][cur.second])
				dist_without_restriction[cur.first][cur.second] = dist_without_restriction[cur.first][cur.second] + 1.0,
				getdist(pair<int, int>(cur.first + i, cur.second + j), false);
}
double supplement(shared_ptr<const THUAI4::Character> myinfo) {
	int bullet_potential = myinfo->maxBulletNum - myinfo->bulletNum;
	getdist_without_restriction(pair<int, int>(myinfo->x, myinfo->y));
	double mn = 20000000.00;
	for (int i = 0; i < 50; i++)
		for (int j = 0; j < 50; j++)
			if (knownmap[i][j] == myinfo->teamID && mn > dist_without_restriction[i][j])
				mn = dist_without_restriction[i][j];
	return 1.0 * bullet_potential / mn;
}
bool cmp_deltdir(pair<double, int> x, pair<double, int> y) {
	return x.first > y.first;
}
double findroute(int destx, int desty, int x, int y, double qdis) {
	if (x == belongcell(destx) && y == belongcell(desty))
		return 0;
	if (qdis <= -500)
		return 19260817;
	double last;
	if ((last = findroute(destx, desty, x - 1, y, qdis - 1)) != 19260817)
		return pi;
	if ((last = findroute(destx, desty, x + 1, y, qdis - 1)) != 19260817)
		return 0;
	if ((last = findroute(destx, desty, x, y - 1, qdis - 1)) != 19260817)
		return pi * 3.0 / 2.0;
	if ((last = findroute(destx, desty, x - 1, y, qdis - 1)) != 19260817)
		return pi / 2.0;
	return 19260817;
}
void AI::play(GameApi& g) { //躲子弹&游走&报告目标
//读取并储存数据
	THUAI4::ColorType mycolor = g.GetSelfTeamColor();
	shared_ptr<const THUAI4::Character> myinfo = g.GetSelfInfo();
	vector<shared_ptr<const THUAI4::Character>> characters = g.GetCharacters();
	vector<shared_ptr<const THUAI4::Wall>> walls = g.GetWalls();
	vector<shared_ptr<const THUAI4::Bullet>> bullets = g.GetBullets();

	static bool firstrun;

	if (!firstrun) { //首次更新
		for (int i = 0; i < 50; i++)
			for (int j = 0; j < 50; j++)
				knownmap[i][j] = -1;
		memset(knowntime, 0x4f, sizeof(knowntime));
		firstrun = true;
	}

	vector<shared_ptr<const THUAI4::Wall>>::iterator iw;
	vector<shared_ptr<const THUAI4::Character>>::iterator ic;
	vector<shared_ptr<const THUAI4::Bullet>>::iterator ib;

	for (iw = walls.begin(); iw != walls.end(); iw++) {
		if (knownmap[belongcell((*iw)->x)][belongcell((*iw)->y)] != 32) {
			knownmap[belongcell((*iw)->x)][belongcell((*iw)->y)] = 32;
			walls_info.push(*iw);
		}
	}
	for (int i = 0, tttt = getTime(); i < 50; i++)
		for (int j = 0; j < 50; j++)
			if (cell_visible(i, j, myinfo)) {
				if (knownmap[i][j] != 32)
					knownmap[i][j] = ((unsigned char)(g.GetCellColor(i, j)));
				knowntime[i][j] = tttt;
			}

	Massage_Node massages;
	massages.update_all(myinfo, &characters);
	massages.sendMassage(&g);

	vector<pair<int, int>> new_walls;
	set<enemy_returning_node> enemies;
	set<enemy_returning_node>::iterator is;
	enemy_returning_node tmp_enemy;
	for (ic = characters.begin(); ic != characters.end(); ic++) {
		if ((*ic)->teamID == myinfo->teamID) //是队友，不管
			continue;
		tmp_enemy.x = (*ic)->x;
		tmp_enemy.y = (*ic)->y;
		tmp_enemy.speed = (*ic)->moveSpeed;
		tmp_enemy.facingDirection = (*ic)->facingDirection;
		tmp_enemy.job = ((int)((unsigned char)(*ic)->jobType));
		enemies.insert(tmp_enemy);
	}
	while (massages.getMassage(&g)) {
		massages.read_enemies(&enemies);
		massages.read_walls(&new_walls);
	}

	//策略部分

#define decorate(x) ((x)<0.0?(2*pi-(x)):(x))

	memset(dist, 0x3f, sizeof(dist));

	getdist(pair<int, int>(myinfo->x, myinfo->y));
	enemy_node sorted[10];
	int cnt_sorted = 0;
	for (ic = characters.begin(); ic != characters.end(); ic++) {
		if ((*ic)->teamID == myinfo->teamID) //显然不能攻击队友
			continue;
		sorted[++cnt_sorted] = enemy_node(*ic, getValue((*ic), myinfo, dist[belongcell((*ic)->x)][belongcell((*ic)->y)]));
	}
	sort(sorted + 1, sorted + cnt_sorted + 1);

	if (sorted[cnt_sorted].val >= 10000000) { //开炮！
		g.Attack(0, decorate(atan((0.0 + sorted[cnt_sorted].data->y - myinfo->y) / (0.0 + sorted[cnt_sorted].data->x - myinfo->x))));
	}
	if (sorted[1].val < 0) { //跑！
		double dir[5]; double tmptmp; int cntdir = 1;
		while ((tmptmp = sorted[cntdir].val) < 0.0) {
			dir[cntdir] = decorate(atan((0.0 + sorted[1].data->y - myinfo->y) / (0.0 + sorted[1].data->x - myinfo->x)));
			cntdir ++;
		}
		cntdir--;
		sort(dir + 1, dir + cntdir + 1);
		dir[0] = dir[cntdir] - (2 * pi);
		pair<double, int> deltdir[5];
		for (int i = 1; i <= cntdir; i++)
			deltdir[i] = pair<double, int>(dir[i], i);
		sort(deltdir + 1, deltdir + cntdir + 1, cmp_deltdir);
		for (int i = 1; i <= cntdir; i++) {
			dir[i] = decorate(dir[i] - (deltdir[i].first / 2.0));
			if (knownmap[d_belongcell(myinfo->x + cos(dir[i]) * 1000.0)][d_belongcell(myinfo->y + sin(dir[i]) * 1000.0)] == 32)
				continue;
			if (knownmap[d_belongcell(myinfo->x + cos(dir[i]) * 2000.0)][d_belongcell(myinfo->y + sin(dir[i]) * 2000.0)] == 32)
				continue;
			//如果会碰到子弹，增加一个小量的扰动
			//回头有空再写吧
			g.MovePlayer(50, dir[i] + (rand() % 200 - 100) * 0.000628);
		}
	}
	else if (sorted[cnt_sorted].val > supplement(myinfo)) { //冲！
		g.MovePlayer(50, findroute(sorted[cnt_sorted].data->x, sorted[cnt_sorted].data->y, myinfo->x, myinfo->y, dist[sorted[cnt_sorted].data->x][sorted[cnt_sorted].data->y]) + (rand() % 200 - 100) * 0.000628);
	}
	else { //游走 
		double mn = 21474836470000ll; int mnx, mny;
		for (int i = 0; i < 50; i++)
			for (int j = 0; j < 50; j++)
				if (knownmap[i][j] == myinfo->teamID && mn > dist_without_restriction[i][j])
					mn = dist_without_restriction[i][j], mnx = i, mny = j;
		g.MovePlayer(50, findroute(mnx, mny, myinfo->x, myinfo->y, dist_without_restriction[mnx][mny]));
	}
}
