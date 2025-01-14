#include <iostream>
#include <thread>
#include <boost/thread/thread.hpp>

#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>

#include <pcl/visualization/cloud_viewer.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/visualization/mouse_event.h> //鼠标事件
#include <pcl/visualization/keyboard_event.h>//键盘事件

#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/filters/passthrough.h>
#include <pcl/filters/uniform_sampling.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/project_inliers.h>
#include <pcl/ModelCoefficients.h>

// ros 
#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <ros/package.h>

#include <Eigen/Core>

#include "vtkCamera.h"
#include <vtkRenderWindow.h>

using namespace std;

typedef pcl::PointXYZ Point;
typedef pcl::PointCloud<Point> Cloud;

pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_in (new pcl::PointCloud<pcl::PointXYZ>);
pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_polygon (new pcl::PointCloud<pcl::PointXYZ>);
pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_cliped (new pcl::PointCloud<pcl::PointXYZ>);

pcl::PointXYZ curP,lastP;
bool flag = false; //判断是不是第一次点击
bool isPickingMode = false;
unsigned int line_id = 0;
bool clip_mode, fix_radius, save_pcd_en = 0;
double search_radius, sample_radius, k_samples, c1, c2 = 0;
float z_lb = -10;
float z_ub = 10;
boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer;

boost::shared_ptr<pcl::visualization::PCLVisualizer> init_viewer();
void getScreentPos(double* displayPos, double* world,void* viewer_void);
void keyboardEventOccurred(const pcl::visualization::KeyboardEvent& event,void* viewer_void);
void mouseEventOccurred(const pcl::visualization::MouseEvent& event,void* viewer_void);
int inOrNot1(int poly_sides, double *poly_X, double *poly_Y, double x, double y);
void projectInliers(void*);
// void computeCovariances(pcl::PointCloud<Point>::ConstPtr cloud,
//                         const pcl::KdTreeFLANN<Point>::Ptr kdtree,
//                         string save_vec_path);
void displayRegion( pcl::PointCloud<Point>::ConstPtr cloud,
                    string save_vec_path);
// void testCovariances(Cloud::ConstPtr target_, 
//                     string save_vec_path);
void testRegion(Cloud::ConstPtr target_,
                string save_vec_path);

int main(int argc, char** argv)
{
    ros::init (argc, argv, "segment");
    ros::NodeHandle nh;
    std::string package_dir = ros::package::getPath("calibration");
    string data_path, save_path, save_vec_path;
    nh.getParam("data_path", data_path);
    nh.getParam("save_path", save_path);
    nh.getParam("save_vec_path", save_vec_path);
    nh.getParam("c2", c2);
    nh.getParam("c1", c1);
    nh.getParam("search_radius", search_radius);
    nh.getParam("sample_radius", sample_radius);
    nh.getParam("k_samples", k_samples);
    nh.getParam("z_lb", z_lb);
    nh.getParam("z_ub", z_ub);
    nh.getParam("clip_mode", clip_mode);
    nh.getParam("fix_radius", fix_radius);
    nh.getParam("save_pcd", save_pcd_en);
    data_path = package_dir + data_path;
    save_path = package_dir + save_path;
    save_vec_path = package_dir + save_vec_path;

    viewer = init_viewer();
    std::cout << "read cloud_in" << endl;
    {
        pcl::PCDReader reader;
        reader.read(data_path, *cloud_in);
        
        pcl::PassThrough<pcl::PointXYZ> pt;	// 创建滤波器对象
        pt.setInputCloud(cloud_in);			//设置输入点云
        pt.setFilterFieldName("z");			//设置滤波所需字段x
        pt.setFilterLimits(z_lb, z_ub);		//设置x字段过滤范围
        pt.setFilterLimitsNegative(false);	//默认false，保留范围内的点云；true，保存范围外的点云
        pt.filter(*cloud_in);

        viewer->addPointCloud(cloud_in, "cloud_in");

    }
    std::cout<<"cloud_in size:"<<cloud_in->size()<<endl;

    Cloud::ConstPtr source_ (cloud_in);
    testRegion(source_, save_vec_path);

    while (!viewer->wasStopped())
    {
        viewer->spinOnce(100);
        boost::this_thread::sleep(boost::posix_time::microseconds(100000));
    }

    if (cloud_cliped->points.size() == 0) {pcl::copyPointCloud(*cloud_in, *cloud_cliped); }
    cout << cloud_cliped->points.size() << endl;
    if (save_pcd_en) {
        cloud_cliped->height = 1;
        cloud_cliped->width = cloud_cliped->points.size();
        pcl::io::savePCDFileBinary(save_path, *cloud_cliped);
    }

    // Cloud::ConstPtr target_ (cloud_cliped);
    // testCovariances(target_, save_vec_path);

    return 0;
}

