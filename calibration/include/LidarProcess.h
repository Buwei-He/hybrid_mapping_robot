#include <string>
#include <vector>
#include <pcl/common/common.h>
/** ros **/
#include <ros/ros.h>
#include <ros/package.h>
using namespace std;
typedef pcl::PointXYZI PointT;
typedef pcl::PointCloud<PointT> CloudT;
typedef pcl::PointCloud<PointT>::Ptr CloudPtr;

class LidarProcess{
public:
    string topic_name = "/livox/lidar";
    const string kPkgPath = ros::package::getPath("calibration");
    const string kDatasetPath = this->kPkgPath + "/data/lh3_global";
    /** essential params **/
    int spot_idx = 0;
    int view_idx = 0;
    int num_spots = 5;
    int num_views = 5; /** note: each spot contains several views, and each view at a specific spot called a pose**/
    int view_angle_init = -50;
    int view_angle_step = 25;
    int fullview_idx = (this->num_views-1) / 2;
    vector<vector<string>> poses_folder_path_vec;

    /** const parameters - original data - images and point clouds **/
    const bool kDenseCloud = true; /** true means merge the dense cloud and create fullview dense cloud,
 * otherwise it will create icp sparse cloud and fullview sparse cloud to be used in visualization **/
    const bool kProjByIntensity = true;
    static const int kNumRecPcds = 500; /** dense point cloud used for reconstruction **/
    static const int kNumIcpPcds = 20; /** sparse point cloud used for ICP registration **/
    const int kFlatRows = 2000;
    const int kFlatCols = 4000;
    const double kRadPerPix = (M_PI * 2) / 4000;
    const bool kHiddenPtsFilter = false; /** hidden points filter of pixels' space in Func SphereToPlane **/

    /** tags and maps **/
    typedef struct Tags {
        int label; /** label = 0->empty pixel; label = 1->normal pixel **/
        int num_pts; /** number of points **/
        vector<int> pts_indices;
        float mean;
        float sigma; /** sigma is the standard deviation estimation of lidar edge distribution **/
        float weight;
        int num_hidden_pts;
    }Tags; /** "Tags" here is a struct type, equals to "struct Tags", LidarProcess::Tags **/
    typedef vector<vector<Tags>> TagsMap;
    vector<vector<TagsMap>> tags_map_vec; /** container of tagsMaps of each pose **/

    /** coordinates of edge pixels (which are considered as the edge) **/
    typedef vector<vector<int>> EdgePixels;
    vector<vector<EdgePixels>> edge_pixels_vec;

    /** spatial coordinates of edge points (center of distribution) **/
    typedef vector<vector<double>> EdgePts;
    vector<vector<EdgePts>> edge_pts_vec;

    /** mean position of the lidar pts in a specific pixel space **/
    vector<vector<CloudPtr>> edge_cloud_vec; /** container of edgeClouds of each pose **/

    /** rigid transformation generated by ICP at different poses(vertical angle) **/
    vector<vector<Eigen::Matrix4f>> pose_trans_mat_vec;

    /***** Extrinsic Parameters *****/
    struct Extrinsic {
        double rx = 0, ry = 0, rz = 0,
               tx = 0, ty = 0, tz = 0;
    } extrinsic;

    /** File Path of the Specific Scene **/
    struct PoseFilePath {
        PoseFilePath()= default;
        PoseFilePath(string& spot_path, string& pose_path) {
            this->fullview_recon_folder_path = spot_path + "/fullview_recon";
            this->fullview_dense_cloud_path = this->fullview_recon_folder_path + "/fullview_dense_cloud.pcd";
            this->fullview_sparse_cloud_path = this->fullview_recon_folder_path + "/fullview_sparse_cloud.pcd";
            this->output_folder_path = pose_path + "/outputs/lidar_outputs";
            this->dense_pcds_folder_path = pose_path + "/dense_pcds";
            this->icp_pcds_folder_path = pose_path + "/icp_pcds";
            this->edge_img_path = pose_path + "/edges/lidEdge.png";
            this->result_folder_path = pose_path + "/results";
            this->dense_pcd_path = this->output_folder_path + "/lidDense" + to_string(kNumRecPcds) + ".pcd";
            this->icp_pcd_path = this->output_folder_path + "/icp_cloud.pcd";
            this->pose_trans_mat_path = this->output_folder_path + "/pose_trans_mat.txt";
            this->flat_img_path = this->output_folder_path + "/flatLidarImage.bmp";
            this->polar_pcd_path = this->output_folder_path + "/lidPolar.pcd";
            this->cart_pcd_path = this->output_folder_path + "/lidCartesian.pcd";
            this->tags_map_path = this->output_folder_path + "/tags_map.txt";
            this->edge_pts_coordinates_path = this->output_folder_path + "/lid3dOut.txt";
            this->edge_fisheye_projection_path = this->output_folder_path + "/lidTrans.txt";
            this->params_record_path = this->output_folder_path + "/ParamsRecord.txt";
        }
        /** pose **/
        string output_folder_path;
        string dense_pcds_folder_path;
        string icp_pcds_folder_path;
        string edge_img_path;
        string result_folder_path;
        string dense_pcd_path;
        string icp_pcd_path;
        string pose_trans_mat_path;
        string flat_img_path;
        string polar_pcd_path;
        string cart_pcd_path;
        string tags_map_path;
        string edge_pts_coordinates_path;
        string edge_fisheye_projection_path;
        string params_record_path;
        /** spot **/
        string fullview_recon_folder_path;
        string fullview_dense_cloud_path;
        string fullview_sparse_cloud_path;
    };
    vector<vector<struct PoseFilePath>> poses_files_path_vec;

    /** Degree Map **/
    std::map<int, int> degree_map;

public:
    LidarProcess();
    /***** Point Cloud Generation *****/
    static int ReadFileList(const string &folder_path, vector<string> &file_list);
    void CreateDensePcd();
    void ICP();
    void CreateFullviewPcd();
    void BagToPcd(string bag_file);

    /***** Edge Related *****/
    void EdgeToPixel();
    void ReadEdge();
    vector<vector<double>> EdgeCloudProjectToFisheye(vector<double> _p);
    vector<double> Kde(vector<vector<double>> edge_pixels, int row_samples, int col_samples);
    void EdgeExtraction();

    /***** LiDAR Pre-Processing *****/
    std::tuple<CloudPtr, CloudPtr> LidarToSphere();
    void SphereToPlane(const CloudPtr& polar_cloud, const CloudPtr& cart_cloud);
    void PixLookUp(const CloudPtr& cart_cloud);

    /***** Get and Set Methods *****/
    void SetExtrinsic(vector<double> _p) {
        this->extrinsic.rx = _p[0];
        this->extrinsic.ry = _p[1];
        this->extrinsic.rz = _p[2];
        this->extrinsic.tx = _p[3];
        this->extrinsic.ty = _p[4];
        this->extrinsic.tz = _p[5];
    }

    void SetSpotIdx(int spot_idx) {
        this->spot_idx = spot_idx;
    }

    void SetViewIdx(int view_idx) {
        this->view_idx = view_idx;
    }
};