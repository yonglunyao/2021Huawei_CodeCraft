//
//                       _oo0oo_
//                      o8888888o
//                      88" . "88
//                      (| -_- |)
//                      0\  =  /0
//                    ___/`---'\___
//                  .' \\|     |// '.
//                 / \\|||  :  |||// \
//                / _||||| -:- |||||- \
//               |   | \\\  -  /// |   |
//               | \_|  ''\---/''  |_/ |
//               \  .-\__  '-'  ___/-. /
//             ___'. .'  /--.--\  `. .'___
//          ."" '<  `.___\_<|>_/___.' >' "".
//         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
//         \  \ `_.   \_ __\ /__ _/   .-` /  /
//     =====`-.____`.___ \_____/___.-`___.-'=====
//                       `=---='
//
//
//     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//               佛祖保佑         永无BUG
//
//
//

//v5.1

#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <ctime>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <iomanip>  //流控制头文件
#include <stdlib.h>
using namespace std;

#define MIG_SWITCH 0x0111//迁移的开关，

// 调用参数
int debug = 0;
int upload = 1;

double cpuRemainRateBase = 0.01;
double memRemainRateBase = 0.05;
double cpuRemainRateBase2 = 0.04;
double memRemainRateBase2 = 0.04;

int locRam = 1;
// cpuRate 占比，从0到10
double cpuPercent = 2.1;
double buyCpuPercent = 2.3;
double buyCpuPercentBase = 2;
double buyCpuPercentDen = 10;
//double hardPercent = 5;
// 排序用的性价比

int sortRate = 1;

//CPU/MEM使用率用于决定是否迁移的上下限,目前还没用到
//double LowCpuUsageRate = 0.7;
//double LowMemUsageRate = 0.7;

//在最小的虚拟机基础上设置一个阈值，因为请求的不一定每次都是最小的
//考虑到最小的虚拟机的CPU是14，内存是1(第一个数据集)
//可以设置阈值如下，可调整
int cpuAdd = 0;
int memAdd = 0;

const double Inf = 1e9 + 7;
int allVmcount = 0;

//定义当前是第几天
int day = 0;
int allDays = 0;

//计算程序运行时间，防止超时
clock_t startTime, currentTime;
bool enableMigration = true;


//竞价参数----------------------------------------------------------------------------
const int matchPriceNumber = 400;
int PriceCount = matchPriceNumber;
int countLoc = 0;

//pair<int, int> singleMatchPriceData;
//vector<pair<int, int>> matchPriceData;//需要全局变量
//int matchPriceDataCount = 0;

double xLowestCost[matchPriceNumber];
double differenceTable[2* matchPriceNumber][2* matchPriceNumber] = { 0 };


// 服务器产品信息
struct Server {
	string name;
	int cpuA;
	int cpuB;
	int memA;
	int memB;
	int allcpu;
	int allmem;
	int hardCost;
	int dayCost;
	double cpuRate;
	double memRate;
	double cpuMemRateA;
	double cpuMemRateB;
	double cpuUsageRateA;
	double cpuUsageRateB;
	double memUsageRateA;
	double memUsageRateB;
	double rate;
	int vmCount = 0; // 虚拟机数量
	int id; // 购买时给的编号
	int buildDay;//创建的那一天

	string toString() {
		return name + "," + to_string(cpuA * 2) + "," + to_string(memA * 2) + "," + to_string(hardCost) + "," + to_string(dayCost);
	}
};

// 虚拟机产品信息
struct Vm {
	string name;
	int cpu;
	int mem;
	int doubleNode;

	//////////
	int serverId;
	int part;//0是双节点，1是A，2是B
	//////////
	string toString() {
		return name + "," + to_string(cpu) + "," + to_string(mem) + "," + to_string(doubleNode);
	}

};

//定义了一个服务器单节点的结构体
struct ServerSingle {
	string name;
	int cpu;
	int mem;
	int id;
	int part;//1:A，2:B
};

// 安装的虚拟机信息
struct VmIns {
	string name;
	int id;
	int serverId;
	int part;  // 0：双节点 1在 A, 2 在 B
	int cpu;
	int mem;
	bool isOnline;//标志allvms中某虚拟机是否部署在服务器上
	int aliveDays;
	int userQuotation;//用户报价
	int myQuotation;//我的报价
	int oppoQuotation;//对手的报价

	bool operator==(const VmIns& vm) const {
		return id == vm.id;
	}
};

struct Command {
	int type; // 0 删除 1 添加
	string vmType;
	int vmId;
	int id;//cmd的ID，只有add的有id，解决排序后的输出问题
	int aliveDays;
	int userQuotation;//用户报价
	bool success = true;
	string toString() {
		return to_string(type) + "," + vmType + "," + to_string(vmId);
	}
};

//用于暂存当天输出信息
struct dayOutputInfo {
	int servId;
	int nodeType;//0:双节点，1：单节点A，2：单节点B
	int id;//命令的id
};

//请求的结果
struct getResult {
	bool success;
	int oppoQuotation;//对手的报价
};

//clock_t timeStart, timeEnd;

// 服务器信息
unordered_map<string, Server> serverInfos;
// 虚拟机
unordered_map<string, Vm> vmInfos;
// 服务器数量
int serverCount = 0;
// 已购买服务器信息
vector<Server> allServers;
//购买所以服务器的备份
vector<Server> allServersBak;
// 所有虚拟机信息
unordered_map<int, VmIns> allVms;
//每台服务器部署的虚拟机列表
unordered_map<int, vector<VmIns>> vmOnServ;
//全部的指令
vector<vector<Command>> allCommands;


// 按性价比排序的服务器
vector<Server> rateServers;

//按核内比排序的服务器
vector<Server> rateRatioServers;

//vector<pair<string, int>> dayServers;
map<string, vector<int>> dayServers;
vector<string> dayOutput;
//vector<string> dayAddOutput;//add请求的输出信息
vector<dayOutputInfo> dayOutInfo;
vector<string> dayMigrationOutput;//暂存当天迁移的输出信息
vector<Server> daySleepServer;//当天睡眠的服务器列表
vector<string> dayQuotationOutput;//每天的报价输出

double dayRequestCpuMemRatio;//每天请求的核内比

//用于映射两个ID
unordered_map<int, int> IdMap;

// 成本
long long SERVERCOST = 0, POWERCOST = 0, TOTALCOST = 0;

// 输出结果
vector<string> output;
//--------------------------------------------------------

// 添加服务器类型
void addServerInfo(string& serverName, int cpu, int mem, int hardCost, int dayCost) {
	string name = serverName.substr(1, serverName.size() - 2);
	//Server server{ name, cpu / 2, cpu / 2, mem / 2, mem / 2, hardCost, dayCost };//此处编译异常
	Server server;
	server.name = name;
	server.cpuA = cpu / 2;
	server.cpuB = cpu / 2;
	server.memA = mem / 2;
	server.memB = mem / 2;
	server.allcpu = cpu;
	server.allmem = mem;
	server.hardCost = hardCost;
	server.dayCost = dayCost;
	server.cpuUsageRateA = server.cpuUsageRateB = 0;
	server.memUsageRateA = server.memUsageRateB = 0;
	server.cpuRate = (hardCost / (server.cpuA + 0.0)) / 2;
	server.memRate = (hardCost / (server.memA + 0.0)) / 2;
	server.cpuMemRateA = (double)server.cpuA / (double)server.memA;
	server.cpuMemRateB = (double)server.cpuB / (double)server.memB;
	server.rate = (server.cpuRate * buyCpuPercent + server.memRate * (10 - buyCpuPercent)) / 2;
	//server.rate = cpuPercent * server.allcpu + server.allmem;
	serverInfos[name] = server;
}

// 添加虚拟机类型
void addVmInfo(string& vmName, int cpu, int mem, int doubleNode) {
	string name = vmName.substr(1, vmName.size() - 2);
	Vm vm = { name, cpu, mem, doubleNode };
	vmInfos[name] = vm;
	//if (cpu < minVm.cpu&& mem < minVm.mem)
	//    minVm = vm;
}

bool compareRate(Server& s1, Server& s2, int type) {
	if (type == 1) {
		return s1.rate > s2.rate;
	}
	else if (type == 2) {
		return s1.cpuRate > s2.cpuRate;
	}
	else {
		return s1.memRate > s2.memRate;
	}
}

// 服务器排序
bool sortServerInfoByCpuMemRatio(const Server& serv1, const Server& serv2) {
	return serv1.allcpu / (serv1.allmem + 0.0) > serv2.allcpu / (serv2.allmem + 0.0);
}

bool sortServerByRate(const Server& serv1, const Server& serv2) {
	return serv1.rate < serv2.rate;
}

void sortServerInfo() {

	//for (auto iter = serverInfos.begin(); iter != serverInfos.end(); ++iter) {
	//    bool add = false;
	//    for (int i = 0; i < rateServers.size(); i++) {
	//        // 按不同的性价比选择服务器
	//        if (compareRate(rateServers.at(i), iter->second, sortRate)) {
	//            rateServers.insert(rateServers.begin() + i, iter->second);
	//            add = true;
	//            break;
	//        }
	//    }
	//    if (!add) {
	//        rateServers.push_back(iter->second);
	//    }
	//}

	rateServers.clear();

	int width = 20;
	//查找今日请求的核内比在哪个范围之内
	int index;
	for (index = 0; index < rateRatioServers.size() - 1; index++) {
		if (dayRequestCpuMemRatio <= rateRatioServers[index].allcpu / (rateRatioServers[index].allmem + 0.0) &&
			dayRequestCpuMemRatio > rateRatioServers[index + 1].allcpu / (rateRatioServers[index + 1].allmem + 0.0)) {
			break;
		}
	}

	if (dayRequestCpuMemRatio > rateRatioServers[0].allcpu / (rateRatioServers[0].allmem + 0.0)) {
		index = -1;
	}

	if (index < width) {
		rateServers = vector<Server>(rateRatioServers.begin(), rateRatioServers.begin() + (index + width));
	}
	else if (index > rateRatioServers.size() - width) {
		rateServers = vector<Server>(rateRatioServers.begin() + (index - width), rateRatioServers.end());
	}
	else {
		rateServers = vector<Server>(rateRatioServers.begin() + (index - width), rateRatioServers.begin() + (index + width));
	}

	std::sort(rateServers.begin(), rateServers.end(), sortServerByRate);

	//把剩下的服务器也加入列表，防止有的虚拟机永远也买不上服务器（有重复的服务器，无所谓了）
	for (auto serv : rateRatioServers) {
		rateServers.push_back(serv);
	}

}


// 取合适的服务器信息 
Server getBestServerInfo(VmIns& vmi) {

	Vm vm = vmInfos[vmi.name];
	for (auto& serv : rateServers) {
		if (vm.doubleNode == 0) {
			if (serv.cpuA >= vm.cpu && serv.memA >= vm.mem) {
				return serv;
			}
		}
		else {
			if (serv.cpuA >= vm.cpu / 2 && serv.memA >= vm.mem / 2) {
				return serv;
			}
		}
	}
	return rateServers.at(0);
}

void copyServInfo(Server& src, Server& dest) {
	dest.name = src.name;
	dest.cpuA = src.cpuA;
	dest.cpuB = src.cpuB;
	dest.memA = src.memA;
	dest.memB = src.memB;
	dest.allcpu = src.allcpu;
	dest.allmem = src.allmem;
	dest.hardCost = src.hardCost;
	dest.dayCost = src.dayCost;
	dest.cpuUsageRateA = src.cpuUsageRateA;
	dest.cpuUsageRateB = src.cpuUsageRateB;
	dest.memUsageRateA = src.memUsageRateA;
	dest.memUsageRateB = src.memUsageRateB;
	dest.cpuRate = src.cpuRate;
	dest.memRate = src.memRate;
}

// 购买新的服务器
//参数定义同addVm函数
void addServer(VmIns& vmi, Server* pServ, int mode) {
	Server info = getBestServerInfo(vmi);
	Server serv;
	copyServInfo(info, serv);
	serv.id = allServers.size();
	serv.buildDay = day;

	if (mode == 0) {
		*pServ = serv;
		return;
	}

	allServers.push_back(serv);

	SERVERCOST += serv.hardCost;
	dayServers[serv.name].push_back(serv.id);
}

void printServerAll(Server& serv, Vm& vm, VmIns& vmi, int add) {
	if (1) {
		return;
	}
	if (add) {

		// cout<<"Server "<<serv.id<<" "<<serv.name<<" cpu "<<serv.cpuA<<" "<<serv.cpuB<<" mem "
		//     <<serv.memA<<" "<<serv.memB<< " vm "<<vm.name<< " id "<<vmi.id<<" cpu "<<vm.cpu<<" "<<vm.mem<<endl;
		cout << "Server " << serv.id << " " << "name" << " cpu " << serv.cpuA << " " << serv.cpuB << " mem "
			<< serv.memA << " " << serv.memB << " vm " << "vmname" << " cpu " << vm.cpu << " " << vm.mem << endl;
	}
	else {
		cout << "del Se " << serv.id << " " << serv.name << " cpu " << serv.cpuA << " " << serv.cpuB << " mem "
			<< serv.memA << " " << serv.memB << " vm " << vm.name << " id " << vmi.id << " cpu " << vm.cpu << " " << vm.mem << endl;

	}
}

void printServerNow(Server& serv, Vm& vm, VmIns& vmi) {
	printServerAll(serv, vm, vmi, 1);
}
void printServerDel(Server& serv, Vm& vm, VmIns& vmi) {
	printServerAll(serv, vm, vmi, 0);
}