//初始化
boost::shared_ptr<pcl::visualization::PCLVisualizer> init_viewer()
{
    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
    viewer->setBackgroundColor(0, 0, 0);
    viewer->addCoordinateSystem(1.0);
    viewer->setWindowName("Mouse & Keyboard Events");

    viewer->addPointCloud(cloud_polygon,"polyline");
    viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR,1,0,0,"polyline");
    viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,8,"polyline");

    viewer->registerKeyboardCallback(keyboardEventOccurred, (void*)viewer.get());
    viewer->registerMouseCallback(mouseEventOccurred, (void*)viewer.get());

    return (viewer);
}

/**
 * @brief getScreentPos     屏幕坐标转换至世界坐标
 * @param displayPos        输入：屏幕坐标
 * @param world             输出：世界坐标
 * @param viewer_void       输入：pclViewer
 */
void getScreentPos(double* displayPos, double* world,void* viewer_void)
{
    pcl::visualization::PCLVisualizer* viewer = static_cast<pcl::visualization::PCLVisualizer*> (viewer_void);
    vtkRenderer* renderer{viewer->getRendererCollection()->GetFirstRenderer()};
    // First compute the equivalent of this display point on the focal plane
    double fp[4], tmp1[4],  eventFPpos[4];
    renderer->GetActiveCamera()->GetFocalPoint(fp);
    fp[3] = 0.0;
    renderer->SetWorldPoint(fp);
    renderer->WorldToDisplay();
    renderer->GetDisplayPoint(tmp1);

    tmp1[0] = displayPos[0];
    tmp1[1] = displayPos[1];

    renderer->SetDisplayPoint(tmp1);
    renderer->DisplayToWorld();

    renderer->GetWorldPoint(eventFPpos);
    // Copy the result
    for (int i=0; i<3; i++)
    {
        world[i] = eventFPpos[i];
    }
}

//键盘事件
void keyboardEventOccurred(const pcl::visualization::KeyboardEvent& event,void* viewer_void)
{
    pcl::visualization::PCLVisualizer* viewer = static_cast<pcl::visualization::PCLVisualizer*> (viewer_void);

    if (event.getKeySym() == "x" && event.keyDown()){
        isPickingMode = !isPickingMode;
        if(isPickingMode){
            std::cout<<endl<<"start draw"<<endl;

            line_id = 0;
            cloud_polygon->clear();
            flag=false;
        }
        else{
            std::cout<<endl<<"stop draw"<<endl;
            projectInliers(viewer_void);
            viewer->removeAllShapes();
        }
    }
}

//鼠标事件
void mouseEventOccurred(const pcl::visualization::MouseEvent& event,void* viewer_void)
{
    pcl::visualization::PCLVisualizer* viewer = static_cast<pcl::visualization::PCLVisualizer*> (viewer_void);
    if (event.getButton() == pcl::visualization::MouseEvent::LeftButton &&
            event.getType() == pcl::visualization::MouseEvent::MouseButtonRelease)
    {
        std::cout << "Left mouse button released at position (" << event.getX() << ", " << event.getY() << ")" << std::endl;

        if(isPickingMode){
            double world_point[3];
            double displayPos[2];
            displayPos[0]=double(event.getX()),displayPos[1]=double(event.getY());
            getScreentPos(displayPos, world_point,viewer_void);

            std::cout<<endl<<world_point[0]<<','<<world_point[1]<<','<<world_point[2]<<endl;

            curP=pcl::PointXYZ(world_point[0], world_point[1], world_point[2]);
            if(!flag)flag=true;
            else {
                char str1[512];
                sprintf(str1, "line#%03d", line_id++);//名字不能重复
                viewer->addLine(lastP,curP,str1);
            }
            lastP=curP;
            //切割点云添加
            cloud_polygon->push_back(curP);
        }
    }
}

