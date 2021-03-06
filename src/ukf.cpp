#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 */
 MeasurementPackage meas_package;

UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5); 

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 2;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 0.3;

  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;

  count = 1;



  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.

  Hint: one or more values initialized above might be wildly off...

  */
    n_x_ = 5;
    n_aug_ = 7;
    lambda_ = 3 - n_x_;


    weights_ = VectorXd(2*n_aug_+1);
    Xsig_pred_ = MatrixXd(n_x_, n_aug_*2 +1);
    
    is_initialized_ = false;
    time_us_ = 0.0;

    //Until here the constructor

    

  /*
  time_us_ = meas_package.timestamp_;
  is_initialized_ = true;
  Xsig_aug = MatrixXd(n_x_, n_aug_*2+1);
  */

  }


UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */

void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */
    // skip predict/update if sensor type is ignored
  if ((meas_package.sensor_type_ == MeasurementPackage::RADAR && use_radar_) ||
      (meas_package.sensor_type_ == MeasurementPackage::LASER && use_laser_)) {
  if(!is_initialized_){

    //first measurement
    x_<<1, 1, 1, 1, 0.1;

    //init covariance matrix
    P_<< 0.15, 0, 0, 0, 0,
             0, 0.15, 0, 0, 0,
             0, 0, 1, 0, 0,
             0, 0, 0, 1, 0,
             0, 0, 0, 0, 1;
  

  time_us_ = meas_package.timestamp_;

  if(use_laser_ && meas_package.sensor_type_ == MeasurementPackage::LASER)
  {
  x_(0) = meas_package.raw_measurements_(0);
  x_(1) = meas_package.raw_measurements_(1);

  }

  else if(use_radar_ = true && meas_package.sensor_type_ == MeasurementPackage::RADAR)
  {
   float rho = meas_package.raw_measurements_(0);
   float phi = meas_package.raw_measurements_(1);
   float rho_d = meas_package.raw_measurements_(2);

   x_(0) = rho * cos(phi);
   x_(1) = rho * sin(phi);
   //We just need to initialize the state
  // x_[2] = rho_d;
  // x_[3] = phi;
  // x_[4] = 0;
  }

  is_initialized_= true;

  return;
  }

//Bis hierher
 


/****************************************
Prediction
***************************************/
 float dt = (meas_package.timestamp_ - time_us_)/1000000.0; //I want it expressed in seconds
time_us_ = meas_package.timestamp_;



Prediction(dt);

/************************************************
Update
*************************************************/

if(meas_package.sensor_type_ == MeasurementPackage::LASER && use_laser_){
  UpdateLidar(meas_package);
}
else if(meas_package.sensor_type_ == MeasurementPackage::RADAR && use_radar_){
  UpdateRadar(meas_package);
}

}
}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */

