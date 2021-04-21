#define int long long
#define belongcell(belongcell__x) (belongcell__x/1000)
const int start_time = 1618993950538ll;

inline int getTime() {
	return (1ll*std::chrono::duration_cast<std::chrono::milliseconds>
	(std::chrono::system_clock::now().time_since_epoch()).count())-start_time;
}

const double pi = 3.1415926;
const int Massage_compress_time = 256;
const int Massage_compress_multiply[4] = {1, 256, 256*256, 256*256*256};
struct enemy_returning_node {
	int x,y; //敌人的坐标
	int job; //敌人的职业
	int speed; //敌人速度的大小
	double facingDirection; //敌人速度的方向
	bool operator < (const enemy_returning_node &u) const { //重载运算符以便set自动排序
		if(x!=u.x)
			return x<u.x;
		return y<u.y;
	}
};
class Massage_Node {
public:
	string str;
private:
	void base_write(int first_pos, int length, int val) ; //进制转换后，将val写入[first_pos,first_pos+length-1]中
	void base_read(int first_pos, int length, int &val) ;//读取[first_pos,first_pos+length-1]的串，转化为十进制后写入val
	int base_read(int first_pos, int length) ; //读取[first_pos,first_pos+length-1]的串，转化为十进制后写入val
	void update_myinfo(shared_ptr<const THUAI4::Character> myinfo) ;
	void update_enemies(vector<shared_ptr<const THUAI4::Character>> *enemies, shared_ptr<const THUAI4::Character> myinfo) ;
	void update_time() ;
	void update_walls(queue<shared_ptr<const THUAI4::Wall>> *walls_info) ;
public:
	void update_all(shared_ptr<const THUAI4::Character> myinfo
	,vector<shared_ptr<const THUAI4::Character>> *enemies,
	queue<shared_ptr<const THUAI4::Wall>> *walls_info) ; //更新出通讯代码！
	Massage_Node () {
		str.clear();
	}
	void sendMassage(GameApi *api) ;
	void read_mate_job(int &mate_job) ; //需要准备返回值的储存位置
	int read_mate_job() ; //直接返回队友职业
	void read_mate_location(int &matex,int &matey) ; //需要准备两个返回值的储存位置
	pair<int,int> read_mate_location() ; //直接返回一个坐标
	void read_enemies(set<enemy_returning_node> *enemies) ; //准备一个set<enemy_returning_node>，然后连着读所有队友传来信息的enemies，以便去重。

	void read_walls(pair<int,int> *walls) ; //准备一个pair数组和组内四个可覆盖元素[a,a+1,a+2,a+3]，调用read_walls(walls+a)。