/**
 * @brief inOrNot1
 * @param poly_sides    平面上绘制多边形的顶点数
 * @param poly_X        顶点的x坐标数组
 * @param poly_Y        顶点的y坐标数组
 * @param x             目标点云的x坐标
 * @param y             目标点云的y坐标
 * @return
 */
int inOrNot1(int poly_sides, double *poly_X, double *poly_Y, double x, double y)
{
    int i, j;
    j = poly_sides - 1;
    int res = 0;

    //对每一条边进行遍历，该边的两个端点，有一个必须在待检测点(x,y)的左边，且两个点中，有一个点的y左边比p.y小，另一个点的y比p.y大。
    for (i = 0; i < poly_sides; i++) {
        if (( (poly_Y[i] < y && poly_Y[j] >= y) || (poly_Y[j] < y && poly_Y[i] >= y) ) && (poly_X[i] <= x || poly_X[j] <= x))
        {   //用水平的直线与该边相交，求交点的x坐标。
            res ^= ((poly_X[i] + (y - poly_Y[i]) / (poly_Y[j] - poly_Y[i])  *(poly_X[j] - poly_X[i])) < x);
        }
        j = i;
    }
    return res;
}

//裁剪
void projectInliers(void* viewer_void)
{
    pcl::visualization::PCLVisualizer* viewer = static_cast<pcl::visualization::PCLVisualizer*> (viewer_void);
    //输入的点云

    double focal[3]={0}; double pos[3] = {0};
    vtkRenderer* renderer{viewer->getRendererCollection()->GetFirstRenderer()};
    renderer->GetActiveCamera()->GetFocalPoint(focal);
    renderer->GetActiveCamera()->GetPosition(pos);

    std::cout<<"focal: "<<focal[0]<<','<<focal[1]<<','<<focal[2]<<endl;
    std::cout<<"pos: "<<pos[0]<<','<<pos[1]<<','<<pos[2]<<endl;

    //获取焦点单位向量
    pcl::PointXYZ eyeLine1 = pcl::PointXYZ(focal[0] - pos[0], focal[1] - pos[1], focal[2] - pos[2]);

    float norm = sqrt(pow(eyeLine1.x, 2) + pow(eyeLine1.y, 2) + pow(eyeLine1.z, 2));//模长
    pcl::PointXYZ eyeLine = pcl::PointXYZ(eyeLine1.x / norm, eyeLine1.y / norm, eyeLine1.z / norm);//单位向量 法向量

    //创建一个平面
    pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients());//ax+by+cz+d=0
    coefficients->values.resize(4);
    coefficients->values[0] = eyeLine.x;
    coefficients->values[1] = eyeLine.y;
    coefficients->values[2] = eyeLine.z;
    coefficients->values[3] = 0;

    //创建保存结果投影的点云
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloudIn_Prj(new pcl::PointCloud<pcl::PointXYZ>);//输入的点云
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloudCiecle_result(new pcl::PointCloud<pcl::PointXYZ>);//绘制的线
    // 创建滤波器对象
    pcl::ProjectInliers<pcl::PointXYZ> proj;//建立投影对象
    proj.setModelType(pcl::SACMODEL_PLANE);//设置投影类型
    proj.setInputCloud(cloud_polygon);//设置输入点云
    proj.setModelCoefficients(coefficients);//加载投影参数
    proj.filter(*cloudCiecle_result);//执行程序，并将结果保存

    // 创建滤波器对象
    pcl::ProjectInliers<pcl::PointXYZ> projCloudIn;//建立投影对象
    projCloudIn.setModelType(pcl::SACMODEL_PLANE);//设置投影类型
    projCloudIn.setInputCloud(cloud_in);//设置输入点云
    projCloudIn.setModelCoefficients(coefficients);//加载投影参数
    projCloudIn.filter(*cloudIn_Prj);//执行程序，并将结果保存

    int ret = -1;
    double *PloyXarr = new double[cloudCiecle_result->points.size()];
    double *PloyYarr = new double[cloudCiecle_result->points.size()];
    for (int i = 0; i < cloudCiecle_result->points.size(); i++)
    {
        PloyXarr[i] = cloudCiecle_result->points[i].x;
        PloyYarr[i] = cloudCiecle_result->points[i].y;
    }

    cloud_cliped->clear();
    for (int i = 0; i < cloudIn_Prj->points.size(); i++)
    {
        ret = inOrNot1(cloud_polygon->points.size(), PloyXarr, PloyYarr, cloudIn_Prj->points[i].x, cloudIn_Prj->points[i].y);
        if (clip_mode == ret) // 1 - 表示在里面
        {
            cloud_cliped->points.push_back(cloud_in->points[i]);
        }//表示在外面
    }

    viewer->removeAllPointClouds();
    viewer->addPointCloud(cloud_cliped, "aftercut");

    cloud_in->clear();
    pcl::copyPointCloud(*cloud_cliped, *cloud_in);

    viewer->getRenderWindow()->Render();
}

