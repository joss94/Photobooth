# -*- coding: utf-8 -*-
"""
Created on Wed Feb 24 09:29:05 2021

@author: josselin_manceau
"""

import sys
import time
import getopt
import os
import numpy as np
import cv2
import tensorflow
import tensorflow.compat.v1 as tf
from PIL import Image
import shutil

def getImageFiles(dir):
    imgs = []
    valid_images =[".jpg", ".png"]
    for f in os.listdir(dir):
        ext = os.path.splitext(f)[1]
        if ext.lower() in valid_images:
            imgs.append(dir + "/" + f) 

    return imgs


def load_graph(frozen_graph_filename):
    # We load the protobuf file from the disk and parse it to retrieve the 
    # unserialized graph_def
    with tf.gfile.GFile(frozen_graph_filename, "rb") as f:
        graph_def = tf.GraphDef.FromString(f.read())

    with tf.Graph().as_default() as graph:
        tf.import_graph_def(graph_def, name="prefix")
    
    return graph

def detect_people(imagePath, sess):
    
    print(imagePath)
    print("Preparing image...\n")

    blur = None

    # Read image and convert as PIL image
    img = cv2.imread(imagePath)

    cv2_im = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    pil_im = Image.fromarray(cv2_im)
    
    #Adjust image size
    width, height = pil_im.size
    resize_ratio = 513.0 / max(width, height)
    target_size = (int(resize_ratio * width), int(resize_ratio * height))
    resized_image = pil_im.convert('RGB').resize(target_size, Image.ANTIALIAS)
   
    print("DNN inference...\n")
    # Forward pass on DNN
    batch_seg_map = sess.run(
            'prefix/SemanticPredictions:0',
            feed_dict={'prefix/ImageTensor:0': [np.asarray(resized_image)]})
    seg_map = batch_seg_map[0] 

    print("Formating results...\n")
    seg_map = ((seg_map == 15).astype(int) * 255).astype(np.uint8)

    # Convert PIL image back to cv2 and resize
    resized = cv2.resize(seg_map, (img.shape[1], img.shape[0]), interpolation = cv2.INTER_CUBIC)
    blur = cv2.blur(resized,(5,5))

    return blur


def main(argv):

    print("\n\n")

    modelPath = ''
    inputDir = ''
    outputDir = ''

    try:
       opts, args = getopt.getopt(argv,"hm:i:o:",["model=","input=","output="])

    except getopt.GetoptError:
       print('inference.py -m <model> -i <input_dir> -o <outputfile>')
       sys.exit(2)

    for opt, arg in opts:
        if opt == '-h':
            print('test.py -i <input_dir> -o <output_dir>')
            sys.exit()
        elif opt in ("-m", "--model"):
            modelPath = arg
        elif opt in ("-i", "--input"):
            inputDir = arg
        elif opt in ("-o", "--output"):
            outputDir = arg

    if not modelPath or not inputDir or not outputDir:
        print('inference.py -m <model> -i <input_dir> -o <output_dir>')
        sys.exit(2)

    print("\n")
    print ('Model path is ', modelPath)
    print ('Input dir is ', inputDir)
    print ('Output dir is ', outputDir)
    print("\n")

    print("Loading graph...\n")
    graph = load_graph(modelPath)
    
    with tf.Session(graph=graph) as sess:
    
        while True:

            images = getImageFiles(inputDir)

            for img in images:
                filename = os.path.split(img)[1]

                outputFile = outputDir + '/' + filename

                if not os.path.exists(outputFile):
                    time.sleep(1)

                    detection = detect_people(img, sess)
                    print("Saving resuts to disk...\n")
                    cv2.imwrite(outputFile, detection)

            time.sleep(0.1)
                        
                


if __name__ == "__main__":
    main(sys.argv[1:])
    

        