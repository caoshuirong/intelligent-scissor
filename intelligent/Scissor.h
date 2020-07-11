//#include "stdafx.h"
#include<opencv2/opencv.hpp>
#include<iostream>
#include<algorithm>
#include<cmath>
#include"PriorityQueue.h"
//ע�����õ����������ȶ��У��Լ����ؽڵ�����ݽṹ������skeleton�Ĵ��룬�������3
using namespace cv;
//�ڵ�״̬
#define INITIAL 0
#define ACTIVE 1
#define EXPAND 2
//Ȩֵ
#define WG 0.43f
#define WZ 0.43f
#define WD 0.14f
//Fz�����ű�����������ͼ�ľ�ϸ�̶ȣ�����ÿ��ͨ����С��15.0f������һЩ�Ǳ�Ե��ͱ�Ե���ϣ�ͨ��Fg�����֣������Ϳ��Ժ���һЩϸС�ı�Ե
#define FzTrs 15.0f
//���������ݴ�Χ������Խ������ƶ���λ�ÿ���Լ����ȷ�����ǣ�ͬ���������겻�ò���

//15.0f,�����ڴ��¿���Ҫ��Ե��������1.0f�����Ǿ�ϸ����
struct PixelNode
{
	//���ؽڵ��Ӧ���ص������
	int col, row;
	//���˸��ڽӵ�Ļ���
	double linkCost[8];
	//�ýڵ��ھӽڵ������
	long long indexs[8];
	//�ýڵ��������е�����
	long long thisIndex;
	//�ڵ��״̬
	int state;
	//���ӵ㵽����ڵ���ܻ���
	double totalCost;
	// ǰһ���ڵ�
	PixelNode *prevNode;
	//��ʱ
	TickMeter tm;
	//��¼��ʾʱ����ݶ�ֵ
	double countTime;
	//��¼�ػ����
	double redraw;
	//�ڵ��ʼ��
	PixelNode() : linkCost{ 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f },
		indexs{ -1,-1,-1,-1,-1 ,-1,-1,-1 },
		thisIndex(0),
		prevNode(nullptr),
		col(0),
		row(0),
		state(INITIAL),
		totalCost(0.0),
		countTime(0.0),
		redraw(0.0)
	{}
	// this function helps to locate neighboring node in node buffer. 
	void nbrNodeOffset(int &offsetX, int &offsetY, int linkIndex)
	{
		//���������ĵ��ƫ������Ҳ����Ϊλ������֮��  p-q

		/*
		*  321
		*  4 0
		*  567
		*/

		//�����������л��ж��Ƿ�Խ�硣

		if (linkIndex == 0)
		{
			offsetX = 1;
			offsetY = 0;
		}
		else if (linkIndex == 1)
		{
			offsetX = 1;
			offsetY = -1;
		}
		else if (linkIndex == 2)
		{
			offsetX = 0;
			offsetY = -1;
		}
		else if (linkIndex == 3)
		{
			offsetX = -1;
			offsetY = -1;
		}
		else if (linkIndex == 4)
		{
			offsetX = -1;
			offsetY = 0;
		}
		else if (linkIndex == 5)
		{
			offsetX = -1;
			offsetY = 1;
		}
		else if (linkIndex == 6)
		{
			offsetX = 0;
			offsetY = 1;
		}
		else if (linkIndex == 7)
		{
			offsetX = 1;
			offsetY = 1;
		}
	}
	//����p-q�����������ص����ھӵ�λ������
	cv::Vec2f genVector(int linkIndex)
	{
		int offsetX, offsetY;
		nbrNodeOffset(offsetX, offsetY, linkIndex);
		return cv::Vec2f(offsetX, offsetY);
	}
	// used by the binary heap operation, 
	// pqIndex is the index of this node in the heap.
	// you don't need to know about it to finish the assignment;
	int pqIndex;

	int &Index(void)
	{
		return pqIndex;
	}

	int Index(void) const
	{
		return pqIndex;
	}
};
//������������Ƚ����ص���ܻ��Ѵ�С
inline int operator < (const PixelNode &a, const PixelNode &b)
{
	return a.totalCost<b.totalCost;
}

class Scissor
{
private:
	//ԭͼ
	Mat src;
	//����ͼ
	Mat fzMat, fdMat, fgMat;
	//x����ƫ������y����ƫ����
	Mat Dx, Dy;
	//�Ƿ���������
	bool seedflag ;  
	//ԭͼ��ͨ������
	int channels;
	//ԭͼһ�е�Ԫ�ظ���
	long long colNums;
	//��������
	long long totalnum;
	
protected:
	//laplacian����
	Mat laplacian = (Mat_<char>(3, 3) <<
		0, -1, 0,
		-1, 4, -1,
		0, -1, 0);
	//��scharr�˲�������sobelЧ������,�Դ�ƽ���;�����Ͳ���gaussianBlur
	Mat scharrx = (Mat_<char>(3, 3) <<
		-3, 0, 3,
		-10, 0, 10,
		-3, 0, 3);//��������xƫ΢��

	Mat scharry = (Mat_<char>(3, 3) <<
		-3, -10, -3,
		0, 0, 0,
		3, 10, 3);//��������yƫ΢��

 
public:
		 Mat oldFgMat;
	    //ԭͼ��Ӧ�ڵ���
	     PixelNode * nodes;
	     Scissor();
		 Scissor(Mat& origin);
		 //�Ƿ��������ӵ�
		 bool isSeed();
		 //���ͨ�ú���������OpenCV�Դ���filter2D����kernel�Ĵ�С>=11ʱ�������DFT�����Ч�ʡ�
		 //���Զ�����߽�
		 void filter(const Mat& origin, Mat& dst, Mat& kernel);
		 //����ƫ����
		 void calculateDxDy();
		 //����Fz,ѡ��ÿ��ͨ����ͬʱС����ֵ�ĵ㣬����Ϊ��
		 void calculateFz();
		 //ѡ������ͨ�����ݶ�ֵ���ĵ㣬�ݶ�ֵԽ�󻨷�Խ��
		 void calculateFg();
		 //��Lpq��p_q��λ��ʸ����Dp,�����жϷ���Lpq�ǽ��
		 void getLpq(Vec2f&p_q, Vec2f&Dp, Vec2f&Lpq);
		 //����Fd,������ͨ����x�����ƫ����֮����Ϊ�ܵģ�x�����ƫ������y����ͬ��Ȼ�����ÿһ�����ص�ĵ�λ�����ݶ�ʸ��������һ��Mat��
		 void calculateFd();
		 //�ۼ�Fz,Fg,��������Ӧ��Ȩ��
		 void accumulateCost();
		 //�����ӵ㵽�õ�����·��
		 void liveWire(long long index);//const int & col, const int & row
		 //����seedFlag
		 void setSeed(bool flag);
		 //����һ��ͼ����������
		 long long getTotalNums();
		 //ѡ��ÿ���ڵ㵽�ھ���̵ķ�����Ϊ�����Ļ��ѣ��ٳ���255 ��Ϊ�õ������ֵ
		 void showGray();

		 void calculateFg(Mat& myMat);

		 long long cursorSnap(long long index);
		 virtual ~Scissor();
};