void testRegion(Cloud::ConstPtr target_, string save_vec_path){
    displayRegion(target_, save_vec_path);
}

void displayRegion( pcl::PointCloud<Point>::ConstPtr cloud,
                    string save_vec_path ) { // vector<Mat3d>
    int k_interv_ = int(k_samples);
    Eigen::Matrix3d cov;
    Eigen::Vector3d mean;
    std::vector<int> nn_indecies; nn_indecies.reserve (k_interv_);
    std::vector<float> nn_dist_sq; nn_dist_sq.reserve (k_interv_);
    Cloud::Ptr cloud_uniform (new Cloud);
    Cloud::Ptr cloud_cp (new Cloud);
    Cloud::Ptr cloud_n (new Cloud);

    std::vector<float> precision_rec;
    float avg_precision = 0;

    pcl::UniformSampling<Point> us;
    us.setRadiusSearch(sample_radius);
    us.setInputCloud(cloud);
    us.filter(*cloud_uniform);
    pcl::KdTreeFLANN<Point>::Ptr kdtree (new pcl::KdTreeFLANN<Point>);
    (*kdtree).setInputCloud(cloud);

    for (Point &query_point: cloud_uniform->points) {
        // Zero out the cov and mean
        cov.setZero ();
        mean.setZero ();

        // Search for the K nearest neighbours
        if (!fix_radius) {
            kdtree->nearestKSearch(query_point, k_interv_, nn_indecies, nn_dist_sq);
        }
        else {
            kdtree->radiusSearch(query_point, search_radius, nn_indecies, nn_dist_sq);
        }

        // Find the covariance matrix
        for(int j = 0; j < nn_indecies.size(); j++) {
            const Point &pt = (*cloud)[nn_indecies[j]];

            mean[0] += pt.x;
            mean[1] += pt.y;
            mean[2] += pt.z;

            cov(0,0) += pt.x*pt.x;

            cov(1,0) += pt.y*pt.x;
            cov(1,1) += pt.y*pt.y;

            cov(2,0) += pt.z*pt.x;
            cov(2,1) += pt.z*pt.y;
            cov(2,2) += pt.z*pt.z;
        }

        mean /= static_cast<double> (nn_indecies.size());
        // Get the actual covariance
        for (int k = 0; k < 3; k++)
        for (int l = 0; l <= k; l++)
        {
            cov(k,l) /= static_cast<double> (nn_indecies.size());
            cov(k,l) -= mean[k]*mean[l];
            cov(l,k) = cov(k,l);
        }

        // Compute the SVD (covariance matrix is symmetric so U = V')
        Eigen::JacobiSVD<Eigen::Matrix3d> svd(cov, Eigen::ComputeFullU);
        Eigen::Vector3d eigen_vec = svd.singularValues();
        cout << sqrt(eigen_vec(0)) << ", " << sqrt(eigen_vec(1)) << ", " << sqrt(eigen_vec(2)) << ", " << endl;
        if (eigen_vec(1) > eigen_vec(2) * c2 && eigen_vec(0) * c1 < eigen_vec(1)){
            Cloud::Ptr cloud_tmp (new Cloud);
            pcl::copyPointCloud(*cloud, nn_indecies, *cloud_tmp);
            *cloud_n += *cloud_tmp;

            float precision = sqrt(eigen_vec(2)) * 6; 
            precision_rec.push_back(precision);
        }
    }

    ofstream outfile;
    outfile.open(save_vec_path, ios::out);
    if (!outfile.is_open()) {
        cout << "Open file failure" << endl;
    }
    for (float &p : precision_rec) {
        avg_precision += p;
        outfile << p << endl;
    }
    
    avg_precision /= precision_rec.size();

    outfile.close();

    cout << "Count: \n" << precision_rec.size() << endl;
    cout << "Precision: \n" << avg_precision << endl;

    int r = 0;
    int g = 255;
    int b = 0;
    pcl::visualization::PointCloudColorHandlerCustom<Point> cloud_color_h(cloud_n, r, g, b);
    viewer->addPointCloud(cloud_n, cloud_color_h, "cloud_n");
}

