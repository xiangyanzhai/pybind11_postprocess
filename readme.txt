1. python3 setup.py  build
2. 复制生成的so文件到项目对应位置

light_pose 输入尺寸变化，需要修改human_pose_estimator.cpp ,里面有human_pose_estimation_25，和human_pose_estimation,修改对应的
heatMapDims2 heatMapDims3