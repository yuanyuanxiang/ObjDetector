#include "python.h"
#include <map>
#include <string>
#include "StrTransfer.h"
#include "config.h"

#pragma once

// 最多可检测目标个数
#define MAX_BOXES_NUM 100

// Py_DECREF 导致程序二次运行时崩溃
#define My_DECREF(p) 

// 包含 numpy 中的头文件arrayobject.h
#include "..\Lib\site-packages\numpy\core\include\numpy\arrayobject.h"


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
* @note 由于该结构需要被频繁使用, 故该类通过引用计数ref管理内存, \n 
    如有内存泄漏, 请联系本人
************************************************************************/
class tfOutput
{
protected:
	int *ref;				// 引用计数
	int addref() const { return ++ (*ref); }
	int removeref() const { return -- (*ref); }
	void destroy()			// 销毁
	{
		if (0 == removeref())
		{
			if (n)
				OUTPUT("======> 为tfOutput回收内存[0x%x].\n", ref);
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
	* @brief 初始化每类个数为0
	*/
	inline void zeros() { if(counts) memset(counts, 0, n * sizeof(float)); }

	/**
	* @brief 构造一个n类tensorflow模型参数
	* @note 默认构造的为空参数
	*/
	tfOutput(int class_num = 0)
	{
		memset(this, 0, sizeof(tfOutput));
		n = class_num;
		ref = new int(1);
		if (n)
		{
			OUTPUT("======> 为tfOutput分配内存[0x%x].\n", ref);
			boxes = new float[n * MAX_BOXES_NUM * 4]();
			scores = new float[n * MAX_BOXES_NUM]();
			classes = new float[n * MAX_BOXES_NUM]();
			counts = new float[n]();
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
		if (this != &o)// 防止自己赋值给自己
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
		}
		return *this;
	}

	// 访问指定元素
	inline float at(int r, int c, int i = 0) const
	{
		return boxes[i * (MAX_BOXES_NUM*4) + r * 4 + c];
	}

	// 打印第i类识别结果
	void PrintBoxes(int i = 0) const 
	{
		if (i < n)
		{
			const float *p = boxes + i * (MAX_BOXES_NUM*4);
			for (int i = 0; i < MAX_BOXES_NUM; ++i)
			{
				OUTPUT("%f\t %f\t %f\t %f\n", *p++, *p++, *p++, *p++);
			}
		}
	}
};

/************************************************************************
* @struct Item
* @brief 类别信息结构体（类别名称, 类别ID）
************************************************************************/
struct Item 
{
	int id;				// 类别ID(从1开始)
	char name[64];		// 类别名称
	Item() { id = 0; memset(name, 0, sizeof(name)); }
	Item(const char *_name, int _id) { strcpy_s(name, _name); id = max(_id, 1); }
};

/************************************************************************
* @class LabelMap
* @brief 类别标签
************************************************************************/
class labelMap
{
public:
	int num;			// 类别个数（至少一个）
	Item *items;		// 类别信息
	labelMap() { num = 0; items = 0; }
	~labelMap() { if(items) delete [] items; }

	// 创建类别标签
	void Create(int n) { num = max(n, 1); if(0 == items) items = new Item[num]; }
	// 销毁类别标签
	void Destroy() { num = 0; if(items) delete [] items; items = 0; }
	// 插入新的类别
	void InsertItem(const Item & it) { if(it.id > 0 && it.id <= num) items[it.id - 1] = it; }
	// 根据ID获取类名
	const char* getItemName(int id) const { return (id > 0 && id <= num) ? items[id - 1].name : ""; }
};

/************************************************************************
* @class pyCaller
* @brief python调用类: 适用于对图片、视频进行识别
* @author 袁沅祥, 2018-4-11
************************************************************************/
class pyCaller
{
private:
	static wchar_t pyHome[_MAX_PATH];			// python路径
	PyObject* pModule;							// python模块
	std::map<std::string, PyObject*> pFunMap;	// 函数列表
	bool bMultiThread;							// 多线程

	// 初始化 numpy 执行环境，主要是导入包
	// python2.7用void返回类型，python3.0以上用int返回类型
	inline int init_numpy()
	{
		import_array();
		return 1;
	}

	// 解析python结果
	tfOutput ParseResult(PyObject *pRetVal, tfOutput *tf);

public:
	// 设置python安装目录
	static bool SetPythonHome(const char *py)
	{
		char pyExe[_MAX_PATH] = { 0 };
		strcat_s(pyExe, py);
		strcat_s(pyExe, "\\python.exe");
		if (-1 != _access(pyExe, 0))
		{
			size_t s;
			mbstowcs_s(&s, pyHome, py, strlen(py));
			OUTPUT("======> SetPythonHome: %s\n", py);
			return true;
		}
		else
		{
			OUTPUT("======> SetPythonHome: home don't include python.exe.\n", py);
			return false;
		}
	}

	/**
	* @brief 构造一个pyCaller对象
	*/
	pyCaller(bool multi_thread = true)
	{
		pModule = NULL;
		bMultiThread = multi_thread;
	}

	/**
	* @brief 初始化pyCaller对象，接收py脚本名称作为传入参数
	*/
	bool Init(const char * module_name)
	{
		if (pyHome[0] && NULL == pModule)
		{
			clock_t t = clock();
			Py_SetPythonHome(pyHome);
			Py_Initialize();
			if (NUMPY_IMPORT_ARRAY_RETVAL == init_numpy())
			{
				OUTPUT("init_numpy failed.\n");
				return false;
			}
			if (bMultiThread)
				PyEval_InitThreads(); // 多线程支持
			PyObject *py = PyImport_ImportModule(module_name);
			if (bMultiThread && PyEval_ThreadsInitialized())
				PyEval_SaveThread();
			if (NULL == py)
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
			pModule = py;
		}
		else
		{
			OUTPUT("Py_SetPythonHome is not called.\n");
		}
		return pModule;
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
		if (pyHome[0])
		{
			if (bMultiThread && !PyGILState_Check())
				PyGILState_Ensure();
			Py_Finalize();
		}
	}

	// 是否加载了指定模块
	bool IsModuleLoaded() const { return pModule; }

	/**
	* @brief 使用前激活指定名称的函数
	*/
	bool ActivateFunc(const char * func_name)
	{
		if (NULL == pModule)
			return false;
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
		PyObject *pFunc = NULL;

		PyGILState_STATE gstate;
		int nHold = bMultiThread ? PyGILState_Check() : TRUE;
		if (!nHold) gstate = PyGILState_Ensure();
		Py_BEGIN_ALLOW_THREADS;
		Py_BLOCK_THREADS;

		pFunc =  pModule ? PyObject_GetAttrString(pModule, func_name) : 0;

		Py_UNBLOCK_THREADS;
		Py_END_ALLOW_THREADS;
		if (!nHold) PyGILState_Release(gstate);

		pFunMap.insert(std::make_pair(func_name, pFunc));

		return pFunc;
	}

	/**
	* @brief 调用python脚本中的指定函数
	*/
	tfOutput CallFunction(const char * func_name, const char *arg, tfOutput *tf = NULL)
	{
		tfOutput out;
		out = tf ? *tf : out;
		PyObject *pFunc = pFunMap[func_name];
		if (pFunc)
		{
			PyGILState_STATE gstate;
			int nHold = bMultiThread ? PyGILState_Check() : TRUE;
			if (!nHold)gstate = PyGILState_Ensure();
			Py_BEGIN_ALLOW_THREADS;
			Py_BLOCK_THREADS;

			const char *utf8 = MByteToUtf8(arg);
			PyObject* pArg = Py_BuildValue("(s)", utf8);
			delete [] utf8;
			if (pArg)
			{
				PyObject* pRetVal = PyEval_CallObject(pFunc, pArg);
				if (pRetVal)
					out = ParseResult(pRetVal, tf);
			}

			Py_UNBLOCK_THREADS;
			Py_END_ALLOW_THREADS;
			if (!nHold)PyGILState_Release(gstate);
		}
		return out;
	}
	/**
	* @brief 调用python脚本中的指定函数
	*/
	tfOutput CallFunction(const char * func_name, PyObject *arg, tfOutput *tf = NULL)
	{
		tfOutput out;
		out = tf ? *tf : out;
#if STATIC_DETECTING
		static PyObject *pFunc = pFunMap[func_name];
#else 
		PyObject *pFunc = pFunMap[func_name];
#endif
		if (pFunc)
		{
			PyGILState_STATE gstate;
			int nHold = bMultiThread ? PyGILState_Check() : TRUE;
			if (!nHold)gstate = PyGILState_Ensure();
			Py_BEGIN_ALLOW_THREADS;
			Py_BLOCK_THREADS;

			PyObject* pRetVal = PyEval_CallObject(pFunc, arg);
			if (pRetVal)
				out = ParseResult(pRetVal, tf);

			Py_UNBLOCK_THREADS;
			Py_END_ALLOW_THREADS;
			if (!nHold)PyGILState_Release(gstate);
		}
		return out;
	}
};