void UKF::Prediction(double delta_t) {
  /**
  TODO:

  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */
  
/****************************
 Generate Sigma Points
 ****************************/

  MatrixXd Xsig = MatrixXd(n_x_, 2*n_x_ +1);
  MatrixXd A = P_.llt().matrixL();

  Xsig.col(0) = x_;
  for(int i=0; i<n_x_; i++) //I forgot that it isnt (n_x *2 +1)
  {
    Xsig.col(i+1) = x_ + sqrt(lambda_ + n_x_)*A.col(i);
    Xsig.col(1+i+n_x_) = x_ - sqrt(lambda_ + n_x_)*A.col(i);  
  }

/********************************************
Augment Sigma Points
********************************************/

  VectorXd x_aug = VectorXd(n_aug_);
  MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);
  MatrixXd Xsig_aug= MatrixXd(n_aug_, n_aug_*2+1);

  x_aug.head(5) = x_;
  x_aug(5) = 0;
  x_aug(6) = 0;

  P_aug.fill(0.0);
  P_aug.topLeftCorner(5,5) = P_;
  P_aug(5, 5) = std_a_*std_a_;
  P_aug(6, 6) = std_yawdd_*std_yawdd_;

  MatrixXd L = P_aug.llt().matrixL();
  
  Xsig_aug.col(0) = x_aug;
  for(int i=0; i<n_aug_; i++)
  {
    Xsig_aug.col(i+1) = x_aug + sqrt(lambda_ + n_aug_)*L.col(i);
    Xsig_aug.col(i+1+n_aug_) = x_aug - sqrt(lambda_ + n_aug_)*L.col(i);
  }


  //Predict sigma points
  for(int i = 0; i<2*n_aug_+1; i++)
  {
  
    //Extract values for better readability
    double p_x = Xsig_aug(0,i);
    double p_y = Xsig_aug(1,i);
    double v = Xsig_aug(2,i);
    double yaw = Xsig_aug(3,i);
    double yawd = Xsig_aug(4,i);
    double nu_a = Xsig_aug(5,i);
    double nu_yawdd = Xsig_aug(6,i);

    //Predicted state values
    double px_p, py_p; 

    //Avoid division by zero
    if(fabs(yawd) > 0.001)
    {
      px_p = p_x + v/yawd * (sin(yaw + yawd*delta_t) - sin(yaw));
      py_p = p_y + v/yawd * (cos(yaw)- cos(yaw + yawd * delta_t));
    }
    else{
      px_p = p_x + v*delta_t*cos(yaw);
      py_p = p_y + v*delta_t*sin(yaw);
    }
    double v_p = v;
    double yaw_p = yaw + yawd*delta_t;
    double yawd_p = yawd;

    //Add noise 
    px_p = px_p + 0.5*delta_t*delta_t*cos(yaw)*nu_a;
    py_p = py_p + 0.5*delta_t*delta_t*sin(yaw)*nu_a;
    v_p = v_p + nu_a*delta_t;

    yaw_p = yaw_p + 0.5*(delta_t*delta_t)*nu_yawdd;
    yawd_p = yawd_p + delta_t*nu_yawdd;

    //Write predicted sigma points into the right column
    Xsig_pred_(0,i) = px_p;
    Xsig_pred_(1,i) = py_p;
    Xsig_pred_(2,i) = v_p;
    Xsig_pred_(3,i) = yaw_p;
    Xsig_pred_(4,i) = yawd_p;

    cout<<"This is Xsig_pred_: "<<Xsig_pred_<<endl;
  }
  x_.fill(0.0);
  for(int i=0; i<2*n_aug_+1; i++)
  {
    if(i==0){
      double weights_0 = lambda_/(lambda_ + n_aug_);
      weights_(i) = weights_0;
    }
    else{
      double weights = 0.5/(lambda_ + n_aug_);
      weights_(i) = weights;
    }

    x_ = x_ + weights_(i)*Xsig_pred_.col(i);
  }
    P_.fill(0.0);
    for(int i=0; i<2*n_aug_+1; i++)
    {
      //State difference
      VectorXd x_diff = Xsig_pred_.col(i) - x_;

      //Angle normalization
      while(x_diff(3) > M_PI) x_diff(3)-= 2.*M_PI;
      while(x_diff(3) < -M_PI) x_diff(3)+= 2.*M_PI;

      P_ = P_ + weights_(i)*x_diff*x_diff.transpose();

    }
}

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */
      MatrixXd Xsig = MatrixXd(2, 2*n_aug_+1);
      MatrixXd S = MatrixXd(2, 2);
      VectorXd x = meas_package.raw_measurements_;


      for(int i=0; i<2*n_aug_ +1; i++)
      {
        double p_x = Xsig_pred_(0,i);
        double p_y = Xsig_pred_(1,i);
        //I dont need the rest of them
        //double v = Xsig_pred_(2,i);

        //double yaw = Xsig_pred_(3,i);
        //double v1 = cos(yaw)*v;
        //double v2 = sin(yaw)*v;

        //Measurement model, I'm going to fill up only the two first values.
        Xsig(0,i) = p_x;
        Xsig(1,i) = p_y;
      }

  //I should change this name by "z_pred"
  VectorXd x_pred = VectorXd(2);
  x_pred.fill(0.0);
  for(int i=0; i<2*n_aug_+1; i++)
  {
    x_pred = x_pred + weights_(i)*Xsig.col(i); 
  }
      //S is for the predicted Measure Covariance
      S.fill(0.0);
      for(int i=0; i<2*n_aug_+1; i++)
      {
        //residual
        VectorXd x_diff = Xsig.col(i) - x_pred;

        S = S + weights_(i) * x_diff *x_diff.transpose();
      }


      //Add measurement noise covariance matrix
      MatrixXd R = MatrixXd(2, 2);
      R << std_laspx_*std_laspx_, 0,
            0, std_laspy_*std_laspy_;
      S = S + R;

      MatrixXd Tc = MatrixXd(n_x_, 2);

      /***************
      Update for Lidar
      ***************/

      Tc.fill(0.0);

      for(int i=0; i<2*n_aug_+1; i++){
      //residual
      VectorXd x_diff = Xsig.col(i) - x_pred;
      //Change this name by x_diff
      VectorXd xx_diff = Xsig_pred_.col(i) - x_;


      //xx_diff is the one that comes from outsie
      Tc = Tc + weights_(i)*xx_diff * x_diff.transpose();
}
      //Kalman gain K
      MatrixXd K = Tc * S.inverse();

      //residual
      VectorXd x_diff = x - x_pred;  //x are the measurements incoming, bear in mind that x_pred is a vector with size(2)

      //Update state mean and covariance matrix
      x_ = x_ + K * x_diff; //Multiplico la ganancia por el valor residual
      P_ = P_ - K*S*K.transpose(); //Now I have updated state mean and covariance.
}

      
/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */

    //Predict Radar Measurement
    //Transform Sigma Points into Measurement Space

      int n_z = 3;
      MatrixXd Zsig = MatrixXd(n_z, 2*n_aug_+1);
      MatrixXd S = MatrixXd(n_z, n_z);
      VectorXd z = meas_package.raw_measurements_;

      for(int i=0; i<2*n_aug_ +1; i++)
      {
        double p_x = Xsig_pred_(0,i);
        double p_y = Xsig_pred_(1,i);
        double v = Xsig_pred_(2,i);
        double yaw = Xsig_pred_(3,i);
        double v1 = cos(yaw)*v;
        double v2 = sin(yaw)*v;

        //Measurement model
        Zsig(0,i) = sqrt(p_x*p_x + p_y*p_y);
        Zsig(1,i) = atan2(p_y, p_x);
        Zsig(2,i) = (p_x*v1 +p_y*v2)/ sqrt(p_x*p_x + p_y*p_y);
      }
      
      //mean predicted measurement
      VectorXd z_pred = VectorXd(n_z);
      z_pred.fill(0.0);

      for(int i=0; i<2*n_aug_+1; i++)
      {
        z_pred = z_pred + weights_(i)*Zsig.col(i);
      }

      
      S.fill(0.0);
      for(int i=0; i<2*n_aug_+1; i++)
      {
        //residual
        VectorXd z_diff = Zsig.col(i) - z_pred;
        //Angle normalization
        while(z_diff(1)>M_PI) z_diff(1) -=2.*M_PI;
        while(z_diff(1)<-M_PI) z_diff(1) +=2.*M_PI;

        S = S + weights_(i) * z_diff *z_diff.transpose();
      }
      //Add measurement noise covariance matrix
      MatrixXd R = MatrixXd(n_z, n_z);
      R << std_radr_*std_radr_, 0, 0,
            0, std_radphi_*std_radphi_, 0,
            0, 0, std_radrd_*std_radrd_;
      S = S + R;

      MatrixXd Tc = MatrixXd(n_x_, n_z);
      Tc.fill(0.0);

      for(int i=0; i< 2*n_aug_+1; i++)
      {
      //Residual
      VectorXd z_diff = Zsig.col(i) - z_pred;
      //Angle normalization
      while(z_diff(1) > M_PI) z_diff(1) -= 2.*M_PI;
      while(z_diff(1) <- M_PI) z_diff(1) += 2.*M_PI;

      VectorXd x_diff = Xsig_pred_.col(i) - x_;

      while(x_diff(3) > M_PI) x_diff(3) -= 2.*M_PI;
      while(x_diff(3) < -M_PI) x_diff(3) += 2.*M_PI;

      Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
    }
      //Kalman gain
      MatrixXd K = Tc* S.inverse();

      //residual
      VectorXd z_diff = z - z_pred; //z are the measurements incoming

      //Angle normalization
      while(z_diff(1) > M_PI) z_diff(1) -= 2.*M_PI;
      while(z_diff(1) <- M_PI) z_diff(1) += 2.*M_PI;
      
      //Update State Mean and Covariance Matrix
      x_ = x_ + K * z_diff;
      P_ = P_ - K*S*K.transpose();
      
    }