// 把虚拟机添加到
int addVmToServer(VmIns& vmi, Server& serv, int part, int cmdId) {

	Vm vm = vmInfos[vmi.name];
	bool added = false;

	if (part == 0) {
		if (serv.cpuA >= vm.cpu / 2 && serv.cpuB >= vm.cpu / 2
			&& serv.memA >= vm.mem / 2 && serv.memB >= vm.mem / 2) {
			serv.cpuA -= vm.cpu / 2;
			serv.cpuB -= vm.cpu / 2;
			serv.memA -= vm.mem / 2;
			serv.memB -= vm.mem / 2;

			added = 1;
			vmi.part = 0;
			vm.part = 0;
		}
	}
	else if (part == 1) {
		if (serv.cpuA >= vm.cpu && serv.memA >= vm.mem) {
			serv.cpuA -= vm.cpu;
			serv.memA -= vm.mem;

			vmi.part = 1;
			vm.part = 1;//与vmi的part定义有区别，后期可以消除该区别
			added = 1;
		}

	}
	else {
		if (serv.cpuB >= vm.cpu && serv.memB >= vm.mem) {
			serv.cpuB -= vm.cpu;
			serv.memB -= vm.mem;

			vmi.part = 2;
			vm.part = 2;
			added = 1;
		}
	}

	if (added) {

		//调试
		//if (serv.id == 339)
		//{
		//    serv.id = serv.id + 0;
		//}

		vmi.serverId = serv.id;
		vmi.isOnline = true;
		serv.vmCount += 1;
		allVms[vmi.id] = vmi;
		vmOnServ[serv.id].push_back(vmi);//将虚拟机信息加入列表
		allVmcount++;
		if (vm.doubleNode) {
			dayOutInfo.push_back({ serv.id,0 , cmdId });
			//dayOutput.push_back("(" + to_string(serv.id) + ")");
		}
		else {
			//string node = vmi.part == 0 ? "A" : "B";
			//dayOutput.push_back("(" + to_string(serv.id) + ", " + node + ")");
			dayOutInfo.push_back({ serv.id,vmi.part == 1 ? 1 : 2 , cmdId });
		}
		//更新CPU和内存的使用率
		serv.cpuUsageRateA = 1 - (double)serv.cpuA / (double)(serv.allcpu / 2);//需要强转double
		serv.cpuUsageRateB = 1 - (double)serv.cpuB / (double)(serv.allcpu / 2);
		serv.memUsageRateA = 1 - (double)serv.memA / (double)(serv.allmem / 2);
		serv.memUsageRateB = 1 - (double)serv.memB / (double)(serv.allmem / 2);

		//更新核内比
		if (serv.memA == 0)
			serv.cpuMemRateA = Inf;
		else
			serv.cpuMemRateA = (double)serv.cpuA / (double)serv.memA;

		if (serv.memB == 0)
			serv.cpuMemRateB = Inf;
		else
			serv.cpuMemRateB = (double)serv.cpuB / (double)serv.memB;


		printServerNow(serv, vm, vmi);//？？？
	}

	return added;
}

// 添加虚拟机
//返回0添加在已有的服务器上，返回0未添加，需要购买服务器
//servId:传出参数，传出尝试添加的服务器ID，当mode=1传NULL
//mode: 0-尝试添加（用于报价，未真实添加）， 1-真实添加
int addVm(const Command& cmd, Server* pServ, int mode) {
	VmIns vmi = { cmd.vmType, cmd.vmId };

	vmi.cpu = vmInfos[vmi.name].cpu;
	vmi.mem = vmInfos[vmi.name].mem;

	daySleepServer.clear();

	bool added = false;

	//双节点
	while (added == false) {
		if (vmInfos[vmi.name].doubleNode == 1) {
			int best_id = -1;
			int best_new_id = -1;
			double minEqValue = Inf;
			int i = 0;
			//先在旧服务器上寻找
			int oldServNum = 0;
			for (auto& mServ : dayServers) {
				oldServNum += mServ.second.size();
			}
			for (i = 0; i < allServers.size() - oldServNum; i++) {
				//if (allServers[i].vmCount == 0) {
				//    daySleepServer.push_back(allServers[i]);
				//    continue;//如果这台服务器是关机状态，就先跳过去
				//}
				if (allServers[i].cpuA >= vmi.cpu / 2 && allServers[i].cpuB >= vmi.cpu / 2 &&
					allServers[i].memA >= vmi.mem / 2 && allServers[i].memB >= vmi.mem / 2) {
					int remainCpu = (allServers[i].cpuA - vmi.cpu / 2) + (allServers[i].cpuB - vmi.cpu / 2);
					int remainMem = (allServers[i].memA - vmi.mem / 2) + (allServers[i].memB - vmi.mem / 2);
					//double eqValue = cpuPercent * remainCpu + remainMem;//等效的剩余价值，自己根据数据集推导的公公式
					double eqValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
					if (eqValue < minEqValue) {
						best_id = i;
						minEqValue = eqValue;
					}
				}
			}

			for (; i < allServers.size(); i++) {
				if (allServers[i].vmCount == 0) {
					daySleepServer.push_back(allServers[i]);
					continue;//如果这台服务器是关机状态，就先跳过去
				}
				if (allServers[i].cpuA >= vmi.cpu / 2 && allServers[i].cpuB >= vmi.cpu / 2 &&
					allServers[i].memA >= vmi.mem / 2 && allServers[i].memB >= vmi.mem / 2) {
					int remainCpu = (allServers[i].cpuA - vmi.cpu / 2) + (allServers[i].cpuB - vmi.cpu / 2);
					int remainMem = (allServers[i].memA - vmi.mem / 2) + (allServers[i].memB - vmi.mem / 2);
					//double eqValue = cpuPercent * remainCpu + remainMem;//等效的剩余价值，自己根据数据集推导的公公式
					double eqValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
					if (eqValue < minEqValue) {
						best_new_id = i;
						minEqValue = eqValue;
					}
				}
			}

			//在睡眠服务器上寻找
			if (best_new_id == -1 && best_id == -1 && daySleepServer.size() != 0) {

				for (int k = 0; k < daySleepServer.size(); k++) {
					if (daySleepServer[k].cpuA >= vmi.cpu / 2 && daySleepServer[k].cpuB >= vmi.cpu / 2 &&
						daySleepServer[k].memA >= vmi.mem / 2 && daySleepServer[k].memB >= vmi.mem / 2) {
						int remainCpu = (daySleepServer[k].cpuA - vmi.cpu / 2) + (daySleepServer[k].cpuB - vmi.cpu / 2);
						int remainMem = (daySleepServer[k].memA - vmi.mem / 2) + (daySleepServer[k].memB - vmi.mem / 2);
						//double eqValue = cpuPercent * remainCpu + remainMem;//等效的剩余价值，自己根据数据集推导的公公式
						double eqValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
						if (eqValue < minEqValue) {
							best_id = daySleepServer[k].id;
							minEqValue = eqValue;
						}
					}
				}
				//return 1;//需要购买服务器
			}

			if (best_new_id == -1 && best_id == -1) {
				//addServer(vmi);
				//added = addVmToServer(vmi, allServers[allServers.size() - 1], 0, cmd.id);
				//break;
				//如果是尝试添加的情况
				if (mode == 0)
					addServer(vmi, pServ, 0);
				return 1;//需要购买服务器
			}
			//在新服务器上
			else if (best_id == -1) {
				if (mode == 0) {
					*pServ = allServers[best_new_id];
					return 0;
				}
				added = addVmToServer(vmi, allServers[best_new_id], 0, cmd.id);//已经判断过了，一定能添加的上，传0表示是双节点
				//added = true;
				if (added == false) {
					cout << "error!" << endl;
				}
			}
			//在旧服务器上
			else {
				if (mode == 0) {
					*pServ = allServers[best_id];
					return 0;
				}
				added = addVmToServer(vmi, allServers[best_id], 0, cmd.id);//已经判断过了，一定能添加的上
				//added = true;
				if (added == false) {
					cout << "error!" << endl;
				}
			}
		}
		//单节点服务器
		else {
			int best_id = -1;
			int best_new_id = -1;
			int part = -1;//区分A、B节点，1：A，2：B
			int newpart = -1;
			double minEqValue = Inf;
			int i = 0;
			int oldServNum = 0;
			for (auto& mServ : dayServers) {
				oldServNum += mServ.second.size();
			}
			//先在旧服务器上寻找剩余空间最小的服务器
			for (i = 0; i < allServers.size() - oldServNum; i++) {
				//if (allServers[i].vmCount == 0) {
				//    daySleepServer.push_back(allServers[i]);
				//    continue;//如果这台服务器是关机状态，就先跳过去
				//}
				if (allServers[i].cpuA >= vmi.cpu && allServers[i].memA >= vmi.mem) {
					int remainCpu = (allServers[i].cpuA - vmi.cpu);
					int remainMem = (allServers[i].memA - vmi.mem);
					//double eqValue = cpuPercent * remainCpu + remainMem;//等效的剩余价值，自己根据数据集推导的公公式
					double eqValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
					if (eqValue < minEqValue) {
						best_id = i;
						minEqValue = eqValue;
						part = 1;
					}

				}
				if (allServers[i].cpuB >= vmi.cpu && allServers[i].memB >= vmi.mem) {
					int remainCpu = (allServers[i].cpuB - vmi.cpu);
					int remainMem = (allServers[i].memB - vmi.mem);
					//double eqValue = cpuPercent * remainCpu + remainMem;//等效的剩余价值，自己根据数据集推导的公公式
					double eqValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
					if (eqValue < minEqValue) {
						best_id = i;
						minEqValue = eqValue;
						part = 2;
					}
				}
			}

			for (; i < allServers.size(); i++) {
				//if (allServers[i].vmCount == 0) {
				//    daySleepServer.push_back(allServers[i]);
				//    continue;//如果这台服务器是关机状态，就先跳过去
				//}
				if (allServers[i].cpuA >= vmi.cpu && allServers[i].memA >= vmi.mem) {
					int remainCpu = (allServers[i].cpuA - vmi.cpu);
					int remainMem = (allServers[i].memA - vmi.mem);
					//double eqValue = cpuPercent * remainCpu + remainMem;//等效的剩余价值，自己根据数据集推导的公公式
					double eqValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
					if (eqValue < minEqValue) {
						best_new_id = i;
						minEqValue = eqValue;
						newpart = 1;
					}

				}
				if (allServers[i].cpuB >= vmi.cpu && allServers[i].memB >= vmi.mem) {
					int remainCpu = (allServers[i].cpuB - vmi.cpu);
					int remainMem = (allServers[i].memB - vmi.mem);
					//double eqValue = cpuPercent * remainCpu + remainMem;//等效的剩余价值，自己根据数据集推导的公公式
					double eqValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
					if (eqValue < minEqValue) {
						best_new_id = i;
						minEqValue = eqValue;
						newpart = 2;
					}
				}
			}

			//在睡眠服务器上寻找
			if (best_new_id == -1 && best_id == -1 && daySleepServer.size() != 0) {

				for (int k = 0; k < daySleepServer.size(); k++) {
					if (daySleepServer[k].cpuA >= vmi.cpu && daySleepServer[k].memA >= vmi.mem) {
						int remainCpu = (daySleepServer[k].cpuA - vmi.cpu);
						int remainMem = (daySleepServer[k].memA - vmi.mem);
						//double eqValue = cpuPercent * remainCpu + remainMem;//等效的剩余价值，自己根据数据集推导的公公式
						double eqValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
						if (eqValue < minEqValue) {
							best_id = daySleepServer[k].id;
							minEqValue = eqValue;
							part = 1;
						}

					}
					if (daySleepServer[k].cpuB >= vmi.cpu && daySleepServer[k].memB >= vmi.mem) {
						int remainCpu = (daySleepServer[k].cpuB - vmi.cpu);
						int remainMem = (daySleepServer[k].memB - vmi.mem);
						//double eqValue = cpuPercent * remainCpu + remainMem;//等效的剩余价值，自己根据数据集推导的公公式
						double eqValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
						if (eqValue < minEqValue) {
							best_id = daySleepServer[k].id;
							minEqValue = eqValue;
							part = 2;
						}
					}
				}
				//return 1;//需要购买服务器
			}

			if (best_new_id == -1 && best_id == -1) {
				//addServer(vmi);
				//added = addVmToServer(vmi, allServers[allServers.size() - 1], 1, cmd.id);
				//break;
				if (mode == 0)
					addServer(vmi, pServ, 0);
				return 1;
			}
			//在新服务器上
			else if (best_id == -1) {
				if (mode == 0) {
					*pServ = allServers[best_new_id];
					return 0;
				}
				added = addVmToServer(vmi, allServers[best_new_id], newpart, cmd.id);//已经判断过了，一定能添加的上
				//added = true;
				if (added == false) {
					cout << "error!" << endl;
				}
			}
			//在旧服务器上
			else {
				if (mode == 0) {
					*pServ = allServers[best_id];
					return 0;
				}
				added = addVmToServer(vmi, allServers[best_id], part, cmd.id);//已经判断过了，一定能添加的上
				//added = true;
				if (added == false) {
					cout << "error!" << endl;
				}
			}
		}
	}

	return 0;
}
// 删除虚拟机
void deleteVm(Command& cmd) {
	VmIns vmi = allVms[cmd.vmId];

	//要删除的虚拟机可能不在虚拟机列表中，因为可能没有成功获取到该虚拟机对应的添加请求
	if (vmi.name == "")
		return;

	Vm vm = vmInfos[vmi.name];
	Server& serv = allServers.at(vmi.serverId);
	serv.vmCount--;

	vector<VmIns>& tmpVmList = vmOnServ[vmi.serverId];
	tmpVmList.erase(remove(tmpVmList.begin(), tmpVmList.end(), vmi), tmpVmList.end());

	if (vm.doubleNode) {
		serv.cpuA += vm.cpu / 2;
		serv.cpuB += vm.cpu / 2;
		serv.memA += vm.mem / 2;
		serv.memB += vm.mem / 2;
	}
	else {
		if (vmi.part == 1) {//1是A节点，2是B节点
			serv.cpuA += vm.cpu;
			serv.memA += vm.mem;
		}
		else {
			serv.cpuB += vm.cpu;
			serv.memB += vm.mem;
		}
	}
	//更新
	serv.cpuUsageRateA = 1 - (double)serv.cpuA / (double)(serv.allcpu / 2);//需要强转double
	serv.cpuUsageRateB = 1 - (double)serv.cpuB / (double)(serv.allcpu / 2);
	serv.memUsageRateA = 1 - (double)serv.memA / (double)(serv.allmem / 2);
	serv.memUsageRateB = 1 - (double)serv.memB / (double)(serv.allmem / 2);

	//更新核内比
	serv.cpuMemRateA = (double)serv.cpuA / (double)serv.memA;
	serv.cpuMemRateB = (double)serv.cpuB / (double)serv.memB;

	allVms[vmi.id].isOnline = false;

	allVmcount--;
	printServerDel(serv, vm, vmi);

}


