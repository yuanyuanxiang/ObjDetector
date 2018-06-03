## python 深度学习目标跟踪检测代码
## 用法: 在Windows命令行输入
#        python detect.py FileName
## 对指定的图片文件或图像数据进行目标检测
## 2018-4-13

import sys
sys.path.append('.')
import time
import tensorflow as tf
import numpy as np
from PIL import Image

# 模型位置
PATH_TO_CKPT = 'frozen_inference_graph.pb'

# 初始化图
detection_graph = tf.Graph()
with detection_graph.as_default():
    od_graph_def = tf.GraphDef()
    with tf.gfile.GFile(PATH_TO_CKPT, 'rb') as fid:
        serialized_graph = fid.read()
        od_graph_def.ParseFromString(serialized_graph)
        tf.import_graph_def(od_graph_def, name='')

detection_graph.as_default()
config = tf.ConfigProto()
config.gpu_options.allow_growth = True
sess = tf.Session(graph=detection_graph, config=config)

# 检测图像数据
def test_src(src):
    image_np = np.array(src).astype(np.uint8)
    image_np_expanded = np.expand_dims(image_np, axis=0)

    boxes = detection_graph.get_tensor_by_name('detection_boxes:0')
    scores = detection_graph.get_tensor_by_name('detection_scores:0')
    classes = detection_graph.get_tensor_by_name('detection_classes:0')
    num_detections = detection_graph.get_tensor_by_name('num_detections:0')
    image_tensor = detection_graph.get_tensor_by_name('image_tensor:0')
    (boxes, scores, classes, num_detections) = sess.run(
        [boxes, scores, classes, num_detections],
        feed_dict={image_tensor: image_np_expanded})

    return (boxes, scores, classes, num_detections)

# 检测图片文件
def test_image(path):
    try:
        image = Image.open(path)
        print('>> Run test on image:', path)
    except IOError:
        print('IOError: File is not accessible.')
        return
    start_time = time.time()
    boxes, scores, classes, num_detections = test_src(image)
    use_time = time.time() - start_time
    print('{} elapsed time: {:.3f}s'.format(time.time(), use_time))
    print('boxes.type =', type(boxes))
    print('boxes.shape =', boxes.shape)
    print('boxes.dtype =', boxes.dtype)
    print('boxes =', boxes)
    print('scores.shape =', scores.shape)
    print('classes.shape =', classes.shape)
    print('num_detections =', num_detections)
    return (boxes, scores, classes, num_detections)

# 激活GPU
test_image('image.jpg')

if __name__ == '__main__':
    test_image('image.jpg' if (1 == len(sys.argv)) else sys.argv[1])