	bool getMassage(GameApi *api) ;
};
void Massage_Node:: base_write(int first_pos, int length, int val) { //进制转换后，将val写入[first_pos,first_pos+length-1]中
	for(int i=0;i<length;i++) 
		str[first_pos+length-i-1] = (char)((unsigned char)
		((val/Massage_compress_multiply[i])%Massage_compress_time));
}
void Massage_Node:: base_read(int first_pos, int length, int &val) { //读取[first_pos,first_pos+length-1]的串，转化为十进制后写入val
	val = 0;
	for(int i=0;i<length;i++) 
		val += str[first_pos+length-i-1]*Massage_compress_multiply[i];
}
int Massage_Node:: base_read(int first_pos, int length) { //读取[first_pos,first_pos+length-1]的串，转化为十进制后写入val
	int val = 0;
	for(int i=0;i<length;i++) 
		val += str[first_pos+length-i-1]*Massage_compress_multiply[i];
	return val;
}
void Massage_Node:: update_myinfo(shared_ptr<const THUAI4::Character> myinfo) {
	base_write(0,1,(int)((unsigned char)(myinfo->jobType)));
	base_write(1,3,myinfo->x);
	base_write(4,3,myinfo->y);
}
void Massage_Node:: update_enemies(vector<shared_ptr<const THUAI4::Character>> *enemies, shared_ptr<const THUAI4::Character> myinfo) {
	//from 7 to 42
	vector<shared_ptr<const THUAI4::Character>>::iterator ic;
	int i=0;
	for(ic = enemies->begin(); ic!=enemies->end(); i++, ic++) {
		base_write(9*i+7,2,(*ic)->x - myinfo->x + 10000); //用笛卡尔坐标表示相对位置
		base_write(9*i+9,2,(*ic)->y - myinfo->y + 10000);
		base_write(9*i+11,1,(int)((unsigned char)((*ic)->jobType)));
		if((*ic)->isMoving) {
			base_write(9*i+12,2,(int)(((*ic)->facingDirection)/(2*pi)*180.0*180.0));
			base_write(9*i+14,2,(*ic)->moveSpeed);
		}
		else {
			base_write(9*i+12,2,0);
			base_write(9*i+14,2,0);
		}
	}
	while(i<4) {
		base_write(9*i+7,2,0);
		base_write(9*i+9,2,0);
		base_write(9*i+11,1,0);
		base_write(9*i+12,2,0);
		base_write(9*i+14,2,0);
		i++;
	}
}
void Massage_Node:: update_time() {
	//43~45
	base_write(43,3,getTime());
}
void Massage_Node:: update_walls(queue<shared_ptr<const THUAI4::Wall>> *walls_info) {
	//46~53
	shared_ptr<const THUAI4::Wall> tmp;
	for(int i=0;i<4;i++) {
		tmp = walls_info->front();
		walls_info->pop();
		base_write(46+i*2,1,belongcell(tmp->x));
		base_write(46+i*2,1,belongcell(tmp->y));
	}
}
void Massage_Node:: update_all(shared_ptr<const THUAI4::Character> myinfo
,vector<shared_ptr<const THUAI4::Character>> *enemies,
queue<shared_ptr<const THUAI4::Wall>> *walls_info) {
	update_myinfo(myinfo);
	update_enemies(enemies, myinfo);
	update_time();
	update_walls(walls_info);
}
void Massage_Node:: sendMassage(GameApi *api) {
	for(int i=0;i<4;i++)
		api->Send(i,str);
}
void Massage_Node:: read_mate_job(int &mate_job) {
//需要准备返回值的储存位置
	mate_job = base_read(0,1);
}
int Massage_Node:: read_mate_job() { //直接返回队友职业
	return base_read(0,1);
}
void Massage_Node:: read_mate_location(int &matex,int &matey) {
//需要准备两个返回值的储存位置
	matex = base_read(1,3);
	matey = base_read(4,3);
}
pair<int,int> Massage_Node:: read_mate_location() { //直接返回一个坐标
	return pair<int,int> (base_read(1,3),base_read(4,3));
}
void Massage_Node:: read_enemies(set<enemy_returning_node> *enemies) {
//准备一个set<enemy_returning_node>，然后连着读所有队友传来信息的enemies，以便去重。
	enemy_returning_node tmp;
	int matex, matey;
	read_mate_location(matex, matey);
	for(int i=0;i<4;i++) { //更新包含的四个敌人
		tmp.x = base_read(9*i+7,2) - 10000 + matex;
		tmp.y = base_read(9*i+9,2) - 10000 + matey;
		tmp.job = base_read(9*i+11,1);
		if(tmp.x == 0 && tmp.y == 0 && tmp.job == 0) //更新完了
			break ;
		tmp.facingDirection = ((double)(base_read(9*i+12,2)))*2*pi/180.0/180.0;
		tmp.speed = base_read(9*i+14,2);
		enemies->insert(tmp);
	}
}
void Massage_Node:: read_walls(pair<int,int> *walls) {
//准备一个pair数组和组内四个可覆盖元素[a,a+1,a+2,a+3]，调用read_walls(walls+a)。
	for(int i=0;i<4;i++) {
		walls[i].first = base_read(46+i*2,1);
		walls[i].second = base_read(46+i*2,1);
	}
}
bool Massage_Node:: getMassage(GameApi *api) {
	if(!api->MessageAvailable())
		return false;
	api->TryGetMessage(str);
	//自动过滤掉自己发出去的信息！
	shared_ptr<const THUAI4::Character> myinfo = api->GameApi::GetSelfInfo();
	if(base_read(0,1) == ((int)((unsigned char)(myinfo->jobType)))) { //通过判断职业是否相同来过滤自己的信息
		if(!api->MessageAvailable())
			return false;
		api->TryGetMessage(str);
	}
	//如果不希望过滤掉自己的信息，请删除这两条注释间的代码。
	return true;
}