//此处准备迁移
bool sortServByVmCount(Server serv1, Server serv2) {
	return serv1.vmCount < serv2.vmCount;
}

bool sortServSingleListByCpuMinusMem(const ServerSingle& sserv1, const ServerSingle& sserv2) {
	return sserv1.cpu * cpuPercent - sserv1.mem > sserv2.cpu * cpuPercent - sserv2.mem;
}

bool sortVmListByMemMinusCpu(const VmIns& vmi1, const VmIns& vmi2) {
	return vmi1.cpu * cpuPercent - vmi1.mem < vmi2.cpu* cpuPercent - vmi2.mem;
}

bool sortVmListByCpuMinusMem(const VmIns& vmi1, const VmIns& vmi2) {
	return vmi1.cpu * cpuPercent - vmi1.mem > vmi2.cpu * cpuPercent - vmi2.mem;
}

bool sortServByCpuMinusME(const Server& serv1, const Server& serv2) {
	return serv1.cpuA * cpuPercent + serv1.cpuB * cpuPercent - serv1.memA - serv1.memB > serv2.cpuA * cpuPercent + serv2.cpuB * cpuPercent - serv2.memA - serv2.memB;
}

////定义一个均衡指数
bool sortServByBalanceIndex(const Server& serv1, const Server& serv2) {
	//double balanceIndex1 = log((serv1.cpuA + serv1.cpuB + 1) / (serv1.memA + serv1.memB + 1.0));
	//double balanceIndex2 = log((serv2.cpuA + serv2.cpuB + 1) / (serv2.memA + serv2.memB + 1.0));
	double balanceIndex1 = (serv1.cpuA + serv1.cpuB) * cpuPercent - serv1.memA - serv1.memB;
	double balanceIndex2 = (serv2.cpuA + serv2.cpuB) * cpuPercent - serv2.memA - serv2.memB;
	return balanceIndex1 > balanceIndex2;
}

//取合适的服务器信息 ，仅用于迁移中购买服务器
Server getBestServerInfoForMig(VmIns& vmi) {
	for (auto& serv : rateServers) {
		if (serv.cpuA >= vmi.cpu && serv.memA >= vmi.mem) {
			return serv;
		}
	}

	Server errorServ;
	errorServ.name = "errorServ";
	return errorServ;

}

// 购买新的服务器，仅用于迁移中购买服务器
bool addServerForMig(VmIns& vmi) {
	Server info = getBestServerInfoForMig(vmi);
	if (info.name == "errorServ") return false;

	Server serv;
	copyServInfo(info, serv);
	serv.id = allServers.size();
	serv.buildDay = day;
	allServers.push_back(serv);

	SERVERCOST += serv.hardCost;
	dayServers[serv.name].push_back(serv.id);

	return true;
}

void migration() {
	int maxMigTimes = allVmcount * 3 / 120.;//考虑allVms是否包含已删除的Vm
	int actualMigTimes = 0;

	allServersBak.clear();
	allServersBak = allServers;
	//对全部购买的备份服务器进行排序
	std::sort(allServersBak.begin(), allServersBak.end(), sortServByVmCount);
	int frontIndex = 0;//存的是每台需要迁移出去虚拟机的服务器的ID
	int backIndex = allServersBak.size() - 1;
	//对每台服务器

#if  MIG_SWITCH & 0x0001
	while (frontIndex < allServersBak.size() - 1 && actualMigTimes < maxMigTimes) {
		//if (frontIndex == allServersBak.size() - 2) {
		//    int i = 1;
		//    fprintf(stderr, "\n");
		//}
		//如果当前服务器为空就寻找下一台服务器，空服务器一般都在前面
		if (allServersBak[frontIndex].vmCount == 0) {
			frontIndex++;
			continue;
		}
		//加一下限制，只迁移有一台和两台虚拟机的服务器，越往后面的迁移成功的概率越低，参数可以根据运行时间要求进行调节
		if (allServersBak[frontIndex].vmCount > 2) {
			break;
		}

		//对每台虚拟机
		vector<VmIns>& VmList = vmOnServ[allServersBak[frontIndex].id];//定义引用，减少查表的时间
		for (int i = 0; i < VmList.size(); i++) {
			//if (VmList[i].id == 193047621) {
			//    fprintf(stderr, " ");
			//}

			//双节点
			if (VmList[i].part == 0) {
				//对每台要迁入的服务器
				int bestServIndex = -1;
				double equalValue;
				double minEqualValue = Inf;
				for (backIndex = allServersBak.size() - 1; frontIndex < backIndex; backIndex--) {//除以2是为了减小计算次数防止超时
					Server& serv = allServers[allServersBak[backIndex].id];
					if (serv.cpuA >= VmList[i].cpu / 2 && serv.cpuB >= VmList[i].cpu / 2 &&
						serv.memA >= VmList[i].mem / 2 && serv.memB >= VmList[i].mem / 2) {
						int remainCpu = serv.cpuA - VmList[i].cpu / 2 + serv.cpuB - VmList[i].cpu / 2;
						int remainMem = serv.memA - VmList[i].mem / 2 + serv.memB - VmList[i].mem / 2;
						//equalValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
						equalValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
						if (equalValue < minEqualValue) {
							minEqualValue = equalValue;
							bestServIndex = backIndex;
						}
					}
				}

				if (bestServIndex == -1) {
					continue;//没有找到合适的服务器
				}
				else {
					//allServers中服务器资源占用，allServersBak[backIndex]先不考虑减少
					allServers[allServersBak[bestServIndex].id].cpuA -= VmList[i].cpu / 2;
					allServers[allServersBak[bestServIndex].id].cpuB -= VmList[i].cpu / 2;
					allServers[allServersBak[bestServIndex].id].memA -= VmList[i].mem / 2;
					allServers[allServersBak[bestServIndex].id].memB -= VmList[i].mem / 2;
					allServers[allServersBak[bestServIndex].id].vmCount++;

					//allServers中服务器资源释放
					allServers[allServersBak[frontIndex].id].cpuA += VmList[i].cpu / 2;
					allServers[allServersBak[frontIndex].id].cpuB += VmList[i].cpu / 2;
					allServers[allServersBak[frontIndex].id].memA += VmList[i].mem / 2;
					allServers[allServersBak[frontIndex].id].memB += VmList[i].mem / 2;
					allServers[allServersBak[frontIndex].id].vmCount--;

					//更新vmOnServ表
					VmIns tmpVm = VmList[i];
					vector<VmIns>& tmpVmList1 = vmOnServ[VmList[i].serverId];
					vector<VmIns>& tmpVmList2 = vmOnServ[allServersBak[bestServIndex].id];
					tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), VmList[i]), tmpVmList1.end());
					//更新虚拟机信息
					tmpVm.serverId = allServersBak[bestServIndex].id;//此处debug需要留意
					tmpVm.part = 0;//加不加都行，双节点
					tmpVmList2.push_back(tmpVm);
					//更新allVms
					allVms[tmpVm.id].serverId = tmpVm.serverId;//上面已经进行了更新
					allVms[tmpVm.id].part = tmpVm.part;//上面已经进行了更新
					//打印信息
					dayMigrationOutput.push_back("(" + to_string(tmpVm.id) + ", " + to_string(IdMap[allServersBak[bestServIndex].id]) + ")");

					//backIndex++;
					actualMigTimes++;
					i--;//因为Vmlist在迁入成功后少了一个成员，需要索引回退
					if (actualMigTimes > maxMigTimes - 1) {
						return;
					}
				}
			}
			//原虚拟机部署在A节点
			else if (VmList[i].part == 1) {
				int bestServIndex = -1;
				int part = -1;//1:A，2：B
				double equalValue;
				double minEqualValue = Inf;
				//查找最优的服务器，评价标准是剩余的资源等效价值最小
				for (backIndex = allServersBak.size() - 1; frontIndex < backIndex; backIndex--) {
					Server& serv = allServers[allServersBak[backIndex].id];
					if (serv.cpuA >= VmList[i].cpu && serv.memA >= VmList[i].mem) {
						int remainCpu = serv.cpuA - VmList[i].cpu;
						int remainMem = serv.memA - VmList[i].mem;
						equalValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
						if (equalValue < minEqualValue) {
							minEqualValue = equalValue;
							bestServIndex = backIndex;
							part = 1;
						}
					}

					if (serv.cpuB >= VmList[i].cpu && serv.memB >= VmList[i].mem) {
						int remainCpu = serv.cpuB - VmList[i].cpu;
						int remainMem = serv.memB - VmList[i].mem;
						equalValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
						if (equalValue < minEqualValue) {
							minEqualValue = equalValue;
							bestServIndex = backIndex;
							part = 2;
						}
					}
				}

				if (bestServIndex == -1) {
					continue;
				}
				else {
					if (part == 1) {
						//allServers中服务器资源占用，allServersBak[backIndex]先不考虑减少
						allServers[allServersBak[bestServIndex].id].cpuA -= VmList[i].cpu;
						allServers[allServersBak[bestServIndex].id].memA -= VmList[i].mem;
						allServers[allServersBak[bestServIndex].id].vmCount++;
						//allServers中服务器资源释放
						allServers[allServersBak[frontIndex].id].cpuA += VmList[i].cpu;
						allServers[allServersBak[frontIndex].id].memA += VmList[i].mem;
						allServers[allServersBak[frontIndex].id].vmCount--;


						//更新vmOnServ表
						VmIns tmpVm = VmList[i];
						vector<VmIns>& tmpVmList1 = vmOnServ[VmList[i].serverId];
						vector<VmIns>& tmpVmList2 = vmOnServ[allServersBak[bestServIndex].id];
						tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), VmList[i]), tmpVmList1.end());
						//更新虚拟机信息
						tmpVm.serverId = allServersBak[bestServIndex].id;//此处debug需要留意
						tmpVm.part = 1;//
						tmpVmList2.push_back(tmpVm);
						allVms[tmpVm.id].serverId = tmpVm.serverId;//上面已经进行了更新
						allVms[tmpVm.id].part = tmpVm.part;//上面已经进行了更新
						//打印信息
						dayMigrationOutput.push_back("(" + to_string(tmpVm.id) + ", " + to_string(IdMap[allServersBak[bestServIndex].id]) + ", A)");

						//backIndex++;
						actualMigTimes++;
						i--;//因为Vmlist在迁入成功后少了一个成员，需要索引回退
						if (actualMigTimes > maxMigTimes - 1) {
							return;
						}
					}
					else if (part == 2) {
						//allServers中服务器资源占用，allServersBak[backIndex]先不考虑减少
						allServers[allServersBak[bestServIndex].id].cpuB -= VmList[i].cpu;
						allServers[allServersBak[bestServIndex].id].memB -= VmList[i].mem;

						allServers[allServersBak[bestServIndex].id].vmCount++;
						//allServers中服务器资源释放
						allServers[allServersBak[frontIndex].id].cpuA += VmList[i].cpu;
						allServers[allServersBak[frontIndex].id].memA += VmList[i].mem;
						allServers[allServersBak[frontIndex].id].vmCount--;

						//更新vmOnServ表
						VmIns tmpVm = VmList[i];
						vector<VmIns>& tmpVmList1 = vmOnServ[VmList[i].serverId];
						vector<VmIns>& tmpVmList2 = vmOnServ[allServersBak[bestServIndex].id];
						tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), VmList[i]), tmpVmList1.end());
						//更新虚拟机信息
						tmpVm.serverId = allServersBak[bestServIndex].id;//此处debug需要留意
						tmpVm.part = 2;//
						tmpVmList2.push_back(tmpVm);
						allVms[tmpVm.id].serverId = tmpVm.serverId;//上面已经进行了更新
						allVms[tmpVm.id].part = tmpVm.part;//上面已经进行了更新
						//打印信息
						dayMigrationOutput.push_back("(" + to_string(tmpVm.id) + ", " + to_string(IdMap[allServersBak[bestServIndex].id]) + ", B)");

						//backIndex++;
						actualMigTimes++;
						i--;//因为Vmlist在迁入成功后少了一个成员，需要索引回退
						if (actualMigTimes > maxMigTimes - 1) {
							return;
						}
					}
					else {
						cout << "migration error!" << endl;
						exit(-1);
					}
				}

			}
			else if (VmList[i].part == 2) {
				int bestServIndex = -1;
				int part = -1;//1:A，2：B
				double equalValue;
				double minEqualValue = Inf;
				for (backIndex = allServersBak.size() - 1; frontIndex < backIndex; backIndex--) {
					Server& serv = allServers[allServersBak[backIndex].id];
					//准备部署在A节点
					if (serv.cpuA >= VmList[i].cpu && serv.memA >= VmList[i].mem) {
						int remainCpu = serv.cpuA - VmList[i].cpu;
						int remainMem = serv.memA - VmList[i].mem;
						equalValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
						if (equalValue < minEqualValue) {
							minEqualValue = equalValue;
							bestServIndex = backIndex;
							part = 1;
						}
					}

					if (serv.cpuB >= VmList[i].cpu && serv.memB >= VmList[i].mem) {
						int remainCpu = serv.cpuB - VmList[i].cpu;
						int remainMem = serv.memB - VmList[i].mem;
						equalValue = sqrt(pow(cpuPercent * remainCpu, 2) + pow(remainMem, 2));
						if (equalValue < minEqualValue) {
							minEqualValue = equalValue;
							bestServIndex = backIndex;
							part = 2;
						}
					}
				}
				if (bestServIndex == -1) {
					continue;
				}
				else {
					if (part == 1) {
						//allServers中新服务器资源占用，allServersBak[backIndex]先不考虑减少
						allServers[allServersBak[bestServIndex].id].cpuA -= VmList[i].cpu;
						allServers[allServersBak[bestServIndex].id].memA -= VmList[i].mem;
						allServers[allServersBak[bestServIndex].id].vmCount++;
						//allServers中原服务器资源释放
						allServers[allServersBak[frontIndex].id].cpuB += VmList[i].cpu;
						allServers[allServersBak[frontIndex].id].memB += VmList[i].mem;
						allServers[allServersBak[frontIndex].id].vmCount--;


						//更新vmOnServ表
						VmIns tmpVm = VmList[i];
						vector<VmIns>& tmpVmList1 = vmOnServ[VmList[i].serverId];
						vector<VmIns>& tmpVmList2 = vmOnServ[allServersBak[bestServIndex].id];
						tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), VmList[i]), tmpVmList1.end());
						//更新虚拟机信息
						tmpVm.serverId = allServersBak[bestServIndex].id;//此处debug需要留意
						tmpVm.part = 1;//
						tmpVmList2.push_back(tmpVm);
						allVms[tmpVm.id].serverId = tmpVm.serverId;//上面已经进行了更新
						allVms[tmpVm.id].part = tmpVm.part;//上面已经进行了更新
						//打印信息
						dayMigrationOutput.push_back("(" + to_string(tmpVm.id) + ", " + to_string(IdMap[allServersBak[bestServIndex].id]) + ", A)");

						//backIndex++;
						actualMigTimes++;
						i--;//因为Vmlist在迁入成功后少了一个成员，需要索引回退
						if (actualMigTimes > maxMigTimes - 1) {
							return;
						}
					}
					else if (part == 2) {
						//allServers中服务器资源占用，allServersBak[backIndex]先不考虑减少
						allServers[allServersBak[bestServIndex].id].cpuB -= VmList[i].cpu;
						allServers[allServersBak[bestServIndex].id].memB -= VmList[i].mem;

						allServers[allServersBak[bestServIndex].id].vmCount++;
						//allServers中服务器资源释放
						allServers[allServersBak[frontIndex].id].cpuB += VmList[i].cpu;
						allServers[allServersBak[frontIndex].id].memB += VmList[i].mem;
						allServers[allServersBak[frontIndex].id].vmCount--;

						//更新vmOnServ表
						VmIns tmpVm = VmList[i];
						vector<VmIns>& tmpVmList1 = vmOnServ[VmList[i].serverId];
						vector<VmIns>& tmpVmList2 = vmOnServ[allServersBak[bestServIndex].id];
						tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), VmList[i]), tmpVmList1.end());
						//更新虚拟机信息
						tmpVm.serverId = allServersBak[bestServIndex].id;//此处debug需要留意
						tmpVm.part = 2;//
						tmpVmList2.push_back(tmpVm);
						allVms[tmpVm.id].serverId = tmpVm.serverId;//上面已经进行了更新
						allVms[tmpVm.id].part = tmpVm.part;//上面已经进行了更新
						//打印信息
						dayMigrationOutput.push_back("(" + to_string(tmpVm.id) + ", " + to_string(IdMap[allServersBak[bestServIndex].id]) + ", B)");
						//backIndex++;
						actualMigTimes++;
						i--;//因为Vmlist在迁入成功后少了一个成员，需要索引回退
						if (actualMigTimes > maxMigTimes - 1) {
							return;
						}
					}
				}
			}
		}
		frontIndex++;
	}
