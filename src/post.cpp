#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
namespace py = pybind11;
#include <algorithm>
#include <stdio.h>
using namespace std;
#define DIVIDE_AND_ROUND_UP(a, b) ((a + b - 1) / b)

#include "paf_score_graph.hpp"
#include "find_peaks.hpp"
#include "refine_peaks.hpp"
#include "munkres.hpp"
#include "connect_parts.hpp"
using namespace trt_pose;
using namespace trt_pose::parse;
unsigned int frame_number_trt=0;
const int topology[] = { 0, 1, 1, 8,
            2, 3, 1, 2,
            4, 5, 1, 5,
            6, 7, 2, 3,
            8, 9, 3, 4,
            10, 11, 5, 6,
            12, 13, 6, 7,
            14, 15, 8, 9,
            16, 17, 9, 10,
            18, 19, 10, 11,
            20, 21, 8, 12,
            22, 23, 12, 13,
            24, 25, 13, 14,
            26, 27, 1, 0,
            28, 29, 0, 15,
            30, 31, 15, 17,
            32, 33, 0, 16,
            34, 35, 16, 18,
            36, 37, 2, 17,
            38, 39, 5, 18,
            40, 41, 14, 19,
            42, 43, 19, 20,
            44, 45, 14, 21,
            46, 47, 11, 22,
            48, 49, 22, 23,
            50, 51, 11, 24 };

int counts[25];
int peaks[25*100*2];
float refined_peaks[25*100* 2];
float score_graph[26*100 * 100];

int connections[26*2 * 100];
int object_counts[1];
int objects[100*25];
int trt_init(){
    for (int i=0;i<25;i++){
    counts[i]=0;
    }
    for (int i=0;i<25*100*2;i++){
    peaks[i]=0;
    }
    for (int i=0;i<100*2;i++){
        refined_peaks[i]=0.0;
    }
    for (int i=0;i<26*100*100;i++){
     score_graph[i]=0.0;
    }
    for (int i=0;i<26*2*100;i++){
    connections[i]=-1;
    }

    object_counts[0]=0;

    for (int i=0;i<100*25;i++){
    objects[i]=-1;
    }
return 0;
}
#include "human_pose_estimator.hpp"
#include "render_human_pose.hpp"
//
using namespace cv;
using namespace human_pose_estimation;
using namespace human_pose_estimation_25;
////#define MIN(a,b) ((a) < (b) ? (a) : (b))
////#define MAX(a,b) ((a) > (b) ? (a) : (b))
////#define CLIP(a,min,max) (MAX(MIN(a, max), min))
////#define DIVIDE_AND_ROUND_UP(a, b) ((a + b - 1) / b)
//
unsigned int frame_number_light=0;
unsigned int frame_number_light_25=0;
human_pose_estimation::HumanPoseEstimator estimator(false);
human_pose_estimation_25::HumanPoseEstimator estimator_25(false);

py::array_t<float> trt_pose_process(py::array_t<float> heatmap_np,py::array_t<float> paf_np,int aa,int bb,int cc){
     trt_init();
     py::buffer_info  heatmap_buf=heatmap_np.request();
     py::buffer_info  paf_buf=paf_np.request();
     float* heatmap=(float *)heatmap_buf.ptr;
     float* paf=(float *)paf_buf.ptr;
     find_peaks_out_chw(counts, peaks, heatmap, aa, bb, cc, 100,0.1, 5);
     refine_peaks_out_chw(refined_peaks,counts,peaks,heatmap, aa, bb, cc,100, 5);
     paf_score_graph_out_khw(score_graph,topology,paf,counts,refined_peaks,26, 25 , bb, cc,100, 7);
     void *workspace = (void *)malloc(assignment_out_workspace(100));
     assignment_out_k(connections,score_graph,topology,counts,26,100,0.1,workspace);
     free(workspace);
     void *workspace2 = malloc(connect_parts_out_workspace(25, 100));

     connect_parts_out(object_counts,     // 1
                       objects,           // PxC
                       connections, // Kx2xM
                       topology,    // Kx4
                       counts,      // C
                       26, 25, 100,100,
                       workspace2) ;
      free(workspace2);
      int k;
        int c=0;
        unsigned int num=0;
        int *flag=new int[object_counts[0]];
        int thresh=6;
        for (int i=0;i<object_counts[0];i++){
            c=0;
            flag[i]=0;
            for (int j=0;j<25;j++){
                k=objects[i*25+j];
                if (k>=0) c++;
                if (c>thresh) break;
                }
            if (c<=thresh) continue;
            num++;
            flag[i]=1;
            }

     auto result = py::array_t<float>(50*num);
     py::buffer_info buf3 = result.request();
    float* class_probability_map = (float*)buf3.ptr;

    for (unsigned int i=0;i<50*num;i++){

        class_probability_map[i]=-1.0;
    }
    float a,b;
    c=0;
    for (int i =0;i<object_counts[0];i++){
        if (flag[i]==0) continue;
        for (int j=0;j<25;j++){
            k=objects[i*25+j];
            if (k>=0){
                a=refined_peaks[j*200+k*2+1]*448;
                b=refined_peaks[j*200+k*2]*448;
            }
            else{
                 a=-1.0;
                 b=-1.0;
            }
            class_probability_map[c]=a;
            c++;
            class_probability_map[c]=b;
            c++;
        }
    }
   delete[] flag;
   return result;
}



