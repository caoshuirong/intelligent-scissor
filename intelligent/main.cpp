
#include "stdafx.h"
#include<opencv2/opencv.hpp>
#include<iostream>
#include<algorithm>
#include<cmath>
#include<vector>
#include<stack>
#include"Scissor.h"
//#include<Windows.h>
using namespace std;
using namespace cv;
//光标矫正的范围
extern int CURSERSNAPTRS ;
Scissor myScissor;
//用于鼠标操作的绘图
Mat paintMat, originImg,cutMat;
//记录最短路径的坐标，方便后面抠图
vector<pair<int, int>> path;
//记录路径最开始的种子
pair<int, int> firstSeed,tmpPath;

//stack<positionAndBGR> backUp;
//判断曲线是否闭合,用于后面的截图
bool isClosed;
//记录图像的坐标和像素值，用于还原
struct positionAndBGR{

	int row;
	int col;
	Vec3f val;
}tmpPosBGR;
//存储还原图像
vector<positionAndBGR> restore;

//
long long newSeed = 0;
//漫水填充
int floodLowValue = 20;
int floodUpValue = 20;
int floodCount = 100;
Point seed;
int b = 185;
int g = 185;
int r = 185;
Rect ccomp;//定义重绘区域的最小边界矩形区域
Mat floodMat ;
Scalar newVal = Scalar(b, g, r);

//路径冷却
#define timeTRS 50.0
#define redrawTRS 100
//鼠标回调函数
void onMouse(int Event, int x_col, int y_row, int flags, void * userData);

bool cmp(const pair<int, int> & a, const pair<int, int> & b)
{
	if (a.first > b.first)
	{
		return 1;
	}
	else if(a.first == b.first && a.second < b.second){
		return 1;
	}

	return 0;
}

