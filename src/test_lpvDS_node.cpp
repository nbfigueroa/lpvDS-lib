#include "ros/ros.h"

#include "lpvDS.h"
#include "utils.h"
#include "eigen3/Eigen/Dense"
#include <vector>


using namespace std;

double K;
double M;
vector<double> Priors;
vector<double> Mu;
vector<double> Sigma;
vector<double> A;
vector<double> attractor;

bool parseParams(const ros::NodeHandle& nh) {
    bool ret = true;

    if (!nh.getParam("K", K))   {
        ROS_ERROR("Couldn't retrieve the number of guassians. ");
        ret = false;
    } else {
        ROS_INFO_STREAM("Number of Components K: "<< (int)K);
    }

    if (!nh.getParam("M", M))  {
        ROS_ERROR("Couldn't retrieve dimension of state. ");
        ret = false;
    } else {
        ROS_INFO_STREAM("Dimensionality of state M: "<< (int)M);
    }

    if (!nh.getParam("Priors", Priors))   {
        ROS_ERROR("Couldn't retrieve Priors. ");
        ret = false;
    } else {
        ROS_INFO_STREAM("Priors: ");
        for (int k = 0; k< int(K); k++)
            cout << Priors.at(k) << " ";
        cout << endl;
    }

    if (!nh.getParam("Mu", Mu))   {
        ROS_ERROR("Couldn't retrieve Mu. ");
        ret = false;
    } else {
        ROS_INFO_STREAM("Mu: ");
        for (int m = 0; m< int(K)*int(M); m++)
            cout << Mu.at(m) << " ";
        cout << endl;
    }

    if (!nh.getParam("Sigma", Sigma))  {
        ROS_ERROR("Couldn't retrieve Sigma. ");
        ret = false;
    } else {
        ROS_INFO_STREAM("Sigma [0]: ");
        for (int m = 0; m< int(M)*int(M); m++)
            cout << Sigma.at(m) << " ";
        cout << endl;
    }

    if (!nh.getParam("A", A))  {
        ROS_ERROR("Couldn't retrieve A. ");
        ret = false;
    } else {
        ROS_INFO_STREAM("A [0]: ");
        for (int m = 0; m< int(M)*int(M); m++)
            cout << A.at(m) << " ";
        cout << endl;
    }

    if (!nh.getParam("attractor", attractor))  {
        ROS_ERROR("Couldn't retrieve attractor. ");
        ret = false;
    } else {
        ROS_INFO_STREAM("Priors: ");
        for (int m = 0; m< int(M); m++)
            cout << attractor.at(m) << " ";
        cout << endl;
    }

    return ret;
}


int main(int argc, char **argv)
{
    ros::init(argc, argv, "test_lpvDS_node");
    ros::NodeHandle nh;
    ros::NodeHandle _nh("~");


    if(!parseParams(_nh)) {
        ROS_ERROR("Errors while parsing arguments.");
        return 1;
    }

    /* Instantiate lpv-DS Model with parameters read from Yaml file*/
    lpvDS lpvDS_((int)K, (int)M, Priors, Mu, Sigma, A);


    /* Testing the LPV-DS on training data from MATLAB */
    string path_model  = "/home/nbfigueroa/proj/catkin_ws_lags/src/lpvDS-lib/models/CShape-bottom-pqlf/";
    string path_data   = path_model +  "Data";
    string path_xi_dot = path_model +  "xi_dot";
    fileUtils fileUtils_;
    MatrixXd Data, xi_dot;
    Data      = fileUtils_.readMatrix(path_data.c_str());
    xi_dot    = fileUtils_.readMatrix(path_xi_dot.c_str());
    int samples = Data.cols();

    cout << "Testing Accuracy of model..." << endl;

    /* Fill in attractor */
    VectorXd att; att.resize(3);
    for (int i = 0; i<(int)M; i++)
        att[i] = attractor.at(i);

    /* Fill in reference trajectories */
    MatrixXd xi_ref;  xi_ref.resize(3,samples);
    xi_ref.row(0)     = Data.row(0);
    xi_ref.row(1)     = Data.row(1);
    xi_ref.row(2)     = Data.row(2);

    /* Compute estimated velocities from model */
    VectorXd xi_ref_test;  xi_ref_test.resize(3);
    VectorXd xi_dot_test;  xi_dot_test.resize(3);
    VectorXd xi_dot_mat;   xi_dot_mat.resize(3);
    VectorXd xi_dot_error; xi_dot_error.resize(3);
    MatrixXd A_matrix; A_matrix.resize(3,3);
    VectorXd  est_error; est_error.resize(samples);
    for (int i=0; i<samples; i++){

        /* Computing desired velocity */
        xi_ref_test = xi_ref.col(i);
        A_matrix = lpvDS_.compute_A(xi_ref_test);
        xi_dot_test = A_matrix*(xi_ref_test - att);

        /* Computing error between this estimate and MATLAB */
        xi_dot_mat = xi_dot.col(i);
        xi_dot_error =  xi_dot_test-xi_dot_mat;
        est_error[i] = xi_dot_error.norm();
    }

    /* Stats on Estimation error between MATLAB-C++ model */
    cout << "Average Estimation Error" << " (Norm of predicted Matlab and C++ velocities): " << est_error.mean() << endl;


    return 0;
}