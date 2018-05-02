# 功能说明：将xml转换为csv。
# 使用说明：python xml_to_csv.py dir
# 注解：其中dir即存放xml的目录。
# 结果：在dir下面生成"images.csv"文件。

import os
import sys
import glob
import pandas as pd
import xml.etree.ElementTree as ET


def xml_to_csv(path):
    xml_list = []
    for xml_file in glob.glob(path + '/*.xml'):
        tree = ET.parse(xml_file)
        root = tree.getroot()
        for member in root.findall('object'):
            value = (path + '/' + root.find('filename').text,
                     int(root.find('size')[0].text),
                     int(root.find('size')[1].text),
                     member[0].text,
                     int(member[4][0].text),
                     int(member[4][1].text),
                     int(member[4][2].text),
                     int(member[4][3].text)
                     )
            xml_list.append(value)
    column_name = ['filename', 'width', 'height', 'class', 'xmin', 'ymin', 'xmax', 'ymax']
    xml_df = pd.DataFrame(xml_list, columns=column_name)
    return xml_df

if __name__ == '__main__':
    if len(sys.argv) == 1:
        image_path='.'
    else:
        image_path = sys.argv[1]
    xml_df = xml_to_csv(image_path)
    xml_df.to_csv(image_path + '/images.csv', index=None)
    print('Successfully converted xml to csv.')