#endif //  MIG_SWITCH

	int i = 0;
	int j = 0;

#if MIG_SWITCH & 0x0010
	//第二轮单节点的迁移
	vector<ServerSingle> servSingleList;
	for (auto serv : allServers) {
		servSingleList.push_back({ serv.name,serv.cpuA,serv.memA,serv.id ,1 });//A节点的信息
		servSingleList.push_back({ serv.name,serv.cpuB,serv.memB,serv.id ,2 });//B节点的信息
	}

	//排序完成后前面的剩余的CPU多，后面的剩余的内存多，前后进行交换
	std::sort(servSingleList.begin(), servSingleList.end(), sortServSingleListByCpuMinusMem);


	i = 0;//tmpVmlist1的索引
	j = 0; //tmpVmlist2的索引
	frontIndex = 0;
	backIndex = servSingleList.size() - 1;

	//if (day == 607) {
	//    day = day;
	//    cout << day << endl;
	//}

	while (frontIndex < backIndex && actualMigTimes < maxMigTimes - 3) {

		//再继续换下去没有意义了，甚至会造成副作用
		if ((servSingleList[frontIndex].cpu - servSingleList[frontIndex].mem < 0) || (servSingleList[backIndex].mem - servSingleList[backIndex].cpu < 0)) {
			break;
		}

		double cpuRemainRate1 = (double)servSingleList[frontIndex].cpu / allServers[servSingleList[frontIndex].id].allcpu;
		double memRemainRate1 = (double)servSingleList[frontIndex].mem / allServers[servSingleList[frontIndex].id].allmem;
		double cpuRemainRate2 = (double)servSingleList[backIndex].cpu / allServers[servSingleList[backIndex].id].allcpu;
		double memRemainRate2 = (double)servSingleList[backIndex].mem / allServers[servSingleList[backIndex].id].allmem;
		if (cpuRemainRate1 < cpuRemainRateBase2 && memRemainRate1 < memRemainRateBase2) {
			frontIndex++;
			continue;
		}
		if (cpuRemainRate2 < cpuRemainRateBase2 && memRemainRate2 < memRemainRateBase2) {
			backIndex--;
			continue;
		}

		if (servSingleList[frontIndex].cpu < 5 && servSingleList[frontIndex].mem < 5) {
			i = 0;
			frontIndex++;
			continue;
		}

		if (servSingleList[backIndex].cpu < 5 && servSingleList[backIndex].mem < 5) {
			j = 0;
			backIndex--;
			continue;
		}


		//同一服务器两个节点相互迁移有bug，先不迁移这种情况
		if (servSingleList[frontIndex].id == servSingleList[backIndex].id) {
			backIndex--;
			continue;
		}

		vector<VmIns>& tmpVmList1 = vmOnServ[servSingleList[frontIndex].id];//找到servSingleList中frontIndex指向的服务器节点对应的虚拟机列表，不明白可以看一下vmOnServ的定义
		vector<VmIns>& tmpVmList2 = vmOnServ[servSingleList[backIndex].id];//找到servSingleList中backIndex指向的服务器节点对应的虚拟机列表，不明白可以看一下vmOnServ的定义

		std::sort(tmpVmList1.begin(), tmpVmList1.end(), sortVmListByMemMinusCpu);//按内存-CPU进行排序，前面的占用的内存较多
		std::sort(tmpVmList2.begin(), tmpVmList2.end(), sortVmListByCpuMinusMem);//按CPU-内存进行排序，前面的占用的CPU较多

		//找一个符合交换条件的虚拟机

		for (; i < tmpVmList1.size(); i++) {
			if (tmpVmList1[i].part != servSingleList[frontIndex].part) continue;
			//if (tmpVmList1[i].mem - tmpVmList1[i].cpu < 5) continue;//内存比CPU多不了太多，交换的意义不大，这个5可以根据情况调节一下
			else break;//找到一个合适的，退出循环
		}
		//没有找到
		if (i >= tmpVmList1.size()) {
			frontIndex++;
			i = 0;
			continue;
		}
		//找一个符合交换条件的虚拟机

		for (; j < tmpVmList2.size(); j++) {
			if (tmpVmList2[j].part != servSingleList[backIndex].part) continue;
			//if (tmpVmList2[j].cpu - tmpVmList2[j].mem < 5) continue;//CPU比内存多不了太多，交换的意义不大，这个5可以根据情况调节一下
			else break;//找到一个合适的，退出循环
		}
		//没有找到
		if (j >= tmpVmList2.size()) {
			backIndex--;
			j = 0;
			continue;
		}

		//同型号的不交换
		if (tmpVmList1[i].name == tmpVmList2[j].name) {
			i++;
			continue;
		}


		//到这里已经找到了两个符合交换条件的虚拟机！

		//判断一下交换后能不能放得下
		int serv1RemainCpu, serv1RemainMem, serv2RemainCpu, serv2RemainMem;//交换后剩余的资源
		if (tmpVmList1[i].part == 1) {//先判断是在服务器的哪个节点上
			serv1RemainCpu = allServers[tmpVmList1[i].serverId].cpuA + tmpVmList1[i].cpu - tmpVmList2[j].cpu;
			serv1RemainMem = allServers[tmpVmList1[i].serverId].memA + tmpVmList1[i].mem - tmpVmList2[j].mem;
		}
		else {
			serv1RemainCpu = allServers[tmpVmList1[i].serverId].cpuB + tmpVmList1[i].cpu - tmpVmList2[j].cpu;
			serv1RemainMem = allServers[tmpVmList1[i].serverId].memB + tmpVmList1[i].mem - tmpVmList2[j].mem;
		}

		if (tmpVmList2[j].part == 1) {
			serv2RemainCpu = allServers[tmpVmList2[j].serverId].cpuA + tmpVmList2[j].cpu - tmpVmList1[i].cpu;
			serv2RemainMem = allServers[tmpVmList2[j].serverId].memA + tmpVmList2[j].mem - tmpVmList1[i].mem;
		}
		else {
			serv2RemainCpu = allServers[tmpVmList2[j].serverId].cpuB + tmpVmList2[j].cpu - tmpVmList1[i].cpu;
			serv2RemainMem = allServers[tmpVmList2[j].serverId].memB + tmpVmList2[j].mem - tmpVmList1[i].mem;
		}

		//资源不足，放不下,serv1资源超了说明从serv2中迁移过来的虚拟机需求的资源较多，换下一台试试
		if (serv1RemainCpu < 0 || serv1RemainMem < 0) {
			j++;//换下一台虚拟机试试
			continue;
		}

		//资源不足，放不下
		if (serv2RemainCpu < 0 || serv2RemainMem < 0) {
			i++;//换下一台虚拟机试试
			continue;
		}

		//判断交换后是否起到正向作用
		double serv1BalanceIndexBefore = 0;
		double serv2BalanceIndexBefore = 0;
		double serv1BalanceIndexAfter = 0;
		double serv2BalanceIndexAfter = 0;
		if (tmpVmList1[i].part == 1) {//先判断是在服务器的哪个节点上
			serv1BalanceIndexBefore = abs(log((allServers[tmpVmList1[i].serverId].cpuA * cpuPercent + 1) / (allServers[tmpVmList1[i].serverId].memA + 1.0)));
		}
		else {
			serv1BalanceIndexBefore = abs(log((allServers[tmpVmList1[i].serverId].cpuB * cpuPercent + 1) / (allServers[tmpVmList1[i].serverId].memB + 1.0)));
		}

		if (tmpVmList2[j].part == 1) {
			serv2BalanceIndexBefore = abs(log((allServers[tmpVmList2[j].serverId].cpuA * cpuPercent + 1) / (allServers[tmpVmList2[j].serverId].memA + 1.0)));
		}
		else {
			serv2BalanceIndexBefore = abs(log((allServers[tmpVmList2[j].serverId].cpuB * cpuPercent + 1) / (allServers[tmpVmList2[j].serverId].memB + 1.0)));
		}

		serv1BalanceIndexAfter = abs(log((serv1RemainCpu * cpuPercent + 1) / (serv1RemainMem + 1.0)));
		serv2BalanceIndexAfter = abs(log((serv2RemainCpu * cpuPercent + 1) / (serv2RemainMem + 1.0)));

		if (serv1BalanceIndexBefore + serv2BalanceIndexBefore <= serv1BalanceIndexAfter + serv2BalanceIndexAfter) {
			if (rand() % 100 < 50) i++;
			else j++;
			continue;
		}


		//两台服务器的资源均允许进行迁移
		//首先应该找一台中转服务器

		int maxNeedCpu = max(tmpVmList1[i].cpu, tmpVmList2[j].cpu);//中转服务器剩余的资源应当比这两个数还要大
		int maxNeedMem = max(tmpVmList1[i].mem, tmpVmList2[j].mem);

		int k;
		int part = -1;
		unordered_map<int, int> sleepServListId;//睡眠的服务器列表ID和part
		for (k = allServers.size() - 1; k >= 0; k--) {//从后往前找，一般后面的是昨天刚买的服务器，可能会有空余的
			if (allServers[k].cpuA >= maxNeedCpu && allServers[k].memA >= maxNeedMem) {

				//先排除中转服务器是这两个待迁移的服务器
				if (allServers[k].id == tmpVmList1[0].serverId || allServers[k].id == tmpVmList2[0].serverId) {
					continue;
				}
				else {
					part = 1;
					break;//不是睡眠的，退出循环，表示找到了
				}
			}
			else if (allServers[k].cpuB >= maxNeedCpu && allServers[k].memB >= maxNeedMem) {

				//先排除中转服务器是这两个待迁移的服务器
				if (allServers[k].id == tmpVmList1[0].serverId || allServers[k].id == tmpVmList2[0].serverId) {
					continue;
				}

				if (allServers[k].vmCount == 0) {//判断一下是不是睡眠的服务器，如果是睡眠的服务器可以先不用这一台，可能会省一点电费
					sleepServListId[k] = 2;
					continue;
				}
				else {
					part = 2;
					break;//不是睡眠的，退出循环，表示找到了
				}
			}
		}
		if (k == -1) {//无论睡眠的还是不睡眠的服务器都不能满足需求，那就买一台新服务器，也不算浪费，后面的部署虚拟机也能用
			VmIns vmi;
			vmi.cpu = maxNeedCpu;
			vmi.mem = maxNeedMem;
			if (!addServerForMig(vmi)) {//买一台新服务器，因为这个vmi是虚构的，可能没有任何一台服务器能够放得下，新买的服务器有一个问题：尚未加入idMap表，但是下面要用，需要特殊处理
				//购买失败，换另一组
				i++;
				j++;
				continue;
			}
			k = allServers.size() - 1;//购买成功，新服务器的id为最后一个
			part = 1;//哪个节点都一样
		}

		if (tmpVmList1[i].part == 1 && tmpVmList2[j].part == 1) {//两个虚拟机都在A节点
			//实际上不需要操作往中转服务器迁移的操作，因为程序内部迁不迁判题器是不知道的，只要给他一个输出就行了，判题器就认为迁了，但是中转服务器的空间一定要足够
			if (allServers[tmpVmList1[i].serverId].cpuA > 1000) {
				int a = 0;
			}

			if (debug == 1)
				fprintf(stderr, "before: CPU1-%d MEM1-%d   CPU2-%d MEM2-%d\n", allServers[tmpVmList1[i].serverId].cpuA, allServers[tmpVmList1[i].serverId].memA,
					allServers[tmpVmList2[j].serverId].cpuA, allServers[tmpVmList2[j].serverId].memA);

			allServers[tmpVmList1[i].serverId].cpuA += (tmpVmList1[i].cpu - tmpVmList2[j].cpu);
			allServers[tmpVmList1[i].serverId].memA += (tmpVmList1[i].mem - tmpVmList2[j].mem);
			allServers[tmpVmList2[j].serverId].cpuA += (tmpVmList2[j].cpu - tmpVmList1[i].cpu);
			allServers[tmpVmList2[j].serverId].memA += (tmpVmList2[j].mem - tmpVmList1[i].mem);

			if (debug == 1)
				fprintf(stderr, "after: CPU1-%d MEM1-%d   CPU2-%d MEM2-%d\n", allServers[tmpVmList1[i].serverId].cpuA, allServers[tmpVmList1[i].serverId].memA,
					allServers[tmpVmList2[j].serverId].cpuA, allServers[tmpVmList2[j].serverId].memA);

			if (allServers[tmpVmList1[i].serverId].cpuA == 65 && allServers[tmpVmList1[i].serverId].memA == 1) {
				int a = 5;
			}

			//更新vmOnServ表
			VmIns tmpVm1 = tmpVmList1[i];
			VmIns tmpVm2 = tmpVmList2[j];

			//打印迁移结果
			string strPart = part == 1 ? ", A)" : ", B)";
			dayMigrationOutput.push_back("0, (" + to_string(tmpVm1.id) + ", " + to_string(allServers[k].id) + strPart);//先将1迁移到中转服务器，注意由于出现了上面购买服务器时ID未映射的问题，加一个前面加一个0表示未映射，最后处理
			dayMigrationOutput.push_back("(" + to_string(tmpVm2.id) + ", " + to_string(IdMap[tmpVm1.serverId]) + ", A)");//将2迁移到1
			dayMigrationOutput.push_back("(" + to_string(tmpVm1.id) + ", " + to_string(IdMap[tmpVm2.serverId]) + ", A)");//将中转服务器中的虚拟机迁移到2

			tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), tmpVmList1[i]), tmpVmList1.end());
			tmpVmList2.erase(remove(tmpVmList2.begin(), tmpVmList2.end(), tmpVmList2[j]), tmpVmList2.end());

			swap(tmpVm1.serverId, tmpVm2.serverId);
			swap(tmpVm1.part, tmpVm2.part);

			tmpVmList1.insert(tmpVmList1.begin(), tmpVm2);
			tmpVmList2.insert(tmpVmList2.begin(), tmpVm1);

			//更新allVms
			allVms[tmpVm1.id].serverId = tmpVm1.serverId;//上面已经进行了更新
			allVms[tmpVm2.id].serverId = tmpVm2.serverId;//上面已经进行了更新
			allVms[tmpVm1.id].part = tmpVm1.part;//上面已经进行了更新
			allVms[tmpVm2.id].part = tmpVm2.part;//上面已经进行了更新

			actualMigTimes += 3;
			if (actualMigTimes > maxMigTimes - 3) {//必须-3,能保证下一次还能迁移，如果不-3，等发现超了就晚了
				return;
			}
		}
		else if (tmpVmList1[i].part == 1 && tmpVmList2[j].part == 2) {
			//实际上不需要操作往中转服务器迁移的操作，因为程序内部迁不迁判题器是不知道的，只要给他一个输出就行了，判题器就认为迁了，但是中转服务器的空间一定要足够


			if (allServers[tmpVmList1[i].serverId].cpuA == 65 && allServers[tmpVmList1[i].serverId].memA == 1) {
				int a = 5;
			}

			if (debug == 1)
				fprintf(stderr, "before: CPU1-%d MEM1-%d   CPU2-%d MEM2-%d\n", allServers[tmpVmList1[i].serverId].cpuA, allServers[tmpVmList1[i].serverId].memA,
					allServers[tmpVmList2[j].serverId].cpuB, allServers[tmpVmList2[j].serverId].memB);

			allServers[tmpVmList1[i].serverId].cpuA += (tmpVmList1[i].cpu - tmpVmList2[j].cpu);
			allServers[tmpVmList1[i].serverId].memA += (tmpVmList1[i].mem - tmpVmList2[j].mem);
			allServers[tmpVmList2[j].serverId].cpuB += (tmpVmList2[j].cpu - tmpVmList1[i].cpu);
			allServers[tmpVmList2[j].serverId].memB += (tmpVmList2[j].mem - tmpVmList1[i].mem);

			if (debug == 1)
				fprintf(stderr, "after: CPU1-%d MEM1-%d   CPU2-%d MEM2-%d\n", allServers[tmpVmList1[i].serverId].cpuA, allServers[tmpVmList1[i].serverId].memA,
					allServers[tmpVmList2[j].serverId].cpuB, allServers[tmpVmList2[j].serverId].memB);

			if (allServers[tmpVmList1[i].serverId].cpuA == 65 && allServers[tmpVmList1[i].serverId].memA == 1) {
				int a = 5;
			}

			VmIns tmpVm1 = tmpVmList1[i];
			VmIns tmpVm2 = tmpVmList2[j];

			//打印迁移结果
			string strPart = part == 1 ? ", A)" : ", B)";
			dayMigrationOutput.push_back("0, (" + to_string(tmpVm1.id) + ", " + to_string(allServers[k].id) + strPart);//先将1迁移到中转服务器，注意由于出现了上面购买服务器时ID未映射的问题，加一个前面加一个0表示未映射，最后处理
			dayMigrationOutput.push_back("(" + to_string(tmpVm2.id) + ", " + to_string(IdMap[tmpVm1.serverId]) + ", A)");//将2迁移到1
			dayMigrationOutput.push_back("(" + to_string(tmpVm1.id) + ", " + to_string(IdMap[tmpVm2.serverId]) + ", B)");//将中转服务器中的虚拟机迁移到2

			//更新vmOnServ表
			tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), tmpVmList1[i]), tmpVmList1.end());
			tmpVmList2.erase(remove(tmpVmList2.begin(), tmpVmList2.end(), tmpVmList2[j]), tmpVmList2.end());

			swap(tmpVm1.serverId, tmpVm2.serverId);
			swap(tmpVm1.part, tmpVm2.part);

			tmpVmList1.insert(tmpVmList1.begin(), tmpVm2);
			tmpVmList2.insert(tmpVmList2.begin(), tmpVm1);

			//更新allVms
			allVms[tmpVm1.id].serverId = tmpVm1.serverId;//上面已经进行了更新
			allVms[tmpVm2.id].serverId = tmpVm2.serverId;//上面已经进行了更新
			allVms[tmpVm1.id].part = tmpVm1.part;//上面已经进行了更新
			allVms[tmpVm2.id].part = tmpVm2.part;//上面已经进行了更新

			actualMigTimes += 3;
			if (actualMigTimes > maxMigTimes - 3) {//必须-3,能保证下一次还能迁移，如果不-3，等发现超了就晚了
				return;
			}
		}
		else if (tmpVmList1[i].part == 2 && tmpVmList2[j].part == 1) {//两个虚拟机都在A节点
			//实际上不需要操作往中转服务器迁移的操作，因为程序内部迁不迁判题器是不知道的，只要给他一个输出就行了，判题器就认为迁了，但是中转服务器的空间一定要足够

			if (debug == 1)
				fprintf(stderr, "before: CPU1-%d MEM1-%d   CPU2-%d MEM2-%d\n", allServers[tmpVmList1[i].serverId].cpuB, allServers[tmpVmList1[i].serverId].memB,
					allServers[tmpVmList2[j].serverId].cpuA, allServers[tmpVmList2[j].serverId].memA);
			allServers[tmpVmList1[i].serverId].cpuB += (tmpVmList1[i].cpu - tmpVmList2[j].cpu);
			allServers[tmpVmList1[i].serverId].memB += (tmpVmList1[i].mem - tmpVmList2[j].mem);
			allServers[tmpVmList2[j].serverId].cpuA += (tmpVmList2[j].cpu - tmpVmList1[i].cpu);
			allServers[tmpVmList2[j].serverId].memA += (tmpVmList2[j].mem - tmpVmList1[i].mem);

			if (debug == 1)
				fprintf(stderr, "after: CPU1-%d MEM1-%d   CPU2-%d MEM2-%d\n", allServers[tmpVmList1[i].serverId].cpuB, allServers[tmpVmList1[i].serverId].memB,
					allServers[tmpVmList2[j].serverId].cpuA, allServers[tmpVmList2[j].serverId].memA);

			//更新vmOnServ表
			VmIns tmpVm1 = tmpVmList1[i];
			VmIns tmpVm2 = tmpVmList2[j];

			//打印迁移结果
			string strPart = part == 1 ? ", A)" : ", B)";
			dayMigrationOutput.push_back("0, (" + to_string(tmpVm1.id) + ", " + to_string(allServers[k].id) + strPart);//先将1迁移到中转服务器，注意由于出现了上面购买服务器时ID未映射的问题，加一个前面加一个0表示未映射，最后处理
			dayMigrationOutput.push_back("(" + to_string(tmpVm2.id) + ", " + to_string(IdMap[tmpVm1.serverId]) + ", B)");//将2迁移到1
			dayMigrationOutput.push_back("(" + to_string(tmpVm1.id) + ", " + to_string(IdMap[tmpVm2.serverId]) + ", A)");//将中转服务器中的虚拟机迁移到2

			tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), tmpVmList1[i]), tmpVmList1.end());
			tmpVmList2.erase(remove(tmpVmList2.begin(), tmpVmList2.end(), tmpVmList2[j]), tmpVmList2.end());

			swap(tmpVm1.serverId, tmpVm2.serverId);
			swap(tmpVm1.part, tmpVm2.part);

			tmpVmList1.insert(tmpVmList1.begin(), tmpVm2);
			tmpVmList2.insert(tmpVmList2.begin(), tmpVm1);

			//更新allVms
			allVms[tmpVm1.id].serverId = tmpVm1.serverId;//上面已经进行了更新
			allVms[tmpVm2.id].serverId = tmpVm2.serverId;//上面已经进行了更新
			allVms[tmpVm1.id].part = tmpVm1.part;//上面已经进行了更新
			allVms[tmpVm2.id].part = tmpVm2.part;//上面已经进行了更新

			actualMigTimes += 3;
			if (actualMigTimes > maxMigTimes - 3) {//必须-3,能保证下一次还能迁移，如果不-3，等发现超了就晚了
				return;
			}
		}
		else if (tmpVmList1[i].part == 2 && tmpVmList2[j].part == 2) {//两个虚拟机都在A节点
			//实际上不需要操作往中转服务器迁移的操作，因为程序内部迁不迁判题器是不知道的，只要给他一个输出就行了，判题器就认为迁了，但是中转服务器的空间一定要足够

			if (debug == 1)
				fprintf(stderr, "before: CPU1-%d MEM1-%d   CPU2-%d MEM2-%d\n", allServers[tmpVmList1[i].serverId].cpuB, allServers[tmpVmList1[i].serverId].memB,
					allServers[tmpVmList2[j].serverId].cpuB, allServers[tmpVmList2[j].serverId].memB);
			allServers[tmpVmList1[i].serverId].cpuB += (tmpVmList1[i].cpu - tmpVmList2[j].cpu);
			allServers[tmpVmList1[i].serverId].memB += (tmpVmList1[i].mem - tmpVmList2[j].mem);
			allServers[tmpVmList2[j].serverId].cpuB += (tmpVmList2[j].cpu - tmpVmList1[i].cpu);
			allServers[tmpVmList2[j].serverId].memB += (tmpVmList2[j].mem - tmpVmList1[i].mem);

			if (debug == 1)
				fprintf(stderr, "adfer: CPU1-%d MEM1-%d   CPU2-%d MEM2-%d\n", allServers[tmpVmList1[i].serverId].cpuB, allServers[tmpVmList1[i].serverId].memB,
					allServers[tmpVmList2[j].serverId].cpuB, allServers[tmpVmList2[j].serverId].memB);


			//更新vmOnServ表
			VmIns tmpVm1 = tmpVmList1[i];
			VmIns tmpVm2 = tmpVmList2[j];

			//打印迁移结果
			string strPart = part == 1 ? ", A)" : ", B)";
			dayMigrationOutput.push_back("0, (" + to_string(tmpVm1.id) + ", " + to_string(allServers[k].id) + strPart);//先将1迁移到中转服务器，注意由于出现了上面购买服务器时ID未映射的问题，加一个前面加一个0表示未映射，最后处理
			dayMigrationOutput.push_back("(" + to_string(tmpVm2.id) + ", " + to_string(IdMap[tmpVm1.serverId]) + ", B)");//将2迁移到1
			dayMigrationOutput.push_back("(" + to_string(tmpVm1.id) + ", " + to_string(IdMap[tmpVm2.serverId]) + ", B)");//将中转服务器中的虚拟机迁移到2

			tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), tmpVmList1[i]), tmpVmList1.end());
			tmpVmList2.erase(remove(tmpVmList2.begin(), tmpVmList2.end(), tmpVmList2[j]), tmpVmList2.end());

			swap(tmpVm1.serverId, tmpVm2.serverId);
			swap(tmpVm1.part, tmpVm2.part);

			tmpVmList1.insert(tmpVmList1.begin(), tmpVm2);
			tmpVmList2.insert(tmpVmList2.begin(), tmpVm1);

			//更新allVms
			allVms[tmpVm1.id].serverId = tmpVm1.serverId;//上面已经进行了更新
			allVms[tmpVm2.id].serverId = tmpVm2.serverId;//上面已经进行了更新
			allVms[tmpVm1.id].part = tmpVm1.part;//上面已经进行了更新
			allVms[tmpVm2.id].part = tmpVm2.part;//上面已经进行了更新

			actualMigTimes += 3;
			if (actualMigTimes > maxMigTimes - 3) {//必须-3,能保证下一次还能迁移，如果不-3，等发现超了就晚了
				return;
			}
		}
		else {
			if (debug == 1)
				fprintf(stderr, "migration error!\n");
			exit(1);
		}

		i++;
		j++;

		if (i == tmpVmList1.size()) {
			i = 0;
			frontIndex++;
		}
		if (j == tmpVmList2.size()) {
			j = 0;
			backIndex--;
		}


		if (debug == 1)
			fprintf(stderr, "mig\n");
	}
	if (debug == 1)
		fprintf(stderr, "day:%d\n\n", day);
