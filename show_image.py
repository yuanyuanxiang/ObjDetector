import sys
sys.path.append('.')
import os
import matplotlib.pyplot as plt # plt 用于显示图片
import matplotlib.image as mpimg # mpimg 用于读取图片
import numpy as np

def show_image(src):
    lena = np.array(src).astype(np.uint8)
    print(lena.shape)
    plt.imshow(lena) # 显示图片
    plt.axis('off') # 不显示坐标轴
    plt.show()
    test = np.array([[[1, 2, 3], [4, 5, 6]], [[0, 0, 0], [1, 1, 1]]])
    print(test.shape)
    return test, test
