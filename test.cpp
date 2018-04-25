#include "config.h"
#include "python.h"

//////////////////////////////////////////////////////////////////////////
// Python 配置
#define PYTHON_HOME L"D:/Anaconda3/envs/tfgpu"

// 包含 numpy 中的头文件arrayobject.h
#include "..\Lib\site-packages\numpy\core\include\numpy\arrayobject.h"

// 初始化 numpy 执行环境，主要是导入包
// python2.7用void返回类型，python3.0以上用int返回类型
int init_numpy(){

	import_array();
	return 0;
}

/************************************************************************
* @class pyCaller
* @brief python调用类: 适用于对图片、视频进行识别
* @author 袁沅祥, 2018-4-11
************************************************************************/
class pyCaller
{
private:
	PyObject* pModule;							// python模块
	PyObject* pFunMap;							// 函数列表

public:
	/**
	* @brief 构造一个pyCaller对象，接收py脚本名称作为传入参数
	*/
	pyCaller(const char * module_name)
	{
		clock_t t = clock();
		Py_SetPythonHome(PYTHON_HOME);
		Py_Initialize();
		init_numpy();
		pModule = PyImport_ImportModule(module_name);
		if (NULL == pModule)
			OUTPUT("PyImport_ImportModule failed.\n");
		t = clock() - t;
		OUTPUT("PyImport_ImportModule using %d ms.\n", t);
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
		if (pFunMap)
			Py_DECREF(pFunMap);
		Py_Finalize();
	}
	/**
	* @brief 使用前激活指定名称的函数
	*/
	bool ActivateFunc(const char * func_name)
	{
		return (pFunMap =  pModule ? PyObject_GetAttrString(pModule, func_name) : 0);
	}
	/**
	* @brief 调用python脚本中的指定函数
	*/
	void CallFunction(const char * func_name, PyObject *arg)
	{
		if (pFunMap && arg)
			PyEval_CallObject(pFunMap, arg);
	}
};


int main(int argc, const char *argv[])
{
	const char *path = argc == 1 ? "image.jpg" : argv[1];

	pyCaller py("show_image");
	py.ActivateFunc("show_image");

	Mat im = imread(path);

	npy_intp dims[] = {im.rows, im.cols, im.step[1]}; // 给定维度信息
	// 生成包含这个多维数组的PyObject对象，使用PyArray_SimpleNewFromData函数
	// 第一个参数2表示维度，第二个为维度数组Dims,第三个参数指出数组的类型，第四个参数为数组
	PyObject *PyArray  = PyArray_SimpleNewFromData(3, dims, NPY_UBYTE, im.data); 
	// 同样定义大小与Python函数参数个数一致的PyTuple对象
	PyObject *ArgArray = PyTuple_New(1);
	PyTuple_SetItem(ArgArray, 0, PyArray); 
	// 调用函数，传入Numpy Array 对象
	py.CallFunction("show_image", ArgArray);

	return getchar();
}