#endif

	//第三轮迁移，以使服务器资源剩余量最小为目标，两层循环
#if MIG_SWITCH & 0x0100
	i = 0;
	for (auto& eachPair : allVms) {
		//VmIns& eachVmi = allVms[vmId];
		VmIns& eachVmi = eachPair.second;
		if (eachVmi.isOnline == false) continue;//已经删除的虚拟机

		i++;
		if (i % 20 != 0) continue;//减小工作量

		double equalRemainRes;//等效的剩余资源
		double cpuRemainRate;//cpu的剩余率
		double memRemainRate;//内存的剩余率

		Server& serv = allServers[eachVmi.serverId];//引用当前虚拟机所在的服务器
		if (eachVmi.part == 0) {//双节点
			//double cpuPercent = 2.5;
			equalRemainRes = (serv.cpuA + serv.cpuB) * cpuPercent + (serv.memA + serv.memB);//计算当前服务器剩余的资源
			cpuRemainRate = (serv.cpuA + serv.cpuB) / (serv.allcpu * 1.0);
			memRemainRate = (serv.memA + serv.memB) / (serv.allmem * 1.0);
			//if (serv.cpuA + serv.cpuB < 10 && serv.memA + serv.memB < 10) continue;
		}
		else if (eachVmi.part == 1) {//A节点
			equalRemainRes = (serv.cpuA) * cpuPercent + (serv.memA);//计算当前服务器剩余的资源
			cpuRemainRate = (serv.cpuA) / (serv.allcpu * 0.5);
			memRemainRate = (serv.memA) / (serv.allmem * 0.5);
			//if (serv.cpuA < 5 && serv.memA < 5) continue;
		}
		else if (eachVmi.part == 2) {//B节点
			equalRemainRes = (serv.cpuB) * cpuPercent + (serv.memB);//计算当前服务器剩余的资源
			cpuRemainRate = (serv.cpuB) / (serv.allcpu * 0.5);
			memRemainRate = (serv.memB) / (serv.allmem * 0.5);
			//if (serv.cpuB < 5 && serv.memB < 5) continue;
		}
		else {//正常不会到这里的
			if (debug == 1)
				fprintf(stderr, "migration error!\n");
			exit(-1);
		}

		if (cpuRemainRate < cpuRemainRateBase && memRemainRate < memRemainRateBase) continue;//当前的服务器利用率很好，不需要迁移

		int bestServId = -1;
		int part = -1;
		//开始遍历服务器，找到最合适的服务器
		for (int servId = 0; servId < allServers.size() && actualMigTimes < maxMigTimes - 1; servId++) {
			Server& eachServ = allServers[servId];
			if (eachServ.vmCount == 0) continue;//不迁移到空闲的服务器上去
			if (eachServ.id == serv.id) continue;//源服务器与目的服务器相同跳过
			if (eachVmi.part == 0) {//双节点
				if (eachServ.cpuA >= eachVmi.cpu / 2 && eachServ.cpuB >= eachVmi.cpu / 2 &&
					eachServ.memA >= eachVmi.mem / 2 && eachServ.memB >= eachVmi.mem / 2) {

					double tmpEqualRemainRes = (eachServ.cpuA + eachServ.cpuB - eachVmi.cpu) * cpuPercent + (eachServ.memA + eachServ.memB - eachVmi.mem);//计算当前服务器剩余的资源

					if (tmpEqualRemainRes < equalRemainRes) {
						equalRemainRes = tmpEqualRemainRes;
						bestServId = servId;
						part = 0;
					}
				}
			}
			else if (eachVmi.part == 1 || eachVmi.part == 2) {
				if (eachServ.cpuA >= eachVmi.cpu && eachServ.memA >= eachVmi.mem) {

					double tmpEqualRemainRes = (eachServ.cpuA - eachVmi.cpu) * cpuPercent + (eachServ.memA - eachVmi.mem);//计算当前服务器剩余的资源

					if (tmpEqualRemainRes < equalRemainRes) {
						equalRemainRes = tmpEqualRemainRes;
						bestServId = servId;
						part = 1;
					}
				}
				if (eachServ.cpuB >= eachVmi.cpu && eachServ.memB >= eachVmi.mem) {

					double tmpEqualRemainRes = (eachServ.cpuB - eachVmi.cpu) * cpuPercent + (eachServ.memB - eachVmi.mem);//计算当前服务器剩余的资源

					if (tmpEqualRemainRes < equalRemainRes) {
						equalRemainRes = tmpEqualRemainRes;
						bestServId = servId;
						part = 2;
					}
				}
			}
			else {
				if (debug == 1) fprintf(stderr, "error!\n");
			}//出错
		}


		//开始放置
		if (part == -1) {//没有找到合适的服务器，不迁移
			continue;
		}
		else if (part == 0) {
			allServers[bestServId].cpuA -= eachVmi.cpu / 2;
			allServers[bestServId].cpuB -= eachVmi.cpu / 2;
			allServers[bestServId].memA -= eachVmi.mem / 2;
			allServers[bestServId].memB -= eachVmi.mem / 2;
			allServers[bestServId].vmCount++;

			serv.cpuA += eachVmi.cpu / 2;
			serv.cpuB += eachVmi.cpu / 2;
			serv.memA += eachVmi.mem / 2;
			serv.memB += eachVmi.mem / 2;
			serv.vmCount--;

			dayMigrationOutput.push_back("(" + to_string(eachVmi.id) + ", " + to_string(IdMap[bestServId]) + ")");//输出

			//更新vmOnServ表
			vector<VmIns>& tmpVmList1 = vmOnServ[eachVmi.serverId];//源
			vector<VmIns>& tmpVmList2 = vmOnServ[bestServId];
			tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), eachVmi), tmpVmList1.end());
			//更新虚拟机信息
			eachVmi.serverId = bestServId;//此处debug需要留意
			eachVmi.part = 0;//加不加都行，双节点
			tmpVmList2.push_back(eachVmi);
			//更新allVms
			allVms[eachVmi.id].serverId = eachVmi.serverId;//上面已经进行了更新
			//allVms[eachVmi.id].part = eachVmi.part;//上面已经进行了更新

			actualMigTimes++;
			if (actualMigTimes > maxMigTimes - 1) {
				return;
			}
		}
		else if (part == 1) {
			//目的服务器
			allServers[bestServId].cpuA -= eachVmi.cpu;
			allServers[bestServId].memA -= eachVmi.mem;
			allServers[bestServId].vmCount++;

			if (eachVmi.part == 1) {
				serv.cpuA += eachVmi.cpu;
				serv.memA += eachVmi.mem;
				serv.vmCount--;
			}
			else if (eachVmi.part == 2) {
				serv.cpuB += eachVmi.cpu;
				serv.memB += eachVmi.mem;
				serv.vmCount--;
			}
			else {}//出错

			dayMigrationOutput.push_back("(" + to_string(eachVmi.id) + ", " + to_string(IdMap[bestServId]) + ", A)");//输出

			//更新vmOnServ表
			vector<VmIns>& tmpVmList1 = vmOnServ[eachVmi.serverId];//源
			vector<VmIns>& tmpVmList2 = vmOnServ[bestServId];
			tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), eachVmi), tmpVmList1.end());
			//更新虚拟机信息
			eachVmi.serverId = bestServId;//此处debug需要留意
			eachVmi.part = 1;//加不加都行，双节点
			tmpVmList2.push_back(eachVmi);
			//更新allVms
			allVms[eachVmi.id].serverId = eachVmi.serverId;//上面已经进行了更新
			allVms[eachVmi.id].part = eachVmi.part;//上面已经进行了更新

			actualMigTimes++;
			if (actualMigTimes > maxMigTimes - 1) {
				return;
			}
		}
		else if (part == 2) {
			allServers[bestServId].cpuB -= eachVmi.cpu;
			allServers[bestServId].memB -= eachVmi.mem;
			allServers[bestServId].vmCount++;

			if (eachVmi.part == 1) {
				serv.cpuA += eachVmi.cpu;
				serv.memA += eachVmi.mem;
				serv.vmCount--;
			}
			else if (eachVmi.part == 2) {
				serv.cpuB += eachVmi.cpu;
				serv.memB += eachVmi.mem;
				serv.vmCount--;
			}
			else {}//出错

			dayMigrationOutput.push_back("(" + to_string(eachVmi.id) + ", " + to_string(IdMap[bestServId]) + ", B)");//输出

			//更新vmOnServ表
			vector<VmIns>& tmpVmList1 = vmOnServ[eachVmi.serverId];//源
			vector<VmIns>& tmpVmList2 = vmOnServ[bestServId];
			tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), eachVmi), tmpVmList1.end());
			//更新虚拟机信息
			eachVmi.serverId = bestServId;//此处debug需要留意
			eachVmi.part = 2;//加不加都行，双节点
			tmpVmList2.push_back(eachVmi);
			//更新allVms
			allVms[eachVmi.id].serverId = eachVmi.serverId;//上面已经进行了更新
			allVms[eachVmi.id].part = eachVmi.part;//上面已经进行了更新

			actualMigTimes++;
			if (actualMigTimes > maxMigTimes - 1) {
				return;
			}
		}
	}
