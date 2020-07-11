#include "stdafx.h"
#include "Scissor.h"
#include<algorithm>
const double SQRT2 = 1.4142135623730950488016887242097;
const double invSqrt2 = 1.0 / SQRT2;
const double PI = 3.141592654f;
const double invPi = 4.0 /( 3.0 * PI);
//const double _2Over3PI = 2.0 / (3.0 * PI);
extern int CURSERSNAPTRS = 10;
using namespace cv;
Scissor::Scissor()
{
}

Scissor::Scissor(Mat& origin)
{
	seedflag = false;

	src = origin.clone();

	totalnum = src.cols*src.rows;

	int& row = src.rows;

	int& col = src.cols;

	channels = src.channels();

	colNums = channels * src.cols;

	fzMat.create(src.size(), CV_32FC1);

	fgMat.create(src.size(), CV_32FC1);

	fdMat.create(src.size(), CV_32FC1);
	//ÿһ�����ص��Ӧһ�����ؽڵ�
	nodes = new PixelNode[totalnum+10];

	int index = 0;
	//��ʼ��ÿ���ڵ�
	for(int i = 0;i < row ; i++)
		for (int j = 0; j < col; j++)
		{
			index = i * col + j;
			nodes[index].row = i;
			nodes[index].col = j;
			nodes[index].totalCost = 0.0f;
			nodes[index].thisIndex = index;
			//��������ٷ���ռ��ʱ���Ѿ���ʼ������
		}
	//����ƫ����
	calculateDxDy();
	calculateFz();
	calculateFg();
	calculateFd();
	accumulateCost();
	//liveWire(150);
}

void Scissor::filter(const Mat& origin, Mat& dst, Mat& kernel)
{
	//�Ѿ���������ת����32λ��߾��ȣ�filter��Զ�ͨ������ͨ������
	filter2D(origin, dst, CV_32F, kernel);
}