py::array_t<float> light_pose_process(py::array_t<float> heatmap_np,py::array_t<float> paf_np){
     py::buffer_info  heatmap_buf=heatmap_np.request();
     py::buffer_info  paf_buf=paf_np.request();

     float* heatmap=(float *)heatmap_buf.ptr;
     float* paf=(float *)paf_buf.ptr;

     std::vector<human_pose_estimation::HumanPose> poses = estimator.estimate(heatmap, paf);
    unsigned int num=0;
    for (human_pose_estimation::HumanPose const& pose : poses) {
        num++;
    }
    auto result = py::array_t<float>(36*num);
     py::buffer_info buf3 = result.request();
    float* class_probability_map = (float*)buf3.ptr;
    for (unsigned int i=0;i<36*num;i++){
        class_probability_map[i]=-1.0;
    }
     unsigned int c=0;
    float a,b;
    for (human_pose_estimation::HumanPose const& pose : poses) {
        for (auto const& keypoint : pose.keypoints) {
            a= keypoint.x*2;
            b=keypoint.y*2;
            if (a<0.0 || b<0.0) {
                a=-1.0;
                b=-1.0;
                }
            class_probability_map[c]=a;
            c++;
            class_probability_map[c]=b;
            c++;
        }
        }
     return result;

}
py::array_t<float> light_pose_process_25(py::array_t<float> heatmap_np,py::array_t<float> paf_np){
     py::buffer_info  heatmap_buf=heatmap_np.request();
     py::buffer_info  paf_buf=paf_np.request();
     float* heatmap=(float *)heatmap_buf.ptr;
     float* paf=(float *)paf_buf.ptr;
     std::vector<human_pose_estimation_25::HumanPose> poses = estimator_25.estimate(heatmap, paf);
    unsigned int num=0;
    for (human_pose_estimation_25::HumanPose const& pose : poses) {
        num++;
    }
    auto result = py::array_t<float>(50*num);
     py::buffer_info buf3 = result.request();
    float* class_probability_map = (float*)buf3.ptr;
    for (unsigned int i=0;i<50*num;i++){
        class_probability_map[i]=-1.0;
    }
    unsigned int c=0;
    float a,b;
    for (human_pose_estimation_25::HumanPose const& pose : poses) {
        for (auto const& keypoint : pose.keypoints) {
            a= keypoint.x*2;
            b=keypoint.y*2;
            if (a<0.0 || b<0.0) {
                a=-1.0;
                b=-1.0;
                }
            class_probability_map[c]=a;
            c++;
            class_probability_map[c]=b;
            c++;
        }
        }
     return result;
}

PYBIND11_MODULE(postprocess, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring
    m.def("trt_pose_process", &trt_pose_process, "trt_pose_process");
    m.def("light_pose_process", &light_pose_process, "light_pose_process");
    m.def("light_pose_process_25", &light_pose_process_25, "light_pose_process_25");
}