#endif // MIG_SWITCH & 0x0100
	//第四轮迁移

#if MIG_SWITCH & 0x1000
	//遍历每台源服务器
	for (auto& srcServ : allServers) {
		double srcServCpuRemainrate = (srcServ.cpuA + srcServ.cpuB + 0.0) / srcServ.allcpu;//资源剩余率
		double srcServMemRemainrate = (srcServ.memA + srcServ.memB + 0.0) / srcServ.allmem;

		if (srcServCpuRemainrate < 0.15 && srcServMemRemainrate < 0.15) continue;//资源满的先不迁移
		if (srcServ.vmCount == 0) continue;//没有虚拟机的服务器跳过

		//遍历每台服务器上的虚拟机列表
		vector<VmIns> vmList = vmOnServ[srcServ.id];//找到源服务器上的虚拟机列表
		for (auto vmi : vmList) {
			if (actualMigTimes >= maxMigTimes - 1) return;
			double srcServEqualRemainSource = (srcServ.cpuA + srcServ.cpuB) * cpuPercent + srcServ.memA + srcServ.memB;//源服务器等效的剩余资源
			//遍历每台服务服务器
			double minDestServEqualRemainSource = Inf;//最小的资源剩余
			int bestDestServId = -1;//最好的服务器ID
			int bestPart = -1;//对应的节点编号
			bool isBoth_A_B_OK;//判断A节点和B节点是否都能放得下，目的是在都能放得下的前提前选择这一个最好的
			for (auto& destServ : allServers) {
				if (srcServ.id == destServ.id) continue;//同一台服务器就跳过
				if (destServ.vmCount == 0) continue;//没有虚拟机的服务器跳过

				double destServCpuRemainrate = (destServ.cpuA + destServ.cpuB + 0.0) / destServ.allcpu;//资源剩余率
				double destServMemRemainrate = (destServ.memA + destServ.memB + 0.0) / destServ.allmem;
				if (destServCpuRemainrate < 0.15 && destServMemRemainrate < 0.15) continue;//资源满的先不迁移

				int destServRemainCpuAAfter = -1;
				int destServRemainCpuBAfter = -1;
				int destServRemainMemAAfter = -1;
				int destServRemainMemBAfter = -1;

				isBoth_A_B_OK = true;//调试的时候需要考虑这个的问题，脑袋晕了，先不考虑这个了

				if (vmi.part == 0) {
					destServRemainCpuAAfter = destServ.cpuA - vmi.cpu / 2;
					destServRemainCpuBAfter = destServ.cpuB - vmi.cpu / 2;
					destServRemainMemAAfter = destServ.memA - vmi.mem / 2;
					destServRemainMemBAfter = destServ.memB - vmi.mem / 2;
					if (destServRemainCpuAAfter < 0 || destServRemainCpuBAfter < 0 || destServRemainMemAAfter < 0 || destServRemainMemBAfter < 0) {
						continue;//这台服务器放不下，换下一台试试
					}
					double destServEqualRemainSource = (destServRemainCpuAAfter + destServRemainCpuBAfter) * cpuPercent + destServRemainMemAAfter + destServRemainMemBAfter;

					if (destServEqualRemainSource < minDestServEqualRemainSource) {
						minDestServEqualRemainSource = destServEqualRemainSource;
						bestDestServId = destServ.id;
						bestPart = 0;
					}
				}
				else if (vmi.part == 1 || vmi.part == 2) {
					if (destServ.cpuA - vmi.cpu >= 0 && destServ.memA - vmi.mem >= 0) {
						destServRemainCpuAAfter = destServ.cpuA - vmi.cpu;
						destServRemainCpuBAfter = destServ.cpuB;
						destServRemainMemAAfter = destServ.memA - vmi.mem;
						destServRemainMemBAfter = destServ.memB;

						double destServEqualRemainSource = (destServRemainCpuAAfter + destServRemainCpuBAfter) * cpuPercent + destServRemainMemAAfter + destServRemainMemBAfter;

						if (destServEqualRemainSource < minDestServEqualRemainSource) {
							minDestServEqualRemainSource = destServEqualRemainSource;
							bestDestServId = destServ.id;
							bestPart = 1;
						}
					}
					else {
						isBoth_A_B_OK = false;
					}

					if (destServ.cpuB - vmi.cpu >= 0 && destServ.memB - vmi.mem >= 0) {
						destServRemainCpuAAfter = destServ.cpuA;
						destServRemainCpuBAfter = destServ.cpuB - vmi.cpu;
						destServRemainMemAAfter = destServ.memA;
						destServRemainMemBAfter = destServ.memB - vmi.mem;

						double destServEqualRemainSource = (destServRemainCpuAAfter + destServRemainCpuBAfter) * cpuPercent + destServRemainMemAAfter + destServRemainMemBAfter;

						if (destServEqualRemainSource < minDestServEqualRemainSource) {
							minDestServEqualRemainSource = destServEqualRemainSource;
							bestDestServId = destServ.id;
							bestPart = 2;
						}
					}
					else {
						isBoth_A_B_OK = false;
					}
				}
				else {}//正常不会到这来
			}
			if (bestDestServId == -1) continue;//没有找到合适的，下一台
			if (minDestServEqualRemainSource < srcServEqualRemainSource) continue;//如果目的服务器迁移后剩余的资源比源服务器还多，那就不迁移了


			Server& destServ = allServers[bestDestServId];//找到对应的目的服务器
			//双节点的，最好处理
			if (bestPart == 0) {
				//更新目的服务器
				destServ.cpuA -= vmi.cpu / 2;
				destServ.cpuB -= vmi.cpu / 2;
				destServ.memA -= vmi.mem / 2;
				destServ.memB -= vmi.mem / 2;
				destServ.vmCount++;

				srcServ.cpuA += vmi.cpu / 2;
				srcServ.cpuB += vmi.cpu / 2;
				srcServ.memA += vmi.mem / 2;
				srcServ.memB += vmi.mem / 2;
				srcServ.vmCount--;

				dayMigrationOutput.push_back("(" + to_string(vmi.id) + ", " + to_string(IdMap[bestDestServId]) + ")");//输出

				//更新vmOnServ表
				vector<VmIns>& tmpVmList1 = vmOnServ[vmi.serverId];//源
				vector<VmIns>& tmpVmList2 = vmOnServ[bestDestServId];//目的
				tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), vmi), tmpVmList1.end());
				//更新虚拟机信息
				vmi.serverId = bestDestServId;//此处debug需要留意
				vmi.part = 0;//加不加都行，双节点
				tmpVmList2.push_back(vmi);
				//更新allVms
				allVms[vmi.id].serverId = vmi.serverId;//上面已经进行了更新
				allVms[vmi.id].part = vmi.part;//上面已经进行了更新

				actualMigTimes++;
				if (actualMigTimes > maxMigTimes - 1) {
					return;
				}
			}
			else if (bestPart == 1) {
				destServ.cpuA -= vmi.cpu;
				destServ.memA -= vmi.mem;
				destServ.vmCount++;

				if (vmi.part == 1) {
					srcServ.cpuA += vmi.cpu;
					srcServ.memA += vmi.mem;
					srcServ.vmCount--;
				}
				else if (vmi.part == 2) {
					srcServ.cpuB += vmi.cpu;
					srcServ.memB += vmi.mem;
					srcServ.vmCount--;
				}
				else {}//出错

				dayMigrationOutput.push_back("(" + to_string(vmi.id) + ", " + to_string(IdMap[bestDestServId]) + ", A)");//输出

				//更新vmOnServ表
				vector<VmIns>& tmpVmList1 = vmOnServ[vmi.serverId];//源
				vector<VmIns>& tmpVmList2 = vmOnServ[bestDestServId];
				tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), vmi), tmpVmList1.end());
				//更新虚拟机信息
				vmi.serverId = bestDestServId;//此处debug需要留意
				vmi.part = 1;
				tmpVmList2.push_back(vmi);
				//更新allVms
				allVms[vmi.id].serverId = vmi.serverId;//上面已经进行了更新
				allVms[vmi.id].part = vmi.part;//上面已经进行了更新

				actualMigTimes++;
				if (actualMigTimes > maxMigTimes - 1) {
					return;
				}
			}
			else if (bestPart == 2) {
				destServ.cpuB -= vmi.cpu;
				destServ.memB -= vmi.mem;
				destServ.vmCount++;

				if (vmi.part == 1) {
					srcServ.cpuA += vmi.cpu;
					srcServ.memA += vmi.mem;
					srcServ.vmCount--;
				}
				else if (vmi.part == 2) {
					srcServ.cpuB += vmi.cpu;
					srcServ.memB += vmi.mem;
					srcServ.vmCount--;
				}
				else {}//出错

				dayMigrationOutput.push_back("(" + to_string(vmi.id) + ", " + to_string(IdMap[bestDestServId]) + ", B)");//输出

				//更新vmOnServ表
				vector<VmIns>& tmpVmList1 = vmOnServ[vmi.serverId];//源
				vector<VmIns>& tmpVmList2 = vmOnServ[bestDestServId];
				tmpVmList1.erase(remove(tmpVmList1.begin(), tmpVmList1.end(), vmi), tmpVmList1.end());
				//更新虚拟机信息
				vmi.serverId = bestDestServId;//此处debug需要留意
				vmi.part = 2;//
				tmpVmList2.push_back(vmi);
				//更新allVms
				allVms[vmi.id].serverId = vmi.serverId;//上面已经进行了更新
				allVms[vmi.id].part = vmi.part;//上面已经进行了更新

				actualMigTimes++;
				if (actualMigTimes > maxMigTimes - 1) {
					return;
				}
			}
		}
	}
