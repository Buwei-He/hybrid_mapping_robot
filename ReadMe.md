# Coarse-to-fine Hybrid 3D Mapping System with Co-calibrated Omnidirectional Camera and Non-repetitive Mid-360 LiDAR
This work is developed by Ziliang Miao, Buwei He, Wenya Xie, supervised by Prof.Xiaoping Hong ([ISEE-Lab](https://isee.technology/), SDIM, SUSTech), and is accepted by IEEE Robotics and Automation Letters (RA-L).

Pre-print Paper: https://arxiv.org/pdf/2301.12934.pdf

Demo Video: https://www.youtube.com/watch?v=Uh0C9VL9YEQ

## 0. Abstract
This paper presents a novel 3D mapping robot with an omnidirectional field-of-view (FoV) sensor suite composed of a non-repetitive LiDAR (Livox Mid-360) and an omnidirectional camera (please consult Huanjun Technology, panoholic). Thanks to the non-repetitive scanning nature of the LiDAR, an automatic targetless co-calibration method is proposed to simultaneously calibrate the intrinsic parameters for the omnidirectional camera and the extrinsic parameters for the camera and LiDAR, which is crucial for the required step in bringing color and texture information to the point clouds in surveying and mapping tasks. Comparisons and analyses are made to target-based intrinsic calibration and mutual information (MI)-based extrinsic calibration, respectively. With this co-calibrated sensor suite, the hybrid mapping robot integrates both the odometry-based mapping mode and stationary mapping mode. Meanwhile, we proposed a new workflow to achieve coarse-to-fine mapping, including efficient and coarse mapping in a global environment with odometry-based mapping mode; planning for viewpoints in the region-of-interest (ROI) based on the coarse map (relies on the previous work); navigating to each viewpoint and performing finer and more precise stationary scanning and mapping of the ROI. The fine map is stitched with the global coarse map, which provides a more efficient and precise result than the conventional stationary approaches and the emerging odometry-based approaches, respectively.

<img src="readme_pics/robot.png" width=45% >
<img src="readme_pics/sensor_suite.png" width=45% >
<img src="readme_pics/nonrepetitive_scanning.png" width=45% >

## 1. Workflow
<img src="readme_pics/workflow.png" width=45% >

## 2. Calibration Results
<img src="readme_pics/cocalibration_result.png" width=45% >

## 3. Mapping Results
<img src="readme_pics/mapping_result.png" width=45% >
<img src="readme_pics/colorized_mapping.png" width=45% >

## 4. Prerequisites
### 4.1 **Ubuntu** and **ROS**
Version: Ubuntu 18.04.

Version: ROS Melodic. 

Please follow [ROS Installation](http://wiki.ros.org/ROS/Installation) to install.
### 4.2. **ceres-solver**
Version: ceres-solver 2.1.0

Please follow [Ceres-Solver Installation](http://ceres-solver.org/installation.html) to install.
### 4.3. **PCL**
Version: PCL 1.7.4

Version: Eigen 3.3.4

Please follow [PCL Installation](http://www.pointclouds.org/downloads/linux.html) to install.
### 4.4. **OpenCV**
Version: OpenCV 3.2.0

Please follow [OpenCV Installation](https://opencv.org/) to install.
### 4.5. **mlpack**
Version: mlpack 3.4.2

Please follow [mlpack Installation](https://mlpack.org/) to install.

### 4.6 Livox SDK and Livox ROS Driver (Optional)
The SDK and driver is used for dealing with Livox LiDAR.
Remenber to install [Livox SDK](https://github.com/Livox-SDK/Livox-SDK) before [Livox ROS Driver](https://github.com/Livox-SDK/livox_ros_driver).

### 4.7 MindVision SDK (Optional)
The SDK of the fisheye camera is in [MindVision SDK](http://www.mindvision.com.cn/rjxz/list_12.aspx?lcid=138).

## 5. Acknowledgements
Thanks for [CamVox](https://github.com/ISEE-Technology/CamVox), [Livox-SDK](https://github.com/Livox-SDK/livox_camera_lidar_calibration), [OCamCalib MATLAB Toolbox](https://sites.google.com/site/scarabotix/ocamcalib-omnidirectional-camera-calibration-toolbox-for-matlab), [Fast-LIO](https://github.com/hku-mars/FAST_LIO), and thanks to the help of Wenquan Zhao, Xiao Huang, Jian Bai.
