#include "python.h"
#include <map>
#include <string>
#include "StrTransfer.h"

#pragma once

// 最多可检测目标个数
#define MAX_BOXES_NUM 100

// Py_DECREF 导致程序二次运行时崩溃
#define My_DECREF(p) 

#ifdef _AFX
#define OUTPUT TRACE
#else
#define OUTPUT printf
#endif

// 包含 numpy 中的头文件arrayobject.h
#include "..\Lib\site-packages\numpy\core\include\numpy\arrayobject.h"
using namespace std;


enum 
{
	_boxes = 0, 
	_scores, 
	_classes, 
	_num_detections
};

/************************************************************************
* @class tfOutput
* @brief tensorflow模型输出的参数结构
************************************************************************/
class tfOutput
{
protected:
	int *ref;				// 引用计数
	int addref() const { return ++ (*ref); }
	int removeref() const { return -- (*ref); }
	void destroy()			// 销毁
	{
		if (ref && 0 == removeref())
		{
			SAFE_DELETE_ARRAY(ref);
			SAFE_DELETE_ARRAY(boxes);
			SAFE_DELETE_ARRAY(scores);
			SAFE_DELETE_ARRAY(classes);
			SAFE_DELETE_ARRAY(counts);
		}
	}
public:
	int n;					// 类别个数(n)
	float *boxes;			// n个 100 x 4 矩阵
	float *scores;			// n x 100 矩阵
	float *classes;			// n x 100 矩阵
	float *counts;			// n 维数组(每类个数)

	/**
	* @brief 构造一个n类tensorflow模型参数
	* 默认构造的为空参数
	*/
	tfOutput(int class_num = 0)
	{
		memset(this, 0, sizeof(tfOutput));
		n = class_num;
		if (n)
		{
			boxes = new float[n * MAX_BOXES_NUM * 4]();
			scores = new float[n * MAX_BOXES_NUM]();
			classes = new float[n * MAX_BOXES_NUM]();
			counts = new float[n]();
			ref = new int(1);
		}
	}
	~tfOutput()
	{
		destroy();
	}
	tfOutput(const tfOutput &o)
	{
		ref = o.ref;
		n = o.n;
		boxes = o.boxes;
		scores = o.scores;
		classes = o.classes;
		counts = o.counts;
		addref();
	}
	tfOutput operator = (const tfOutput &o)
	{
		// 先清理本对象
		destroy();
		// this被o代替
		ref = o.ref;
		n = o.n;
		boxes = o.boxes;
		scores = o.scores;
		classes = o.classes;
		counts = o.counts;
		addref();
		return *this;
	}

	// 访问指定元素
	inline float p(int r, int c, int i = 0) const
	{
		return boxes[i * (MAX_BOXES_NUM*4) + r * 4 + c];
	}

	// 打印第i类识别结果
	void PrintBoxes(int i = 0) const 
	{
		if (i < n)
		{
			const float *header = boxes + i * (MAX_BOXES_NUM*4);
			for (int i = 0; i < MAX_BOXES_NUM; ++i)
			{
				const float *row = header;
				OUTPUT("%f\t %f\t %f\t %f\n", *row++, *row++, *row++, *row++);
				header += 4;
			}
		}
	}
};

/************************************************************************
* @class pyCaller
* @brief python调用类: 适用于对图片、视频进行识别
* @author 袁沅祥, 2018-4-11
************************************************************************/
class pyCaller
{
private:
	PyObject* pModule;							// python模块
	std::map<std::string, PyObject*> pFunMap;	// 函数列表

	// 初始化 numpy 执行环境，主要是导入包
	// python2.7用void返回类型，python3.0以上用int返回类型
	inline int init_numpy(){

		import_array();
		return 0;
	}

	// 解析python结果
	tfOutput ParseResult(PyObject *pRetVal);

public:
	/**
	* @brief 构造一个pyCaller对象，接收py脚本名称作为传入参数
	*/
	pyCaller(const char * module_name)
	{
		clock_t t = clock();
		Py_SetPythonHome(L"D:/Anaconda3/envs/tfgpu");
		Py_Initialize();
		init_numpy();
		pModule = PyImport_ImportModule(module_name);
		if (NULL == pModule)
			OUTPUT("PyImport_ImportModule failed.\n");
		t = clock() - t;
		char szOut[128];
		sprintf_s(szOut, "PyImport_ImportModule using %d ms.\n", t);
		OutputDebugStringA(szOut);
#ifndef _AFX
		printf(szOut);
#endif
		if (0 == Py_IsInitialized())
		{
			OUTPUT("Py_IsInitialized = 0.\n");
		}
	}

	/**
	* @brief 反初始化python环境，释放python对象的内存
	*/
	~pyCaller()
	{
		if (pModule)
			Py_DECREF(pModule);
		for (std::map<std::string, PyObject*>::iterator p = pFunMap.begin(); 
			p != pFunMap.end(); ++p)
			if (p->second) Py_DECREF(p->second);
		Py_Finalize();
	}

	/**
	* @brief 使用前激活指定名称的函数
	*/
	bool ActivateFunc(const char * func_name)
	{
		bool bFind = false;
		std::string fun(func_name);
		for (std::map<std::string, PyObject*>::iterator p = pFunMap.begin(); 
			p != pFunMap.end(); ++p)
		{
			if (p->first == fun)
			{
				bFind = true;
				break;
			}
		}
		if (bFind)
			return true;

		PyObject *pFunc =  pModule ? PyObject_GetAttrString(pModule, func_name) : 0;
		pFunMap.insert(std::make_pair(func_name, pFunc));

		return pFunc;
	}

	/**
	* @brief 调用python脚本中的指定函数
	*/
	tfOutput CallFunction(const char * func_name, const char *arg)
	{
		tfOutput out;
		PyObject *pFunc = pFunMap[func_name];
		if (pFunc)
		{
			const char *utf8 = MByteToUtf8(arg);
			PyObject* pArg = Py_BuildValue("(s)", utf8);
			delete [] utf8;
			if (NULL == pArg)
				return out;
			PyObject* pRetVal = PyEval_CallObject(pFunc, pArg);
			if (NULL == pRetVal)
				return out;
			out = ParseResult(pRetVal);
		}
		return out;
	}
	/**
	* @brief 调用python脚本中的指定函数
	*/
	tfOutput CallFunction(const char * func_name, PyObject *arg)
	{
		tfOutput out;
		PyObject *pFunc = pFunMap[func_name];
		if (pFunc)
		{
 			PyObject* pRetVal = PyEval_CallObject(pFunc, arg);
			if (NULL == pRetVal)
				return out;
			out = ParseResult(pRetVal);
		}
		return out;
	}
};
