import sys
import os
import matplotlib.pyplot as plt # plt 用于显示图片
import numpy as np

def show_image(src):
    lena = np.array(src).astype(np.uint8)
    print('shape =', lena.shape)
    plt.imshow(lena) # 显示图片
    plt.axis('off') # 不显示坐标轴
    plt.show()