#endif
}

//bool sortServByRate(const Server& serv1, const Server& serv2) {
//    return serv1.rate > serv1.rate;
//}

void updateServInfo() {
	//hardPercent = 1.0 + 9.0 * day / (allDays + 0.0);//越往后电费占比越小
	int allRemainCpu = 0;
	int allRemainMem = 0;
	for (auto serv : allServers) {
		allRemainCpu += serv.cpuA + serv.cpuB;
		allRemainMem += serv.memA + serv.memB;
	}

	if (allRemainMem != 0)
		buyCpuPercent = buyCpuPercentBase + allRemainMem / (allRemainCpu * buyCpuPercentDen);

	for (auto& serv : rateRatioServers) {
		//serv.second.cpuRate = (serv.second.hardCost / (serv.second.cpuA + 0.0) * hardPercent + serv.second.dayCost / (serv.second.cpuA + 0.0) * (10 - hardPercent)) / 2;
		//serv.second.memRate = (serv.second.hardCost / (serv.second.memA + 0.0) * hardPercent + serv.second.dayCost / (serv.second.memA + 0.0) * (10 - hardPercent)) / 2;
		//serv.cpuRate = (serv.hardCost + serv.dayCost * (allDays - day)) / (serv.cpuA + 0.0);
		//serv.memRate = (serv.hardCost + serv.dayCost * (allDays - day)) / (serv.memA + 0.0);
		//serv.rate = (serv.cpuRate * buyCpuPercent + serv.memRate * (10 - buyCpuPercent)) / 2;
		serv.rate = (serv.hardCost + serv.dayCost * (allDays - day)) / (serv.allcpu * buyCpuPercent + serv.allmem + 0.0);
	}


	//rateServers.clear();
	sortServerInfo();
	//std::sort(rateserv.seconders.begin(), rateserv.seconders.end(), sortserv.secondByRate);
	return;
}


int countShutDownServer() {
	int count = 0;
	for (auto serv : allServers) {
		if (serv.vmCount == 0) count++;
	}
	return count;
}

//排序的辅助函数，按照每个请求中的虚拟机的资源大小进行排序
bool sortAddCmdByVmSize(const Command& cmd1, const Command& cmd2) {
	Vm vm1 = vmInfos[cmd1.vmType];
	Vm vm2 = vmInfos[cmd2.vmType];
	double vm1EqualRes = vm1.cpu * cpuPercent + vm1.mem;//vm1的等效资源
	double vm2EqualRes = vm2.cpu * cpuPercent + vm2.mem;
	return vm1EqualRes < vm2EqualRes;
}

void CalculateResourceRemainRate() {
	//ofstream out;
	//out.open("ResourceRemainingRate.txt", ios::app);
	//for (auto& serv : allServers) {
	//    if (serv.vmCount == 0) continue;
	//    out << (double)(serv.cpuA + serv.cpuB) / serv.allcpu << " " << (double)(serv.memA + serv.memB) / serv.allmem << endl;
	//}
	//out << endl;
	//out.close();
}


//按核内比排序
void sortRateRatioServers() {
	for (auto& mServ : serverInfos) {
		rateRatioServers.push_back(mServ.second);
	}
	std::sort(rateRatioServers.begin(), rateRatioServers.end(), sortServerInfoByCpuMemRatio);
}

//计算当天购买的服务的核内比
double calculateCpuMemRatioForBuy(const vector<Command>& cmds) {
	int dayNeedCpu = 0;
	int dayNeedMem = 0;
	for (auto& cmd : cmds) {
		if (cmd.type == 1) {
			if (cmd.success == false) continue;
			Vm vm = vmInfos[cmd.vmType];
			dayNeedCpu += vm.cpu;
			dayNeedMem += vm.mem;
		}
		else {
			VmIns vmi = allVms[cmd.id];
			dayNeedCpu -= vmi.cpu;
			dayNeedMem -= vmi.mem;
		}
	}

	if (dayNeedCpu > 0 && dayNeedMem > 0)
		return dayNeedCpu / (dayNeedMem + 0.0);
	else if (dayNeedCpu < 0 && dayNeedMem < 0)
		return dayNeedMem / (dayNeedCpu + 0.0);
	else if (dayNeedCpu <= 0 && dayNeedMem > 0)
		return 0;
	else if (dayNeedCpu > 0 && dayNeedMem <= 0)
		return Inf;
	else
		return 1;

	//return abs(dayNeedCpu / (dayNeedMem + 0.0));//不太合理
}

//获取最低报价
int geLowestQuotation(const Command& cmd) {
	//servId:传出参数，传出尝试添加的服务器ID，当mode=1传NULL
	//mode: 0-尝试添加（用于报价，未真实添加）， 1-真实添加
	Server serv;
	addVm(cmd, &serv, 0);//尝试添加在目的服务器上，以获取最低的报价
	Vm vm = vmInfos[cmd.vmType];
	//double cpuRate = vm.cpu / (serv.allcpu * 1.0);
	//double memRate = vm.mem / (serv.allmem * 1.0);

	double allServCpuRemainRate, allServMemRemainRate;//资源剩余率（浪费率）
	int allCpu = 0, allMem = 0;//全部的cpu和内存
	int remainCpu = 0, remainMem = 0;
	for (auto& serv1 : allServers) {
		if (serv1.vmCount == 0) continue;
		allCpu += serv1.allcpu;
		allMem += serv1.allmem;
		remainCpu += (serv1.cpuA + serv1.cpuB);
		remainMem += (serv1.memA + serv1.memB);
	}

	allServCpuRemainRate = allCpu == 0 ? 0 : remainCpu / (allCpu * 1.0);
	allServMemRemainRate = allMem == 0 ? 0 : remainMem / (allMem * 1.0);

	int dailyCost = serv.hardCost / (allDays - serv.buildDay) + serv.dayCost;//平均这台服务器每天的成本

	double eachResPrice = dailyCost / (serv.allcpu * (1 - allServCpuRemainRate) * cpuPercent + serv.allmem * (1 - allServMemRemainRate));//对服务器的资源价格进行每日量化

	return eachResPrice * (vm.cpu * cpuPercent + vm.mem) * cmd.aliveDays;
}
int getx(const Command& cmd) {

	Vm vm = vmInfos[cmd.vmType];
	//double cpuRate = vm.cpu / (serv.allcpu * 1.0);
	//double memRate = vm.mem / (serv.allmem * 1.0);

	return  (vm.cpu * cpuPercent + vm.mem) * cmd.aliveDays;
}


void readGetResult_debug(vector<Command>& commands) {
	const string filename = "../training-data/reback_2.txt";
	static ifstream in;
	if (day == 0) in.open(filename, ios::in);
	string success, pass;
	int oppoQuotation = -1;//暂时先不处理对手的报价

	int addNum;
	in >> addNum;
	for (auto& cmd : commands) {
		if (cmd.type == 0) continue;//删除的节点跳过
		in >> success >> oppoQuotation >> pass;
		if (success[1] == '0') {
			cmd.success = false;
				//插值拟合横坐标取值
				if (countLoc < PriceCount) {
                    differenceTable[countLoc][0] = oppoQuotation;//插商表赋初值
					xLowestCost[countLoc] = getx(cmd);
					countLoc++;
				}
			
		}
		else if (success[1] == '1') {
			cmd.success = true;//1表示成功
			if (countLoc < PriceCount) {
				differenceTable[countLoc][0] = oppoQuotation;//插商表赋初值
				xLowestCost[countLoc] = getx(cmd);
				countLoc++;
			}
		}
		else {
			if (debug == 1) fprintf(stderr, "readGetResult error!\n");
		}
	}
	if (day == allDays - 1) in.close();
}