void Scissor::calculateDxDy()
{
	filter(src, Dx, scharrx);

	filter(src, Dy, scharry);
}
//modified
void Scissor::calculateFz()
{
	int type = 0;
	//��Ӧ��ͨ������ͨ�����ɲ�ͬ��Mat
	if (channels == 1)
	{
		type = CV_32FC1;
	}
	else {
		type = CV_32FC3;
	}

	Mat tmpFz(src.rows, src.cols, type);
	//��Ÿ�˹�˲������ͼ
	Mat afterBlur(src.rows, src.cols, src.type());

	Size mysize;
	//��5*5�ĸ�˹��
	mysize.width = mysize.height =  5;

	//��Ϊ������������У����ø�˹�˲����� !!!
	GaussianBlur(src, afterBlur,mysize , 0, 0);

	filter(afterBlur, tmpFz, laplacian);//--------afterBlur----------------

	float* p, *q;

	int count = 0;
	
	for (int i = 0; i < tmpFz.rows; i++)
	{
		p = tmpFz.ptr<float>(i);

		q = fzMat.ptr<float>(i);

		//��Ϊ��Ե����laplacian���Ӿ��������ͨ���Ķ��׵�����С���㣬����ֱ�����������׵�����С��������ص㣬���Ǳ�Ե��
		for (int j = 0; j < colNums - 2; j += 3)//������������  
											 //j = j + 3;���������Ͷ�أ�3�����أ����˰����Сʱ������
		{
			
			if (p[j] < FzTrs && p[j + 1] < FzTrs && p[j + 2] < FzTrs)
			{
				q[count++] = 0;
			}
			else
			{
				q[count++] = 1;
			}
		}
		//��ԭ
		count = 0;
	}
	p = q = NULL;
}
//modified
void Scissor::calculateFg()
{
	float* p;
	float* q;
	float* g;
	float gMax = -1.0f;
	float gMin = 6553666.0f;

	int type = 0;
	//��Ӧ��ͨ������ͨ�����ɲ�ͬ��Mat
	if (channels == 1)
	{
		type = CV_32FC1;
	}
	else {
		type = CV_32FC3;
	}
	//�������ͨ�����ݶȷ�ֵ
	Mat tmpFg(src.rows, src.cols, type);
	//���ݶȷ�ֵ
	for (int i = 0; i < Dx.rows; i++)
	{
		p = Dx.ptr<float>(i);
		q = Dy.ptr<float>(i);
		g = tmpFg.ptr<float>(i);
		for (int j = 0; j < colNums; j++)
		{
			g[j] = sqrt(p[j] * p[j] + q[j] * q[j]);

		}

	}
	//��¼ԭ�����ݶȷ�ֵ������·����ȴ��
	oldFgMat = tmpFg.clone();

	int count = 0;
	for (int i = 0; i < tmpFg.rows; i++)
	{
		p = tmpFg.ptr<float>(i);

		g = fgMat.ptr<float>(i);

		//ѡȡÿ��ͨ�������ֵ,Ч�����
		for (int j = 0; j < colNums - 2; j += 3)
		{
			
			//g[count] = (p[j] + p[j + 1] + p[j + 2]);///3.0f;//
			g[count] = max({ p[j],p[j + 1],p[j + 2] });

			if (g[count]>gMax)
			{
				gMax = g[count];
			}
			if (g[count] < gMin)
			{
				gMin = g[count];
			}

			count++;
		}

		count = 0;
	}

	float coefG = gMax - gMin;

	for (int i = 0; i < fgMat.rows; i++)
	{
		g = fgMat.ptr<float>(i);

		//�ݶȷ�ֵԽ�󣬱�Ե�Ŀ�����Խ�󣬷���Խ��
		for (int j = 0; j < fgMat.cols; j++)
		{
			g[j] = 1.0f - (g[j] - gMin) / coefG;
		}

	}

	p = q = g = NULL;

	//ע������ۼӵ�ʱ�򣬶Խ���Ҫ����sqrt(2);
}
//
void Scissor::calculateFd()//˼·�����ر�˳������
{
	//����ÿһ��ͨ�����ݶȷ���ĵ�λ��ʸ�������Ҵ浽һ��Mat������ظ���ȡ
	//��Ϊ�Ƕ�άʸ������������ͨ����Mat�洢
	Mat tmpFd(src.rows, src.cols, CV_32FC2);
	int count = 0 ;
	float* dx;
	float* dy;
	Vec2f* p , *q;
	/*if (channels == 1)
	{
		for (int i = 0; i < tmpFd.rows; i++)
		{
			count = 0;
			dx = Dx.ptr<float>(i);
			dy = Dy.ptr<float>(i);
			p = tmpFd.ptr<Vec2f>(i);//�޵���ʹ��Vec2f,����һ�仰������������vec2f�ķ�ʽ����,����ͬʱ��������ͨ��
			for (int j = 0; j < colNums; j++)
			{
				//��һ��
				p[count] = normalize(Vec2f(dy[j], -dx[j]));

				count++;
			}
		}
	}*/
	 if(channels == 3)
	{		
		float sumx;
		float sumy;
		for (int i = 0; i < tmpFd.rows; i++)
		{
			count = 0;
			dx = Dx.ptr<float>(i);
			dy = Dy.ptr<float>(i);
			p = tmpFd.ptr<Vec2f>(i);//�޵���ʹ��Vec2f,����һ�仰��������1.5h
			for (int j = 0; j < colNums - 2; j += 3)
			{
				//����CSDN�������Ĵ���ʽ��������ͨ����x����ƫ����ֵ���
				//��Ϊ������ʸ��֮�Ͷ������ķ���ı����������������죬����ʹ���ۼ�
				sumx = (dx[j] + dx[j + 1] + dx[j + 2]);
				
				sumy = (dy[j] + dy[j + 1] + dy[j + 2]);
				
				 p[count] = normalize(Vec2f(sumy, -sumx));

				count++;
			}
		}

	}

	dx = dy = NULL;

	Vec2f q_p, Lpq,tmpSum;//p-q , Lpq
	long long index = 0;
	int offSetRow, offSetCol;
	float dp,dq,tmp;
	//����genVector�Լ�nodes�Դ��ĺ���������p-q��λ�������������ھ����λ�õ�������


	//�������߽�Ԫ�ز����ǣ���������һȦ�����ǣ���Ϊ����һȦ�Ǳ߽�ֻ���ĸ�ת�ǵ㣬Fd��Ϊ�㣬���Ժ��Բ��ƣ���������
	for (int i = 1; i < tmpFd.rows-1; i++)
	{
		for (int j = 1; j < tmpFd.cols-1; j++)
		{
			//�õ���λ��ʸ��
			p = tmpFd.ptr<Vec2f>(i,j);
			//�ҵ���Ӧ���ؽڵ���±�,����ֱ��index++,��Ϊ��Ե��Ĭ��Ϊ��
			index = i * tmpFd.cols + j;
			
			for (int k = 0; k < 8; k++)
			{
				
				//�õ� Q - P ������ʽ��Vec2f�� 
				q_p = nodes[index].genVector(k);
				//��ÿһ���ھ���һ��Lpq,����һ��
				getLpq(q_p, (*p), Lpq);
				//ע��nbrNodeOffset�����ص����С��У���Ū���ˣ���������������������
				nodes[index].nbrNodeOffset(offSetCol,offSetRow,k);
				
				q = tmpFd.ptr<Vec2f>(i + offSetRow, j + offSetCol);
				//��dp(p,q) dq(p,q)������fD

				tmpSum = normalize(*p + *q);

				tmp = tmpSum.dot(Lpq);

				//dp = p->dot(Lpq);

				//dq = q->dot(Lpq);
				//----����ʵ��ʱ���ҽ�������λ��������ӣ��ٹ�һ����������Ӻ��������Lpq֮��ļнǣ���Ϊ��ʱ�����ļн�Ϊ3.0*pi/4,���Թ�һ����ϵ����4/��3.0*pi��
				nodes[index].linkCost[k] = invPi *(acos(tmp))*WD;//--_2Over3PI *(acos(dp)+acos(dq))*WD ---------------invPi *(acos(tmp))*WD--------------


			}
			
		}
		
	}

	p = q = NULL;

}
void Scissor::getLpq(Vec2f&p_q,Vec2f&Dp,Vec2f&Lpq)
{
	float invP_Q = 1.0f / sqrt(p_q.dot(p_q));

	if (Dp.dot(p_q) >= 0.0f)
	{
		Lpq = invP_Q * p_q;
	}
	else
	{
		Lpq = -1.0f * invP_Q * p_q;
	}

}
//�ۼӵ��ھ���,���Ż�
void Scissor::accumulateCost()
{
	long long index = 0;
	int offSetRow, offSetCol ,i,j,k;
	int rows = fzMat.rows, cols = fzMat.cols;
	int neighborRow, neighborCol;
	//long long neighborIndex;//˳����ھӵ�����Ҳ��¼����������������ڵ㣬Ϊ�����������ȶ��з���
	float *p, *q;
	for ( i = 0; i < rows; i++)
	{
		for (j = 0; j < cols; j++)
		{
			//index = i * cols + j;

			for (k = 0; k < 8; k++)
			{
				//�ȷ������ٷ����С�����������
				nodes[index].nbrNodeOffset(offSetCol, offSetRow, k);
				neighborRow = i + offSetRow;
				neighborCol = j + offSetCol;
				//Խ����
				if (neighborCol < 0 || neighborCol >= cols || neighborRow < 0 || neighborRow >= rows)
				{
					continue;
				}
				p = fzMat.ptr<float>(neighborRow, neighborCol);

				q = fgMat.ptr<float>(neighborRow, neighborCol);

				//neighborIndex = 
				//���������Ķ��ķ�ʽΪ����ֱ�����ڵ�Fg���ѳ���sqrt(2);
				if (k % 2 == 0)
				{
					nodes[index].linkCost[k] += (*p)*WZ + (*q)*WG*invSqrt2;//--------------------------���Ķ�
				}
				else
				{
					nodes[index].linkCost[k] += (*p)*WZ + (*q)*WG;
				}

				nodes[index].indexs[k] = neighborRow * cols + neighborCol;
			}

			index++;
		}
	}
}
//pixelNode���ݽṹ�޸ģ����ұ����죬����Զѵ�ʹ�û���һ�¡��������ȶ��У�֪����ô�þ����ˡ����Ż�������һ��tmpIndex��ֵ�����Ķ����Ͳ��ڼ���
void Scissor::liveWire(long long index)//const int & col , const int & row
{

	//�ֲ���ʼ���������Ȱ��ھӳ�ʼ�����ٴ��ھ����ң�������С�ģ�
	//������ھ��Ѿ���ʼ�����ˣ��ͱȽ����ӵ�ͨ������㵽�����ھӺ����ӵ�ͨ�������㵽���ľ���
	//������Ѹ�С�͸���updata(r);
	//ֱ���ѿ��ˣ�˵�����е㶼�Ѿ�����չ�ˡ�
	//����ȫ�����Լ�����֮ǰ���뷨�Լ�CSDN���������͵Ĳ���ӡ��д��

	//�������� X = COL  Y = ROW
	//int index = row * src.cols + col;
	//int offSetRow, offSetCol, neiBorRow, neiBorCol;

	CTypedPtrHeap<PixelNode> pq;

	int i = 0;

	long long tmpIndex;

	PixelNode * node = (nodes+index);

	node->totalCost = 0.0f;

	node->prevNode = NULL;

	seedflag = true;

	pq.Insert(node);

	while (!pq.IsEmpty())
	{
		//�ӶѶ�ȡ����С��Ԫ��
		node = pq.ExtractMin();
		node->state = EXPAND;
		for (i = 0; i < 8; i++)
		{
			tmpIndex = node->indexs[i];

			if (tmpIndex == -1 || nodes[tmpIndex].state == EXPAND)
			{
				continue;
			}
			
			else if (nodes[tmpIndex].state == INITIAL)
			{
				nodes[tmpIndex].totalCost = node->totalCost + node->linkCost[i];

				nodes[tmpIndex].state = ACTIVE;

				nodes[tmpIndex].prevNode = node;

				pq.Insert(&nodes[tmpIndex]);
			}
			else if (nodes[tmpIndex].state == ACTIVE)
			{
				if (nodes[tmpIndex].totalCost > node->totalCost + node->linkCost[i])
				{
					nodes[tmpIndex].totalCost = node->totalCost + node->linkCost[i];

					nodes[tmpIndex].prevNode = node;

					pq.Update(&nodes[tmpIndex]);

				}

			}
			
		}

	}

	//��Ϊÿһ�ε�������ظ�ʹ��nodes,���Զ�nodes�ı�־��������
	//-----------------------------------------------------------------------
	for (i = 0; i < totalnum; i++)
	{
		nodes[i].state = INITIAL;
	}

	//-----------------------------------------------------------------------


}