// void computeCovariances(pcl::PointCloud<Point>::ConstPtr cloud,
//                         const pcl::KdTreeFLANN<Point>::Ptr kdtree,
//                         string save_vec_path ) { // vector<Mat3d>
//     int k_interval_ = int(cloud->size() / k_samples_ / 2) + 1;
//     Eigen::Matrix3d cov;
//     Eigen::Vector3d mean;
//     vector<float> precision_rec;
//     float avg_precision = 0;
//     std::vector<int> nn_indecies; nn_indecies.reserve (k_interval_);
//     std::vector<float> nn_dist_sq; nn_dist_sq.reserve (k_interval_);

//     typename pcl::PointCloud<Point>::const_iterator points_iterator = cloud->begin ();
//     for(;
//         points_iterator < cloud->end ();
//         points_iterator += k_interval_)
//     {
//         const Point &query_point = *points_iterator;
//         // Zero out the cov and mean
//         cov.setZero ();
//         mean.setZero ();

//         // Search for the K nearest neighbours
//         if (!fix_radius) {
//             kdtree->nearestKSearch(query_point, k_interval_, nn_indecies, nn_dist_sq);
//         }
//         else {
//             kdtree->radiusSearch(query_point, search_radius, nn_indecies, nn_dist_sq);
//             // k_interval_ = nn_indecies.size();
//         }

//         // Find the covariance matrix
//         for(int j = 0; j < nn_indecies.size(); j++) {
//             const Point &pt = (*cloud)[nn_indecies[j]];

//             mean[0] += pt.x;
//             mean[1] += pt.y;
//             mean[2] += pt.z;

//             cov(0,0) += pt.x*pt.x;

//             cov(1,0) += pt.y*pt.x;
//             cov(1,1) += pt.y*pt.y;

//             cov(2,0) += pt.z*pt.x;
//             cov(2,1) += pt.z*pt.y;
//             cov(2,2) += pt.z*pt.z;
//         }

//         mean /= static_cast<double> (nn_indecies.size());
//         // Get the actual covariance
//         for (int k = 0; k < 3; k++)
//         for (int l = 0; l <= k; l++)
//         {
//             cov(k,l) /= static_cast<double> (nn_indecies.size());
//             cov(k,l) -= mean[k]*mean[l];
//             cov(l,k) = cov(k,l);
//         }

//         // Compute the SVD (covariance matrix is symmetric so U = V')
//         Eigen::JacobiSVD<Eigen::Matrix3d> svd(cov, Eigen::ComputeFullU);
//         Eigen::Vector3d eigen_vec = svd.singularValues();
//         float precision = 0.0f;
//         cout << sqrt(eigen_vec(0)) << ", " << sqrt(eigen_vec(1)) << ", " << sqrt(eigen_vec(2)) << ", " << endl;
//         // if (eigen_vec(1) > eigen_vec(2) * 2 || eigen_vec(0) * 0.5 < eigen_vec(1)){
//             precision = sqrt(eigen_vec(2)) * 6; 
//             precision_rec.push_back(precision);
//         // }
//     }

//     ofstream outfile;
//     outfile.open(save_vec_path, ios::out);
//     if (!outfile.is_open()) {
//         cout << "Open file failure" << endl;
//     }
//     for (float &p : precision_rec) {
//         avg_precision += p;
//         outfile << p << endl;
//     }
    
//     avg_precision /= precision_rec.size();

//     outfile.close();

//     cout << "Count: \n" << precision_rec.size() << endl;
//     cout << "Precision: \n" << avg_precision << endl;
// }

