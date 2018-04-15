## python深度学习目标跟踪检测代码
## 用法: 在Windows命令行输入
#        python infer.py FileName
## 对指定的图片文件进行目标检测
## 2018-4-10

import sys
sys.path.append('..')
import os
import time
import tensorflow as tf
import numpy as np
from PIL import Image
import cv2  
import numpy
from object_detection.utils import label_map_util
from object_detection.utils import visualization_utils as vis_util

# 模型位置
PATH_TO_CKPT = 'D:/GitHub/my_path/vest/result/frozen_inference_graph.pb'
# 类别信息
PATH_TO_LABELS = 'D:/GitHub/my_path/vest/vest_label_map.pbtxt'
# 分类数
NUM_CLASSES = 1

# 初始化图
label_map = label_map_util.load_labelmap(PATH_TO_LABELS)
categories = label_map_util.convert_label_map_to_categories(
    label_map, max_num_classes=NUM_CLASSES, use_display_name=True)
category_index = label_map_util.create_category_index(categories)

detection_graph = tf.Graph()
with detection_graph.as_default():
    od_graph_def = tf.GraphDef()
    with tf.gfile.GFile(PATH_TO_CKPT, 'rb') as fid:
        serialized_graph = fid.read()
        od_graph_def.ParseFromString(serialized_graph)
        tf.import_graph_def(od_graph_def, name='')

config = tf.ConfigProto()
config.gpu_options.allow_growth = True
detection_graph.as_default()
sess = tf.Session(graph=detection_graph, config=config)
image_tensor = detection_graph.get_tensor_by_name('image_tensor:0')

def test_image(path):
    image = Image.open(path)
    image_np = np.array(image).astype(np.uint8)
    image_np_expanded = np.expand_dims(image_np, axis=0)

    start_time = time.time()
    boxes = detection_graph.get_tensor_by_name('detection_boxes:0')
    scores = detection_graph.get_tensor_by_name('detection_scores:0')
    classes = detection_graph.get_tensor_by_name('detection_classes:0')
    num_detections = detection_graph.get_tensor_by_name('num_detections:0')
    (boxes, scores, classes, num_detections) = sess.run(
        [boxes, scores, classes, num_detections],
        feed_dict={image_tensor: image_np_expanded})
    use_time = time.time() - start_time
    print('{} elapsed time: {:.3f}s'.format(time.time(), use_time))

    vis_util.visualize_boxes_and_labels_on_image_array(
        image_np, np.squeeze(boxes), np.squeeze(classes).astype(np.int32), np.squeeze(scores),
        category_index, use_normalized_coordinates=True, line_thickness=2)
    cv2.imshow("object detection", image_np)
    cv2.waitKey(0)

if __name__ == '__main__':
    if len(sys.argv) == 1:
        print("Not enough arguments.\n")
    else:
        path=sys.argv[1]
        test_image(path)