void Scissor::setSeed(bool flag)
{
	seedflag = flag;
}

void Scissor::showGray()
{
	Mat Gray = Mat(src.rows, src.cols, CV_8UC1);

	double pixVal = 0;

	for (int i = 0; i < totalnum; i++)
	{
		pixVal = min({ nodes[i].linkCost[0],nodes[i].linkCost[1], nodes[i].linkCost[2], nodes[i].linkCost[3], nodes[i].linkCost[4], nodes[i].linkCost[5],nodes[i].linkCost[6],nodes[i].linkCost[7] });
	
		*Gray.ptr<uchar>(nodes[i].row, nodes[i].col) = pixVal * 255.0;
	}

	imshow("Unequalized", Gray);

	Mat tmpGray = Gray.clone();

	//ֱ��ͼ���⻯�����ӶԱȶȡ�
	equalizeHist(Gray, tmpGray);

	imshow("equalizeHistGray", tmpGray);
	//gama��ǿ
	double coef = 1.0 / (100.0),gama = 0.6;

	for (int i = 0; i < tmpGray.rows; i++)
	{
		for (int j = 0; j < tmpGray.cols; j++)
		{
			*tmpGray.ptr<uchar>(i, j) *= coef;
			*tmpGray.ptr<uchar>(i, j) = pow(*tmpGray.ptr<uchar>(i,j), gama);
			*tmpGray.ptr<uchar>(i,j) *= 255;
		}
	}
	
	imshow("gamaGray", tmpGray);
	
	calculateFg(Gray);
	
	imshow("magnitude", Gray);

	tmpGray.~Mat();

	Gray.~Mat();
}
//������
long long Scissor::cursorSnap(long long index) 
{
	if (index <0 || index >= totalnum)
	{
		std::cout << "�������Խ��" << std::endl;
		return 0;
	}

	float MinFg, *p;
	
	long long maxIndex = index;
	
	int tmpRow = nodes[index].row, tmpCol = nodes[index].col;
	
	p = fgMat.ptr<float>(tmpRow, tmpCol);

	MinFg = *p;

	for (int i = nodes[index].row - CURSERSNAPTRS; i < nodes[index].row + CURSERSNAPTRS; i++)
	{
		if (i < 0 || i >= fgMat.rows)
		{
			continue;
		}
		for (int j = nodes[index].col - CURSERSNAPTRS; j < nodes[index].col + CURSERSNAPTRS; j++)
		{
			if (j < 0 || j >= fgMat.cols)
			{
				continue;
			}

			p = fgMat.ptr<float>(i,j);

			if (*p < MinFg)
			{
				tmpRow = i;
				tmpCol = j;
				MinFg = *p;
			}

		}

	}

	return tmpRow*fgMat.cols + tmpCol;

}

