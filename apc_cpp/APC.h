#include "SuperController_Types.h"
#include <sstream>
#include <iostream>
#include "hdf5.h"
#include <string>
#include <mpi.h>
#include <map>
#include <fstream>
#include <unistd.h>
#include <iomanip>
#include <sys/stat.h>
#include <stdio.h>
#include <math.h>
#include <vector>   // <<---- ADD THIS
//#include "libstateupdater.h"
//#include "libstateupdater_baseline.h"
#define GetCurrentDir getcwd

class SuperController {

 private:

//------APC() related members---------
  int    firstAPCAction;
  double    next_lut;
  double average_tso;
  int    time_table_idx;
  double  coeffLine;
  double  demanded_farm_P;
  double  delta_P_ref;
  double* sum_error_CLD;
  double* carico;
  double* apc_time;
  double* apc_power;
  double Kp_CLD;
  double Ki_CLD;
  double Kp_APC;
  double Ki_APC;
  double alfa_1;
  double alfa_2;
  double alfa_3;
  double* alpha_coeff;
  double* old_alpha_coeff;
  double* lock_power;
  double  sum_error_APC;
  double*  old_power_demand;
  double previous_apc_error;
  int* isSaturated;
  int* isPower;
  int* contatore;
  int* contatore_P_demand;
  double* P_demand_medio;
  double* P_demand;
//------APC() related members---------
  int    middle;
  double* power_lut;
  double* derate0;
  double* derate1;
  double* derate2;
  double* alfa;
  double* yaw_APC;
  double* yaw0_APC;
  double* yaw1_APC;
  double* yaw2_APC;
  int	start;
  int	finish;
  double ang_coeff;
  double quota;
  int differenza;
  int indice;
//---------------------------------------------------
 
 public:  
//--------------constructor and destructor---------------------------------------------
  SuperController(int n, int numScInputs, int numScOutputs, int worldMPIRank);
  ~SuperController() ;

//--------------Active power controller related functions---------------------------------
  void init_opendLoop();
  void init_loadBalance_apc();
  void init_closedLoop();
  void initmaxReserve_apc();
  void loadBalance_apc(int Rank, MPI_Comm fastMPIComm);
  void opendLoop_apc(int Rank, MPI_Comm fastMPIComm);
  void closedLoop_apc(int Rank, MPI_Comm fastMPIComm);
  void maxReserve_apc(int Rank, MPI_Comm fastMPIComm);
};