void readGetResult_upload(vector<Command>& commands) {

	string success, pass;
	int oppoQuotation = -1;//暂时先不处理对手的报价
	for (auto& cmd : commands) {
		if (cmd.type == 0) continue;//删除的节点跳过
		cin >> success >> oppoQuotation >> pass;
		if (success[1] == '0') {
			cmd.success = false;
			if (countLoc < PriceCount) {
				differenceTable[countLoc][0] = oppoQuotation;//插商表赋初值
				xLowestCost[countLoc] = getx(cmd);
				countLoc++;
			}
		}
		else if (success[1] == '1') {
			cmd.success = true;//1表示成功
			if (countLoc < PriceCount) {
				differenceTable[countLoc][0] = oppoQuotation;//插商表赋初值
				xLowestCost[countLoc] = getx(cmd);
				countLoc++;
			}
		}
		else {
			if (debug == 1) fprintf(stderr, "readGetResult error!\n");
		}
	}
}
//计算差商表
void calcTable()
{
	int dValue = 1;
	//此处 i 表示列
	for (int i = 0; i < PriceCount; i++)
	{
		//此处 j 表示行
		for (int j = i; j < PriceCount; j++)
		{
			double dQuotient = (differenceTable[j + 1][i] - differenceTable[j][i]) / (xLowestCost[j + 1] - xLowestCost[j + 1 - dValue]);
			differenceTable[j + 1][i + 1] = dQuotient;
		}
		dValue += 1;
	}
}
//牛顿插值
double NewtonValue(double x)
{
	double calcValue = 0, temp = 1;
	for (int i = 0; i < PriceCount; i++)
	{
		temp = 1;
		//计算(X-X0)*(X-X1).....*(X-Xn-1)
		for (int j = 0; j < i; j++)
		{
			temp *= (x - xLowestCost[j]);
		}
		//计算f(X0,X1,....,Xn)*(X-X0)*(X-X1).....*(X-Xn-1)
		temp *= differenceTable[i][i];
		//计算f(X0,X1,....,Xn)*(X-X0)*(X-X1).....*(X-Xn-1)的和
		calcValue += temp;
	}
	return calcValue;
}
bool cmp(pair<int, int> a, pair<int, int> b) {
	return a.first < b.first;
}
int matchPriceRes(const Command& cmd) {//插值拟合，计算出对手大概的出价
	int myx = getx(cmd);
	//存储数据
	//pair<int, int> singleMatchPriceData;
	//vector<pair<int, int>> matchPriceData;//需要全局变量
	//int matchPriceDataCount = 0;
	//sort(matchPriceData.begin(), matchPriceData.end(), cmp);
	calcTable();//计算差分表
	return NewtonValue(myx);

}

int comperitiveAnalysis(const Command& cmd) {

	return matchPriceRes(cmd);
}

int eachVmReqQuotation(const Command& cmd) {
	//if (debug == 1) fprintf(stderr, "%d\n", cmd.userQuotation / 2);
	int finalPriceBase = geLowestQuotation(cmd);
	int myAnalysisPrice = comperitiveAnalysis(cmd);
	if (countLoc >= PriceCount && myAnalysisPrice >= finalPriceBase) {
			return myAnalysisPrice * 0.92;
	}
	if (countLoc < PriceCount )
	return cmd.userQuotation;//以最低价的%110出售
	else
	return finalPriceBase * 1.15;
}

void quotation(const vector<Command>& commands) {
	for (auto& cmd : commands) {
		if (cmd.type == 0) continue;//删除的请求就跳过
		int quo = eachVmReqQuotation(cmd);

			cout << quo << endl;
		//cout << eachVmReqQuotation(cmd) << endl;
	}
}

// 处理一天的请求
void process(vector<Command>& commands) {

	//先更新服务器信息
	dayRequestCpuMemRatio = calculateCpuMemRatioForBuy(commands);//计算今天购买的核内比
	updateServInfo();

	//向用户报价
	quotation(commands);
	if (day == 30) {
		int i = 0;
	}

	//读取获取是否成功的信息
	if (debug == 1)
		readGetResult_debug(commands);
	else
		readGetResult_upload(commands);

	migration();

	for (int i = 0; i < commands.size(); i++) {
		Command cmd = commands[i];
		if (cmd.type == 1) {
			if (cmd.success == false) continue;//如果获取失败就跳过
			if (addVm(cmd, NULL, 1) == 1) {
				dayRequestCpuMemRatio = calculateCpuMemRatioForBuy(vector<Command>(commands.begin() + i, commands.end()));//计算一下这些指令所需的核内比
				sortServerInfo();
				VmIns vmi = { commands[i].vmType, commands[i].vmId };
				vmi.cpu = vmInfos[vmi.name].cpu;
				vmi.mem = vmInfos[vmi.name].mem;
				vmi.part = vmInfos[vmi.name].doubleNode == 1 ? 0 : 1;
				addServer(vmi, NULL, 1);//买服务器
				addVmToServer(vmi, allServers[allServers.size() - 1], vmi.part, commands[i].id);
			}
		}
		else {
			deleteVm(cmd);
		}
	}
}

void calculateDayCost() {
	for (auto& serv : allServers) {
		if (serv.vmCount > 0) {
			POWERCOST += serv.dayCost;//这种计算方式是不对的
		}
	}
}

bool sortDayOutInfoByCmdId(const dayOutputInfo& opi1, const dayOutputInfo& opi2) {
	return opi1.id < opi2.id;
}

void serverIdRemap()
{
	int servcount = 0;
	int i = 0;
	for (auto serv = dayServers.begin(); serv != dayServers.end(); serv++)
		servcount += serv->second.size();
	for (auto serv = dayServers.begin(); serv != dayServers.end(); serv++)
	{
		for (auto it = serv->second.begin(); it != serv->second.end(); it++)
		{
			//IdMap[allServers.size() - servcount + i] = *it;//可能是映射反了
			IdMap[*it] = allServers.size() - servcount + i;
			i++;
		}
	}

	//顺带处理一下打印信息

	std::sort(dayOutInfo.begin(), dayOutInfo.end(), sortDayOutInfoByCmdId);

	for (auto it = dayOutInfo.begin(); it != dayOutInfo.end(); it++)
	{
		if (it->nodeType == 0)
			dayOutput.push_back("(" + to_string(IdMap[it->servId]) + ")");
		else {
			string node = it->nodeType == 1 ? "A" : "B";
			dayOutput.push_back("(" + to_string(IdMap[it->servId]) + ", " + node + ")");
		}
	}
}

void printAnalyzeData() {
	//ofstream out;
	//out.open("./RemainingResources.txt", ios::out | ios::trunc);
	//for (auto serv : allServers) {
	//    out << serv.cpuA << " " << serv.cpuB << " " << serv.memA << " " << serv.memB << endl;
	//}
	//cout << endl;
	//out.close();
}

void dayProcess() {

	currentTime = clock();
	//if (currentTime - startTime >= 75 * 1000) enableMigration = false;//运行时间已经超过75s了就应该关闭第三轮迁移，防止超时

	int reqNum, vmId, aliveDays, userQuotation;
	string op, vmType, pass;
	dayServers.clear();
	dayOutput.clear();
	dayOutInfo.clear();
	dayMigrationOutput.clear();
	daySleepServer.clear();

	//sortServerInfo();

	vector<Command> commands = allCommands[day];
	process(commands);//处理请求包括迁移
	calculateDayCost();//计算每日消耗
	serverIdRemap();//服务器ID映射


	vector<string> dayout;
	//购买的输出
	dayout.push_back("(purchase, " + to_string(dayServers.size()) + ")");
	//if (debug == 1)
	//    fprintf(stderr, "%d\n", dayServers.size());
	for (auto s : dayServers) {
		dayout.push_back("(" + s.first + ", " + to_string(s.second.size()) + ")");
	}

	dayout.push_back("(migration, " + to_string(dayMigrationOutput.size()) + ")");
	for (auto& s : dayMigrationOutput) {
		if (s[0] == '0') {//首位为零表示未映射服务器ID的迁移输出，需要在此处映射ID
			stringstream ss;
			int vmId, servId;
			string str;
			ss << s;
			ss >> str;
			ss >> str;
			vmId = atoi(str.substr(1, str.size() - 2).c_str());
			ss >> str;
			servId = atoi(str.substr(0, str.size() - 1).c_str());
			ss >> str;
			s = "(" + to_string(vmId) + ", " + to_string(IdMap[servId]) + ", " + str;//映射服务器ID
			int a = 1;
		}
		else if (s[0] == '1') {//首位为零表示未映射服务器ID的迁移输出（双节点），需要在此处映射ID
			stringstream ss;
			int vmId, servId;
			string str;
			ss << s;
			ss >> str;
			ss >> str;
			vmId = atoi(str.substr(1, str.size() - 2).c_str());
			ss >> str;
			servId = atoi(str.substr(0, str.size() - 1).c_str());
			s = "(" + to_string(vmId) + ", " + to_string(IdMap[servId]) + ")";//映射服务器ID
		}

		dayout.push_back(s);
	}

	//处理请求的输出
	for (auto& s : dayOutput) dayout.push_back(s);

	for (auto& s : dayout) cout << s << endl;
	fflush(stdout);
}

//读数据集信息
void readInfo() {
	if (upload == 0) {
		const string filePath = "../training-data/training-2.txt";
		std::freopen(filePath.c_str(), "rb", stdin);
	}

	//////////输出
	if (debug == 1) {
		const string filePath1 = "./output1.txt";
		std::freopen(filePath1.c_str(), "wb", stdout);
	}

	//timeStart = clock();
	int inputNum;
	cin >> inputNum;

	//练习数据集作弊
	//if (inputNum == 80) buyCpuPercent = 6.1;
	//else buyCpuPercent = 7.3;

	// 读入服务器类型信息
	string serverName, pass;
	int cpuNum, mem, hardCost, dayCost;
	for (int i = 0; i < inputNum; i++) {
		cin >> serverName >> cpuNum >> pass >> mem >> pass >> hardCost >> pass >> dayCost >> pass;
		addServerInfo(serverName, cpuNum, mem, hardCost, dayCost);
	}
	//sortServerInfo();
	sortRateRatioServers();
	// 读入虚拟机类型信息
	cin >> inputNum;
	int doubleNode;
	for (int i = 0; i < inputNum; i++) {
		cin >> serverName >> cpuNum >> pass >> mem >> pass >> doubleNode >> pass;
		addVmInfo(serverName, cpuNum, mem, doubleNode);
	}
}


void printInfo(int day) {
	if (!debug) {
		return;
	}

	//timeEnd = clock();
	//cout << "time:" << double(timeEnd - timeStart) / CLOCKS_PER_SEC << endl;


	int used = 0;
	for (auto& serv : allServers) if (serv.vmCount > 0)used++;
	fprintf(stderr, "day %d serverNumber %lu used %d server: %lld  power: %lld total: %lld \n", day, allServers.size(), used, SERVERCOST, POWERCOST, TOTALCOST);
	// int i = 0;
	// for(auto &cou:serverRunVms) if(cou>0)cout<<"["<<i++<<" "<<cou<<"]";
	// cout<<endl;

	// int i = 0;
	// for(auto &serv:allServers) if(serv.vmCount>0)cout<<"["<<i++<<" "<<serv.vmCount<<"]";
	// cout<<endl;
}
void printInfo2(int day) {
	if (!debug) {
		return;
	}

	//timeEnd = clock();
	//cout << "time:" << (double)((double)timeEnd - (double)timeStart) / CLOCKS_PER_SEC << endl;

	int used = 0;
	for (auto& serv : allServers) if (serv.vmCount > 0)used++;
	printf("day %d serverNumber %lu used %d server: %lld  power: %lld total: %lld \n", day, allServers.size(), used, SERVERCOST, POWERCOST, TOTALCOST);
	// int i = 0;
	// for(auto &cou:serverRunVms) if(cou>0)cout<<"["<<i++<<" "<<cou<<"]";
	// cout<<endl;

	int i = 0;
	for (auto& serv : allServers) if (serv.vmCount > 0)cout << "[" << i++ << " " << serv.vmCount << "]";
	cout << endl;
}

void printArgs() {
	if (!debug) {
		return;
	}
	fprintf(stderr, "cpuPercent %f sortRate %d\n", cpuPercent, sortRate);
}

void readDayCommands() {
	int reqNum;
	string op, vmType, pass;
	int vmId, aliveDays, userQuotation;
	cin >> reqNum;
	vector<Command> commands;
	int cmdId = 0;//add请求的id
	for (int j = 0; j < reqNum; j++) {
		cin >> op;
		if (op[1] == 'a') {
			cin >> vmType >> vmId >> pass >> aliveDays >> pass >> userQuotation >> pass;
			//Command cmd = { 1, vmType.substr(0, vmType.size() - 1), vmId, cmdId, aliveDays , userQuotation };
			Command cmd;
			cmd.type = 1;
			cmd.vmType = vmType.substr(0, vmType.size() - 1);
			cmd.vmId = vmId;
			cmd.id = j;
			cmd.aliveDays = aliveDays;
			cmd.userQuotation = userQuotation;
			commands.push_back(cmd);
			cmdId++;
		}
		else {
			cin >> vmId >> pass;
			Command cmd;
			cmd.type = 0;
			cmd.vmId = vmId;
			commands.push_back(cmd);
		}
	}
	allCommands.push_back(commands);
}

int main(int argc, char* argv[]) {

	startTime = clock();

	//remove("ResourceRemainingRate.txt");

	//读数据集信息
	readInfo();

	int dayNum, KnowDayNum;
	cin >> dayNum >> KnowDayNum;
	allDays = dayNum;
	//先读取已知天数的请求
	for (day = 0; day < KnowDayNum; day++) {
		readDayCommands();
	}

	//处理未知的天数
	int outputPos = 0;//输出列表的索引
	for (day = 0; day < dayNum; day++) {

		//当天的决策
		dayProcess();
		//读取下一天的信息
		if (allCommands.size() < allDays) readDayCommands();

		//if (day == 600 && debug == 1)
		//    printAnalyzeData();

	}


	TOTALCOST = SERVERCOST + POWERCOST;

	printArgs();
	printInfo(-1);

	return 0;
}