void Scissor::calculateFg(Mat& myMat)
{
	float* p;
	float* q;
	float* g;
	Mat dx, dy;

	filter(myMat, dx, scharrx);

	filter(myMat, dy, scharry);
	
	int	type = CV_32FC1;
	
	//��������ĻҶ�ֵ
	Mat tmpFg(myMat.rows, myMat.cols, type);
	//���ݶȷ�ֵ
	for (int i = 0; i < myMat.rows; i++)
	{
		p = dx.ptr<float>(i);
		q = dy.ptr<float>(i);
		g = tmpFg.ptr<float>(i);
		for (int j = 0; j < myMat.cols; j++)
		{
			//ֱ�����ų���255.0
			g[j] = sqrt(p[j] * p[j] + q[j] * q[j])/255.0;
		}

	}
	
	for (int i = 0; i < tmpFg.rows; i++)
	{
		g = tmpFg.ptr<float>(i);

		//�ݶȷ�ֵԽ�󣬱�Ե�Ŀ�����Խ�󣬷���Խ��
		for (int j = 0; j < tmpFg.cols; j++)
		{
			//�򵥵Ķ�ֵ����Ŀ�����ñ�ԵΪ��ɫ���Ǳ�ԵΪ��ɫ
			if (g[j] >= 1.0)
			{
				g[j] = 0.0;
			}
			else {
				g[j] = 1.0 - g[j];
			}
		}

	}
	tmpFg.copyTo(myMat);
	//waitKey(0);
	p = q = g = NULL;

	dx.~Mat();

	dy.~Mat();

	tmpFg.~Mat();
}

long long Scissor::getTotalNums()
{
	return totalnum;
}

bool Scissor::isSeed()
{
	return seedflag;

}

Scissor::~Scissor()
{
}