int main()
{
	//读入图片,计算Fz Fg Fd.
    originImg = imread("6.jpg");//1.jpg  png

	if (!originImg.data)
	{
		printf_s("图片读取失败，请确认文件名是否有误！");
		return 0;
	}

	paintMat = originImg.clone();
	floodMat = originImg.clone();
    cutMat = Mat(paintMat.rows, paintMat.cols, paintMat.type(), Scalar(185,185,185));

	myScissor = Scissor(originImg);

	namedWindow("show", WINDOW_AUTOSIZE);


	imshow("show", originImg);

	isClosed = false;
	//cursorSnapBar
	createTrackbar("CursorSnapCoef", "show", &CURSERSNAPTRS, 60);
	
	//FloodFill
	
	createTrackbar("low", "show", &floodLowValue, floodCount);

	createTrackbar("up", "show", &floodUpValue, floodCount);

	setMouseCallback("show", onMouse, &paintMat);

	waitKey(0);

	//显示总的花费图-----------------------------------------------------------------------------------------------------------
	//myScissor.showGray();
	
	//waitKey(0);
	
    return 0;
}
//为什么有些线是断断续续的，什么原因？？像素被压缩了
void onMouse(int Event, int x_col, int y_row, int flags, void * userData)
{
	//因为鼠标的滑轮还包含滑动条，所以行会越界，必须检查！！！
	if (x_col < 0 || x_col >= originImg.cols || y_row < 0 || y_row >= originImg.rows)
	{
		x_col = y_row = 1;
	}
	//cout << '(' << y_row << ',' << x_col << ')' << endl;//调试用

	Mat &pMat = *(Mat * )userData;

	long long mouseIndex = y_row * pMat.cols + x_col;

	mouseIndex = myScissor.cursorSnap(mouseIndex);
	//-----------------------------------------------------------
	newSeed = mouseIndex;
	//因为原图是三通道uchar类型的，所以必须用uchar指针访问和修改，否则会丢精度，图像无法完全还原！！！！！！！！！！！！！！！！！！！！！！
	Vec3b * p;

	vector<positionAndBGR>::iterator it;

	seed.x = x_col;
	
	seed.y = y_row;

	if (!myScissor.isSeed())
	{

		firstSeed.first = myScissor.nodes[mouseIndex].row;

		firstSeed.second = myScissor.nodes[mouseIndex].col;
	}
	
	switch (Event)
	{
		case EVENT_MOUSEMOVE:
		{
			if (!myScissor.isSeed())
			{
				return;
			}
			//每弹出一个就还原对应的像素值

			long long tmpIndex = 0;

			if (!restore.empty())
			{  
			    for (it = restore.begin(); it != restore.end(); it++)
				{
					tmpIndex = it->row * pMat.cols + it->col;

					(myScissor.nodes[tmpIndex]).tm.stop();
					//记录时间
					((myScissor.nodes[tmpIndex]).countTime) += ((myScissor.nodes[tmpIndex]).tm).getTimeSec();//+ *(myScissor.oldFgMat).ptr<float>(it->row,it->col);
					//记录次数
					(myScissor.nodes[tmpIndex]).redraw++;
					//cout << (myScissor.nodes[tmpIndex]).countTime << ' ' << (myScissor.nodes[tmpIndex]).redraw<<endl;


					//还原像素值
					if (((myScissor.nodes[tmpIndex]).countTime) < timeTRS || ((myScissor.nodes[tmpIndex]).redraw) < redrawTRS)
					{
						*pMat.ptr<Vec3b>(it->row, it->col) = it->val;
					}
				}

				restore.clear();
			}
			//存下原有的像素，修改图片的像素值，用于显示轮廓线
			while (myScissor.nodes[mouseIndex].prevNode != NULL)
			{
				//路径冷却
				if (((myScissor.nodes[mouseIndex]).countTime) >= timeTRS && ((myScissor.nodes[mouseIndex]).redraw) >= redrawTRS)
				{
					cout << (myScissor.nodes[mouseIndex]).row << "," << (myScissor.nodes[mouseIndex]).col << endl;
					if (!restore.empty())
					{
						//记录所走的路径上的每一个点
						for (it = restore.begin(); it != restore.end(); it++)
						{
							//用于对象提取
							path.push_back({ it->row,it->col });

						}
						//查找路径是否闭合，只有回到原点认为是闭合
						for (it = restore.begin(); it != restore.end(); it++)
						{
							if (it->row == firstSeed.first && it->col == firstSeed.second)//-------------------------------------------------------
							{
								isClosed = true;
								break;
							}
						}
						//手动冷却…………
						//restore.clear();

					}

					//重新计算所有点到新种子点的最短路径
					myScissor.liveWire(mouseIndex);

					*pMat.ptr<Vec3d>((myScissor.nodes[mouseIndex]).row, (myScissor.nodes[mouseIndex]).col) = Vec3d(255, 255, 255);
					
					//让自动生成的种子点距离更远一点
					for (it = restore.begin(); it != restore.end(); it++)
					{
						tmpIndex = it->row * pMat.cols + it->col;
						//记录时间
						((myScissor.nodes[tmpIndex]).countTime) = 0;
						//记录次数
						(myScissor.nodes[tmpIndex]).redraw = 0;

						(myScissor.nodes[tmpIndex]).tm.reset();
						
					}

					while (myScissor.nodes[mouseIndex].prevNode != NULL)
					{
						myScissor.nodes[mouseIndex].tm.stop();
						myScissor.nodes[mouseIndex].tm.reset();
						mouseIndex = (myScissor.nodes[mouseIndex].prevNode->thisIndex);
					}

					break;
				}

				p = pMat.ptr<Vec3b>(myScissor.nodes[mouseIndex].row, myScissor.nodes[mouseIndex].col);
				
				((myScissor.nodes[mouseIndex]).tm).reset();

				((myScissor.nodes[mouseIndex]).tm).start();

				restore.push_back({ myScissor.nodes[mouseIndex].row ,myScissor.nodes[mouseIndex].col ,*p });
				//设置所走路径颜色
				*p = {0,0,255};
				//移动到下一个节点！！！
				mouseIndex = (myScissor.nodes[mouseIndex].prevNode->thisIndex);
			}


			break;
		}
		case EVENT_LBUTTONDBLCLK:
		{
			/*if (path.empty())
			{
				return;
			}*/
			//双击还原画布
			cutMat = Mat(paintMat.rows, paintMat.cols, paintMat.type(), Scalar(185, 185, 185));
			imshow("cutImg", cutMat);
			break;
		}
		case EVENT_LBUTTONUP:
		{
			
			if (!restore.empty())
			{
				//记录所走的路径上的每一个点
				for (it = restore.begin(); it != restore.end(); it++)
				{
					//用于对象提取
					path.push_back({it->row,it->col});
					*pMat.ptr<Vec3b>(it->row, it->col) = {255,0,0};
					//画圆
					circle(pMat, Point(it->col, it->row), 1, { 255,0,0 });
				}
				//查找路径是否闭合，只有回到原点认为是闭合
				for (it = restore.begin(); it != restore.end(); it++)
				{
					if (it->row == firstSeed.first && it->col == firstSeed.second)//-------------------------------------------------------
					{
						isClosed = true;
						break;
					}
				}
				//手动冷却…………
				restore.clear();
				
			}
			
			//重新计算所有点到新种子点的最短路径
			myScissor.liveWire(mouseIndex);

			break;
		}

		case EVENT_RBUTTONDOWN://描线法---------------------------------------------------------------
		{
			//如果最短路径为空则不绘图
			if (path.empty())
			{
				return;
			}
			//描线法实现抠图，
			//检查曲线是否闭合！//闭合与否对我的程序无影响，不会报错，只不过有时候需要多扣一次
			if (isClosed == false)
			{
				onMouse(EVENT_MOUSEMOVE, firstSeed.second, firstSeed.first, NULL, &paintMat);

				onMouse(EVENT_LBUTTONUP, firstSeed.second, firstSeed.first, NULL, &paintMat);
				//默认回到原点为闭合
				isClosed = true;
				//cout << "闭合路径中!" << endl;
			}
			//path，pair类型的vec

			sort(path.begin(), path.end(), cmp);

			vector<pair<int, int>>::iterator pit, nextIt, qit;

			qit = nextIt = pit = path.begin();

			while (nextIt != path.end() && nextIt->first == pit->first)
			{
				nextIt++;
			}

			int maxrow = pit->first, maxcol = 0, endCol,  i;

			int mincol = 65536, minrow = path.back().first;

			for (; qit != path.end(); qit++)
			{
				if (qit->second > maxcol)
				{
					maxcol = qit->second;
				}
				else if (qit->second < mincol)
				{
					mincol = qit->second;
				}

			}
			endCol = (nextIt - 1)->second;

			//不能用uchar,因为i会被当做，3*元素个数的小标，会偏左很多。
			//对基本的像素访问了解为零
			while (pit != path.end())
			{
				//p = originImg.ptr<Point3f>(pit->first);
				//q = cutMat.ptr<Point3f>(pit->first);
				for (i = pit->second; i <= endCol; i++)
				{
					//提取floodMat上的对象，行列对应的像素值
					*cutMat.ptr<Point3i>(pit->first, i) = *floodMat.ptr<Point3i>(pit->first, i);
				
				}
				pit = nextIt;

				while (nextIt != path.end() && nextIt->first == pit->first)
				{
					nextIt++;
				}

				endCol = (nextIt - 1)->second;

			}

			cv::imshow("cutImg", cutMat);
			//允许多次抠图
			path.clear();

			break;
		}

		case EVENT_MOUSEWHEEL:
		{
			//还原为原图
			int i, j;
			//真的不理解……，为什么会越界覆盖，不是553刚刚好吗？？？不改，就报HEAP CORRUPTION  只能修改到第552行？？？
			for( i = 0;i < originImg.rows-1;i++)
			{
				for ( j = 0; j < originImg.cols; j++)
				{
					//还原为原图
					*pMat.ptr<Point3d>(i, j) = *originImg.ptr<Point3d>(i, j);

					*floodMat.ptr<Point3d>(i, j) = *originImg.ptr<Point3d>(i, j);
				}
			}
			//cout << i << endl;
			//种子点标志还原
			myScissor.setSeed(false);
			isClosed = false;
			//已经记录的最短路径清空，如果没有右键，则不会画在图上，否则，画在图上的不会被清空
			if(!path.empty())
				path.clear();
			//必须清空restore，因为不清空，path,会存下一些移动后的点，导致绘图会粘连
			if(!restore.empty())
				restore.clear();
			
			break;
		}
		case EVENT_RBUTTONDBLCLK:
		{
			
			//只是为了显示在show窗口
			floodFill(pMat, seed, newVal, &ccomp, Scalar(floodLowValue, floodLowValue, floodLowValue),
				Scalar(floodUpValue, floodUpValue, floodUpValue), 4|FLOODFILL_FIXED_RANGE);

			//真正改变的是floodMat，如果直接把pMat上的提取，会把包络线也提取上去。
			floodFill(floodMat, seed, newVal, &ccomp, Scalar(floodLowValue, floodLowValue, floodLowValue),
				Scalar(floodUpValue, floodUpValue, floodUpValue), 4 | FLOODFILL_FIXED_RANGE);

			
			break;
		}

		default:
			break;
	}
	//更新窗口
	imshow("show", pMat);
}