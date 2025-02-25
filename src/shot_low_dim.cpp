# include <Mshot/cbshot_bits.h>

#define MAXBUFSIZE  ((int) 1e6)

MatrixXf readMatrix(const char *filename)
{
    int cols = 0, rows = 0;
    double buff[MAXBUFSIZE];

    // Read numbers from file into buffer.
    ifstream infile;
    infile.open(filename);
    while (! infile.eof())
    {
        string line;
        getline(infile, line);

        int temp_cols = 0;
        stringstream stream(line);
        while(! stream.eof())
            stream >> buff[cols*rows+temp_cols++];

        if (temp_cols == 0)
            continue;

        if (cols == 0)
            cols = temp_cols;

        rows++;
    }

    infile.close();

    rows--;

    // Populate matrix with numbers.
    MatrixXf result(rows,cols);
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            result(i,j) = buff[ cols*i+j ];

    return result;
}





int main(int argc, char **argv)
{

    cbSHOT cb;

    ofstream CBshot;
    CBshot.open ("../results/32dim_SHOT_ISS.txt", ios::out | ios::app);

    /*****************************************/
    if(argc < 4)
    {
        std::cerr << "Usage:" << std::endl;
        std::cerr << argv[0] << " [-a] model.ply scene.ply Tx file " << std::endl;
        std::cerr << "\t-a\tASCII output" << std::endl;
        return (1);
    }

    Eigen::Matrix4f T;
    T = readMatrix(argv[3]);
    //cout << T << endl;

    /*******************************************/


    pcl::PolygonMesh mesh;
    pcl::io::loadPolygonFilePLY(argv[1], mesh);
    std::cerr << "Read cloud: " << std::endl;
    pcl::io::saveVTKFile ("temp_bshot_shot1.vtk", mesh);// I think that the below setfilename should be the same
    vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New ();
    reader->SetFileName ("temp_bshot_shot1.vtk");// i think that this argument should be same as above if the path is not specified...
    reader->Update ();
    vtkSmartPointer<vtkPolyData> polydata = reader->GetOutput ();
    pcl::PointCloud<pcl::PointXYZ> cloud;
    vtkPolyDataToPointCloud (polydata, cloud);
    cb.cloud1 = cloud;


    pcl::PolygonMesh mesh1;
    pcl::io::loadPolygonFilePLY(argv[2], mesh1);
    std::cerr << "Read cloud: " << std::endl;
    pcl::io::saveVTKFile ("temp_bshot_shot2.vtk", mesh1);
    vtkSmartPointer<vtkPolyDataReader> reader1 = vtkSmartPointer<vtkPolyDataReader>::New ();
    reader1->SetFileName ("temp_bshot_shot2.vtk");
    reader1->Update ();
    vtkSmartPointer<vtkPolyData> polydata1 = reader1->GetOutput ();
    pcl::PointCloud<pcl::PointXYZ> cloud1;
    vtkPolyDataToPointCloud (polydata1, cloud1);
    cb.cloud2 = cloud1;



    clock_t start_n, end_n;
    double cpu_time_used_n;
    start_n = clock();

    cb.calculate_normals (0.02);

    end_n = clock();
    cpu_time_used_n = ((double) (end_n - start_n)) / CLOCKS_PER_SEC;
    CBshot << "Time taken for creating normals   " << (double)cpu_time_used_n << std::endl;


    cb.calculate_iss_keypoints_for_3DLRF(0.001);

    std::cout << "\nNo. of keypoints : "<< cb.cloud1_keypoints.size() << endl;

    clock_t start_, end_;
    double cpu_time_used_;
    start_ = clock();

    cb.calculate_low_dimensional_SHOT(0.06);

    end_ = clock();
    cpu_time_used_ = ((double) (end_ - start_)) / CLOCKS_PER_SEC;
    CBshot << "Time taken for creating SHOT descriptors + " << (double)cpu_time_used_ << std::endl;

    cout << "Calculated SHOT Descriptors" << endl;

    /*
    for (int i =0; i<352; i++)
        cout << cb.cloud1_shot[25].descriptor[i]<<" ";
    cout << endl;
  */



    pcl::KdTreeFLANN<pcl::PointXYZ> kdtree;
    kdtree.setInputCloud(cb.cloud2_keypoints.makeShared());
    pcl::PointXYZ searchPoint;
    int actual_keypoints = 0;

    for(int i = 0; i < cb.cloud1_keypoints.size(); i++)
    {

        Eigen::Vector4f e_point1(cb.cloud1_keypoints[i].getVector4fMap());
        Eigen::Vector4f transformed_point(T*e_point1);

        //cout << T << endl;
        //cout << transformed_point << endl;

        searchPoint.x = transformed_point[0];
        searchPoint.y = transformed_point[1];
        searchPoint.z = transformed_point[2];

        int K = 1;
        std::vector<int> pointIdxNKNSearch(K);
        std::vector<float> pointNKNSquaredDistance(K);

        if ( kdtree.nearestKSearch (searchPoint, K, pointIdxNKNSearch, pointNKNSquaredDistance) > 0 )
        {
            if ( 0.02 > sqrt(pointNKNSquaredDistance[0]))
            {
                actual_keypoints++;
            }
        }


    }

    cout << "\n\nWe use the actual keypoints present in the scene rather than total number of detected keypoints \n\n";

    cout << "Actual Keypoints : " << actual_keypoints << endl;
    cout << "Total Keypoints : "<< cb.cloud1_keypoints.size()<< endl;






    clock_t start_shot1, end_shot1;
    double cpu_time_used_shot1;
    start_shot1 = clock();

    // determine correspondences
    // This uses KdTree based NN search ..so is kind of Optimal :)..we are 10 times faster :)
    pcl::registration::CorrespondenceEstimation<pcl::SHOT352, pcl::SHOT352> corr_est_shot;
    corr_est_shot.setInputSource(cb.cloud1_shot.makeShared());
    corr_est_shot.setInputTarget(cb.cloud2_shot.makeShared());
    pcl::Correspondences correspondeces_reciprocal_shot;
    //corr_est_shot.determineReciprocalCorrespondences(correspondeces_reciprocal_shot);
    corr_est_shot.determineCorrespondences(correspondeces_reciprocal_shot);

    end_shot1 = clock();
    cpu_time_used_shot1 = ((double) (end_shot1 - start_shot1)) / CLOCKS_PER_SEC;

    cout <<  "Time taken for NN of SHOT : " << (double)cpu_time_used_shot1 << "\n";
    CBshot <<  "Time taken for NN of SHOT : " << (double)cpu_time_used_shot1 << "\n";



    clock_t start_shot2, end_shot2;
    double cpu_time_used_shot2;
    start_shot2 = clock();

    pcl::CorrespondencesConstPtr correspond_shot = boost::make_shared< pcl::Correspondences >(correspondeces_reciprocal_shot);

    pcl::Correspondences corr_shot;
    pcl::registration::CorrespondenceRejectorSampleConsensus< pcl::PointXYZ > Ransac_based_Rejection_shot;
    Ransac_based_Rejection_shot.setInputSource(cb.cloud1_keypoints.makeShared());
    Ransac_based_Rejection_shot.setInputTarget(cb.cloud2_keypoints.makeShared());
    Ransac_based_Rejection_shot.setInlierThreshold(0.03);
    Ransac_based_Rejection_shot.setInputCorrespondences(correspond_shot);
    Ransac_based_Rejection_shot.getCorrespondences(corr_shot);

    end_shot2 = clock();
    cpu_time_used_shot2 = ((double) (end_shot2 - start_shot2)) / CLOCKS_PER_SEC;

    cout <<  "Time taken for RANSAC of SHOT : " << (double)cpu_time_used_shot2 << "\n";
    CBshot <<  "Time taken for RANSAC of SHOT ` " << (double)cpu_time_used_shot2 << "\n";


    cout << "No. of RANSAC matches of SHOT : " << corr_shot.size() << endl;



    int cnt=0;
    for (int i = 0; i < (int)corr_shot.size(); i++)
    {
        pcl::PointXYZ point1 = cb.cloud1_keypoints[corr_shot[i].index_query];
        pcl::PointXYZ point2 = cb.cloud2_keypoints[corr_shot[i].index_match];

        Eigen::Vector4f e_point1(point1.getVector4fMap());
        Eigen::Vector4f e_point2(point2.getVector4fMap());

        Eigen::Vector4f transformed_point(T*e_point1);
        Eigen::Vector4f diff(e_point2 - transformed_point);

        if (diff.norm() < 0.05)
            cnt++;
    }


    CBshot << "RRR of SHOT * " << ((float)cnt/(float)actual_keypoints)*100 << "\n\n";
    cout << "Groundtruth matches of SHOT * " << cnt << endl;
    cout << "RRR of SHOT * : "<< ((float)cnt/(float)actual_keypoints)*100 << endl;


    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer (new pcl::visualization::PCLVisualizer ("3D Viewer"));
    viewer->setBackgroundColor (255, 255, 255);

    pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> single_color1(cb.cloud1_keypoints.makeShared(), 255, 0, 0);
    viewer->addPointCloud<pcl::PointXYZ> (cb.cloud1_keypoints.makeShared(), single_color1, "sample cloud1");
    viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 4, "sample cloud1");
    //viewer->addCoordinateSystem (1.0);
    viewer->initCameraParameters ();

    Eigen::Matrix4f t;
    t<<1,0,0,0.75,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1;

    //cloudNext is my target cloud
    pcl::transformPointCloud(cb.cloud2_keypoints,cb.cloud2_keypoints,t);

    //int v2(1);
    //viewer->createViewPort (0.5,0.0,0.1,1.0,1);
    pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> single_color2(cb.cloud2_keypoints.makeShared(), 0, 0, 255);
    viewer->addPointCloud<pcl::PointXYZ> (cb.cloud2_keypoints.makeShared(), single_color2, "sample cloud2");
    viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 4, "sample cloud2");


    viewer->addCorrespondences<pcl::PointXYZ>(cb.cloud1_keypoints.makeShared(), cb.cloud2_keypoints.makeShared(), corr_shot, "correspondences");


// To Display Correspondences
/*
    while (!viewer->wasStopped ())
    {
        viewer->spinOnce (100);
        boost::this_thread::sleep (boost::posix_time::microseconds (100000));
    }
*/

    return 0;
}



