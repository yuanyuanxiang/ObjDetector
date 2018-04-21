#include "pyCaller.h"

wchar_t pyCaller::pyHome[] = { 0 };

tfOutput pyCaller::ParseResult(PyObject *pRetVal, tfOutput *tf)
{
	tfOutput out;
	out = tf ? *tf : out;
	if (PyTuple_Check(pRetVal))
	{
		PyArrayObject *temp = (PyArrayObject *)PyTuple_GetItem(pRetVal, 0);
		if (NULL == tf)
			out = tfOutput(temp->dimensions[0]);
		My_DECREF(temp);

		int nSize = PyTuple_Size(pRetVal);
		for(int i = 0; i < nSize; ++i)
		{
			PyArrayObject *pMatrix = (PyArrayObject *)PyTuple_GetItem(pRetVal, i);
			switch (i)
			{
			case _boxes:
				{
					int x1 = pMatrix->dimensions[0], x2 = pMatrix->dimensions[1], x3 = pMatrix->dimensions[2];
					memcpy(out.boxes, pMatrix->data, x1 * x2 * x3 * sizeof(float));
				}
				break;
			case _scores:
				{
					int x1 = pMatrix->dimensions[0], x2 = pMatrix->dimensions[1];
					memcpy(out.scores, pMatrix->data, x1 * x2 * sizeof(float));
				}
				break;
			case _classes:
				{
					int x1 = pMatrix->dimensions[0], x2 = pMatrix->dimensions[1];
					memcpy(out.classes, pMatrix->data, x1 * x2 * sizeof(float));
				}
				break;
			case _num_detections:
				{
					int x1 = pMatrix->dimensions[0];
					memcpy(out.counts, pMatrix->data, x1 * sizeof(float));
				}
				break;
			default:
				break;
			}
			My_DECREF(pMatrix);
		}
	}
	return out;
}
