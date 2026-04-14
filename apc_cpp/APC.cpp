#include "SC.h"

void SuperController::init_opendLoop()  // Reads input reference power to track and stores time and W in power_ref array
{

	std::ifstream APCInputFile2(currentPath + "Pref.in"); //open your file
	std::string line;
	int number_of_lines = 0;
	while (std::getline(APCInputFile2, line)) // get length of TSO file
	{
        ++number_of_lines;
	}
	APCInputFile2.close();

        apc_time = new double[number_of_lines];
        apc_power = new double[number_of_lines];

	std::ifstream APCInputFile(currentPath + "Pref.in");

	if(!APCInputFile.is_open())
	{
		std::cout << "File not opened!" << std::endl;
		std::abort();
	}
		//read the file (two columns) and store data in two arrays > tso power and tso time
	for(int i=0; !APCInputFile.eof(); i++)
	{
		APCInputFile >> apc_time[i];
		APCInputFile >> apc_power[i];
	}
	
	APCInputFile.close();	

	// Determine initial power demand of the wind farm	
	float targetT = globStates[0]; //timestep of the simulation!
	// float coeffLine; // angular coefficient for 1D interpolation!
	int i, j, mid; // parameters of search algorithm
	// float demanded_farm_P; // Power demanded by TSO
	// Find where we are in the simulation relatively to TSOs request of power! Binary search...
	
	std::cout << " Binary search starting ... " << std::endl;

	if (targetT <= apc_time[0]){
		std::cout << " check 1" << std::endl;
		demanded_farm_P = apc_power[0];
		int time_table_Pref = 0;	
	}
	else if (targetT >= apc_time[number_of_lines-1]){
		std::cout << " check 2" << std::endl;
		demanded_farm_P = apc_power[number_of_lines-1];	
		int time_table_Pref = number_of_lines-1;	
	}
	else
		std::cout << " check 3" << std::endl;
		i = 0, j = number_of_lines, mid = 0;
		while (i < j) {
			mid = (i + j) / 2;
		 
			if (apc_time[mid] == targetT)
			    demanded_farm_P = apc_power[mid];
		 
			/* If target is less than array element,
			    then search in left */
			if (targetT < apc_time[mid]) {
		 
			    // If target is greater than previous
			    // to mid, return closest of two
			    if (mid > 0 && targetT > apc_time[mid - 1])
				coeffLine = (apc_power[mid]-apc_power[mid-1])/(apc_time[mid]-apc_time[mid-1]); // angular coefficient to interpolate power at timestep from lookup ref TSO signal
				demanded_farm_P = apc_power[mid] + coeffLine*(targetT-apc_time[mid]); // 1D interpolation
		 
			    /* Repeat for left half */
			    j = mid;
			}
		 
			// If target is greater than mid
			else {
			    if (mid < number_of_lines - 1 && targetT < apc_time[mid + 1])
				coeffLine = (apc_power[mid]-apc_power[mid-1])/(apc_time[mid]-apc_time[mid-1]); // angular coefficient to interpolate power at timestep from lookup ref TSO signal
				demanded_farm_P = apc_power[mid] + coeffLine*(targetT-apc_time[mid]); // 1D interpolation
		 
		    // update i
		    i = mid + 1;
		}
	}	
	
	
	time_table_idx = mid;	

	std::string APCgainsInputFile = currentPath + "opendLoop_open_loop.in";

	readVarArray(APCgainsInputFile,std::string("alfa_1"),&alfa_1,1);
	readVarArray(APCgainsInputFile,std::string("alfa_2"),&alfa_2,1);
	readVarArray(APCgainsInputFile,std::string("alfa_3"),&alfa_3,1);

	return;
}

void SuperController::init_closedLoop()  // Reads input reference power to track and stores time and W in power_ref array
{

	std::ifstream APCInputFile2(currentPath + "Pref.in"); //open your file
	std::string line;
	int number_of_lines = 0;
	while (std::getline(APCInputFile2, line)) // get length of TSO file
	{
        ++number_of_lines;
	}
	APCInputFile2.close();

        apc_time = new double[number_of_lines];
        apc_power = new double[number_of_lines];

	std::ifstream APCInputFile(currentPath + "Pref.in");

	if(!APCInputFile.is_open())
	{
		std::cout << "File not opened!" << std::endl;
		std::abort();
	}
		//read the file (two columns) and store data in two arrays > tso power and tso time
	for(int i=0; !APCInputFile.eof(); i++)
	{
		APCInputFile >> apc_time[i];
		APCInputFile >> apc_power[i];
	}
	
	APCInputFile.close();	

	// Determine initial power demand of the wind farm	
	float targetT = globStates[0]; //timestep of the simulation!
	// float coeffLine; // angular coefficient for 1D interpolation!
	int i, j, mid; // parameters of search algorithm
	// float demanded_farm_P; // Power demanded by TSO
	// Find where we are in the simulation relatively to TSOs request of power! Binary search...
	
	std::cout << " Binary search starting ... " << std::endl;

	if (targetT <= apc_time[0]){
		std::cout << " check 1" << std::endl;
		demanded_farm_P = apc_power[0];
		int time_table_Pref = 0;	
	}
	else if (targetT >= apc_time[number_of_lines-1]){
		std::cout << " check 2" << std::endl;
		demanded_farm_P = apc_power[number_of_lines-1];	
		int time_table_Pref = number_of_lines-1;	
	}
	else
		std::cout << " check 3" << std::endl;
		i = 0, j = number_of_lines, mid = 0;
		while (i < j) {
			mid = (i + j) / 2;
		 
			if (apc_time[mid] == targetT)
			    demanded_farm_P = apc_power[mid];
		 
			/* If target is less than array element,
			    then search in left */
			if (targetT < apc_time[mid]) {
		 
			    // If target is greater than previous
			    // to mid, return closest of two
			    if (mid > 0 && targetT > apc_time[mid - 1])
				coeffLine = (apc_power[mid]-apc_power[mid-1])/(apc_time[mid]-apc_time[mid-1]); // angular coefficient to interpolate power at timestep from lookup ref TSO signal
				demanded_farm_P = apc_power[mid] + coeffLine*(targetT-apc_time[mid]); // 1D interpolation
		 
			    /* Repeat for left half */
			    j = mid;
			}
		 
			// If target is greater than mid
			else {
			    if (mid < number_of_lines - 1 && targetT < apc_time[mid + 1])
				coeffLine = (apc_power[mid]-apc_power[mid-1])/(apc_time[mid]-apc_time[mid-1]); // angular coefficient to interpolate power at timestep from lookup ref TSO signal
				demanded_farm_P = apc_power[mid] + coeffLine*(targetT-apc_time[mid]); // 1D interpolation
		 
		    // update i
		    i = mid + 1;
		}
	}	
		
	time_table_idx = mid;	

	alpha_coeff   = new double[nTurbines];
	isSaturated   = new int[nTurbines];
	old_power_demand  = new double[nTurbines];
	old_alpha_coeff   = new double[nTurbines];

	previous_apc_error = 1.0;
	sum_error_APC  = 0.0;
	delta_P_ref    = 0.0;
	firstAPCAction = 1;

	std::string APCgainsInputFile = currentPath + "APC_gains.in";

	readVarArray(APCgainsInputFile,std::string("Kp_APC"),&Kp_APC,1);
	readVarArray(APCgainsInputFile,std::string("Ki_APC"),&Ki_APC,1);

	readVarArray(APCgainsInputFile,std::string("alfa_1"),&alfa_1,1);
	readVarArray(APCgainsInputFile,std::string("alfa_2"),&alfa_2,1);
	readVarArray(APCgainsInputFile,std::string("alfa_3"),&alfa_3,1);

	old_alpha_coeff[0] = alfa_1;
	old_alpha_coeff[1] = alfa_2;
	old_alpha_coeff[2] = alfa_3;

	old_power_demand[0] = alfa_1*demanded_farm_P;
	old_power_demand[1] = alfa_2*demanded_farm_P;	
	old_power_demand[2] = alfa_3*demanded_farm_P;

	return;
}

void SuperController::init_loadBalance_apc()  // Reads input reference power to track and stores time and W in power_ref array
{

	std::ifstream APCInputFile2(currentPath + "Pref.in"); //open your file
	std::string line;
	int number_of_lines = 0;
	while (std::getline(APCInputFile2, line)) // get length of TSO file
	{
        ++number_of_lines;
	}
	APCInputFile2.close();

        apc_time = new double[number_of_lines];
        apc_power = new double[number_of_lines];

	std::ifstream APCInputFile(currentPath + "Pref.in");

	if(!APCInputFile.is_open())
	{
		std::cout << "File not opened!" << std::endl;
		std::abort();
	}
		//read the file (two columns) and store data in two arrays > tso power and tso time
	for(int i=0; !APCInputFile.eof(); i++)
	{
		APCInputFile >> apc_time[i];
		APCInputFile >> apc_power[i];
	}
	
	APCInputFile.close();	
 
	//Display the array
/*	for(int i=0; i<number_of_lines; i++){
			std::cout << apc_time[i] << ", " << apc_power[i] << '\n';
	}
*/

	// Determine initial power demand of the wind farm	
	float targetT = globStates[0]; //timestep of the simulation!
	// float coeffLine; // angular coefficient for 1D interpolation!
	int i, j, mid; // parameters of search algorithm
	// float demanded_farm_P; // Power demanded by TSO
	// Find where we are in the simulation relatively to TSOs request of power! Binary search...
	
	std::cout << " Binary search starting ... " << std::endl;

	if (targetT <= apc_time[0]){
		std::cout << " check 1" << std::endl;
		demanded_farm_P = apc_power[0];
		int time_table_Pref = 0;	
	}
	else if (targetT >= apc_time[number_of_lines-1]){
		std::cout << " check 2" << std::endl;
		demanded_farm_P = apc_power[number_of_lines-1];	
		int time_table_Pref = number_of_lines-1;	
	}
	else
		std::cout << " check 3" << std::endl;
		i = 0, j = number_of_lines, mid = 0;
		while (i < j) {
			mid = (i + j) / 2;
		 
			if (apc_time[mid] == targetT)
			    demanded_farm_P = apc_power[mid];
		 
			/* If target is less than array element,
			    then search in left */
			if (targetT < apc_time[mid]) {
		 
			    // If target is greater than previous
			    // to mid, return closest of two
			    if (mid > 0 && targetT > apc_time[mid - 1])
				coeffLine = (apc_power[mid]-apc_power[mid-1])/(apc_time[mid]-apc_time[mid-1]); // angular coefficient to interpolate power at timestep from lookup ref TSO signal
				demanded_farm_P = apc_power[mid] + coeffLine*(targetT-apc_time[mid]); // 1D interpolation
		 
			    /* Repeat for left half */
			    j = mid;
			}
		 
			// If target is greater than mid
			else {
			    if (mid < number_of_lines - 1 && targetT < apc_time[mid + 1])
				coeffLine = (apc_power[mid]-apc_power[mid-1])/(apc_time[mid]-apc_time[mid-1]); // angular coefficient to interpolate power at timestep from lookup ref TSO signal
				demanded_farm_P = apc_power[mid] + coeffLine*(targetT-apc_time[mid]); // 1D interpolation
		 
		    // update i
		    i = mid + 1;
		}
	}	
	
	//std::cout << "Power demanded by TSO = " << demanded_farm_P << " W !!!" << std::endl;
	
	time_table_idx = mid;	

	//std::cout << "Time demanded by TSO = " << apc_time[mid] << " s !!!" << std::endl;
	//std::cout << "Time index by TSO = " << time_table_idx << std::endl;

	sum_error_CLD     = new double[nTurbines];	
	carico            = new double[nTurbines];
	alpha_coeff       = new double[nTurbines];
	old_power_demand  = new double[nTurbines];
	old_alpha_coeff   = new double[nTurbines];
	isSaturated       = new int[nTurbines];

	previous_apc_error = 1.0;
	sum_error_APC      = 0.0;
	delta_P_ref    = 0.0;
	firstAPCAction = 1;

	std::string APCgainsInputFile = currentPath + "APC_gains.in";
	//  if(strategy.compare("APC")!=0)
	//  {

	readVarArray(APCgainsInputFile,std::string("Kp_CLD"),&Kp_CLD,1);
	readVarArray(APCgainsInputFile,std::string("Ki_CLD"),&Ki_CLD,1);
	readVarArray(APCgainsInputFile,std::string("Kp_APC"),&Kp_APC,1);
	readVarArray(APCgainsInputFile,std::string("Ki_APC"),&Ki_APC,1);
	//readVarArray(APCgainsInputFile,std::string("Kp_SAT"),&Kp_SAT,1);

	readVarArray(APCgainsInputFile,std::string("alfa_1"),&alfa_1,1);
	readVarArray(APCgainsInputFile,std::string("alfa_2"),&alfa_2,1);
	readVarArray(APCgainsInputFile,std::string("alfa_3"),&alfa_3,1);

	sum_error_CLD[0] = alfa_1;
	sum_error_CLD[1] = alfa_2;
	sum_error_CLD[2] = alfa_3;
	old_alpha_coeff[0] = alfa_1;
	old_alpha_coeff[1] = alfa_2;
	old_alpha_coeff[2] = alfa_3;
	alpha_coeff[0] = alfa_1;
	alpha_coeff[1] = alfa_2;
	alpha_coeff[2] = alfa_3;
	old_power_demand[0] = alfa_1*demanded_farm_P;
	old_power_demand[1] = alfa_2*demanded_farm_P;	
	old_power_demand[2] = alfa_3*demanded_farm_P;

	return;
}


void SuperController::initmaxReserve_apc()  // Reads input reference power to track and stores time and W in power_ref array
{

	std::cout << " Check out Pref.in ... " << std::endl;

	std::ifstream APCInputFile2(currentPath + "Pref.in"); //open your file
	std::string line;
	int number_of_lines = 0;
	while (std::getline(APCInputFile2, line)) // get length of TSO file
	{
        ++number_of_lines;
	}
	APCInputFile2.close();

        apc_time = new double[number_of_lines];
        apc_power = new double[number_of_lines];

	std::ifstream APCInputFile(currentPath + "Pref.in");

	if(!APCInputFile.is_open())
	{
		std::cout << "File not opened!" << std::endl;
		std::abort();
	}
		//read the file (two columns) and store data in two arrays > tso power and tso time
	for(int i=0; !APCInputFile.eof(); i++)
	{
		APCInputFile >> apc_time[i];
		APCInputFile >> apc_power[i];
	}
	
	APCInputFile.close();	

	std::cout << " DONE! Check out Pref.in ... " << std::endl;

////////////////////////////////////////////////////////////////////////////////////////////

	std::cout << " Now, check out alfa.in ... " << std::endl;

	std::ifstream APCInputFile3(currentPath + "alfa.in"); //open your file
	std::string line2;
	int number_of_lines2 = 0;
	while (std::getline(APCInputFile3, line2)) // get length of TSO file
	{
        ++number_of_lines2;
	}
	APCInputFile3.close();

	const int nturbi = 3;
        power_lut = new double[number_of_lines2];
        derate0 = new double[number_of_lines2]; // !!!!!!!!!!!!!!!!!!!!!DANGER--DANGER--DANGER!!!!!!!!!!!!!!!!!!!!! CAREFUL HARDCODED NTURBINES
	derate1 = new double[number_of_lines2]; 
	derate2 = new double[number_of_lines2]; 

	std::ifstream APCInputFile4(currentPath + "alfa.in");

	if(!APCInputFile4.is_open())
	{
		std::cout << "File derate not opened!" << std::endl;
		std::abort();
	}
		//read the file (two columns) and store data in two arrays > tso power and tso time
	for(int i=0; !APCInputFile4.eof(); i++)
	{
		APCInputFile4 >> power_lut[i];
		APCInputFile4 >> derate0[i];
		APCInputFile4 >> derate1[i];
		APCInputFile4 >> derate2[i];
	}
	
	APCInputFile4.close();	
	
	std::cout << " DONE!!! check out derate.in ... " << std::endl;
// ################################################################################################################## 
        std::cout << " Now, check out yaw.in ... " << std::endl;

        std::ifstream APCInputFile5(currentPath + "yaw.in"); //open your file
        std::string line3;
        int number_of_lines3 = 0;
        while (std::getline(APCInputFile5, line3)) // get length of TSO file
        {
        ++number_of_lines3;
        }
        APCInputFile5.close();

        //const int nturbi = 3;
        //power_lut = new double[number_of_lines2];
        yaw0_APC = new double[number_of_lines3]; // !!!!!!!!!!!!!!!!!!!!!DANGER--DANGER--DANGER!!!!!!!!!!!!!!!!!!!!! CAREFUL HARDCODED NTURBINES
        yaw1_APC = new double[number_of_lines3];
	yaw2_APC = new double[number_of_lines3];
	double p_lut [number_of_lines3];

        //yaw2 = new double[number_of_lines2];

        std::ifstream APCInputFile6(currentPath + "yaw.in");

        if(!APCInputFile6.is_open())
        {
                std::cout << "File derate not opened!" << std::endl;
                std::abort();
        }
                //read the file (two columns) and store data in two arrays > tso power and tso time
        for(int i=0; !APCInputFile6.eof(); i++)
        {
                //APCInputFile4 >> power_lut[i];
	        APCInputFile6 >> p_lut[i];
                APCInputFile6 >> yaw0_APC[i];
                APCInputFile6 >> yaw1_APC[i];
                APCInputFile6 >> yaw2_APC[i];

		//APCInputFile6 >> derate2[i];
        }

        APCInputFile6.close();

	// Determine initial power demand of the wind farm	
	float targetT = globStates[0]; //timestep of the simulation!
	// float coeffLine; // angular coefficient for 1D interpolation!
	int i, j, mid; // parameters of search algorithm
	// float demanded_farm_P; // Power demanded by TSO
	// Find where we are in the simulation relatively to TSOs request of power! Binary search...
	
	std::cout << " Binary search starting ... " << std::endl;

	if (targetT <= apc_time[0]){
		std::cout << " check 1" << std::endl;
		demanded_farm_P = apc_power[0];
		int time_table_Pref = 0;	
	}
	else if (targetT >= apc_time[number_of_lines-1]){
		std::cout << " check 2" << std::endl;
		demanded_farm_P = apc_power[number_of_lines-1];	
		int time_table_Pref = number_of_lines-1;	
	}
	else
		std::cout << " check 3" << std::endl;
		i = 0, j = number_of_lines, mid = 0;
		while (i < j) {
			mid = (i + j) / 2;
		 
			if (apc_time[mid] == targetT)
			    demanded_farm_P = apc_power[mid];
		 
			/* If target is less than array element,
			    then search in left */
			if (targetT < apc_time[mid]) {
		 
			    // If target is greater than previous
			    // to mid, return closest of two
			    if (mid > 0 && targetT > apc_time[mid - 1])
				coeffLine = (apc_power[mid]-apc_power[mid-1])/(apc_time[mid]-apc_time[mid-1]); // angular coefficient to interpolate power at timestep from lookup ref TSO signal
				demanded_farm_P = apc_power[mid] + coeffLine*(targetT-apc_time[mid]); // 1D interpolation
		 
			    /* Repeat for left half */
			    j = mid;
			}
		 
			// If target is greater than mid
			else {
			    if (mid < number_of_lines - 1 && targetT < apc_time[mid + 1])
				coeffLine = (apc_power[mid]-apc_power[mid-1])/(apc_time[mid]-apc_time[mid-1]); // angular coefficient to interpolate power at timestep from lookup ref TSO signal
				demanded_farm_P = apc_power[mid] + coeffLine*(targetT-apc_time[mid]); // 1D interpolation
		 
		    // update i
		    i = mid + 1;
		}
	}	
	
	//std::cout << "Power demanded by TSO = " << demanded_farm_P << " W !!!" << std::endl;
	
	time_table_idx = mid;	

	//std::cout << "Time demanded by TSO = " << apc_time[mid] << " s !!!" << std::endl;
	//std::cout << "Time index by TSO = " << time_table_idx << std::endl;

	sum_error_CLD = new double[nTurbines];	
	alpha_coeff   = new double[nTurbines];
	isSaturated   = new int[nTurbines];
	isPower       = new int[nTurbines];
	//contatore     = new int[nTurbines];

	for (int i=0;i<nTurbines;i++)
	{	
		sum_error_CLD[i] = 0;
	}

	sum_error_APC  = 0.0;
	delta_P_ref    = 0.0;
	firstAPCAction = 1;

	std::string APCgainsInputFile = currentPath + "APC_gains.in";
	//  if(strategy.compare("APC")!=0)
	//  {

	readVarArray(APCgainsInputFile,std::string("Kp_CLD"),&Kp_CLD,1);
	readVarArray(APCgainsInputFile,std::string("Ki_CLD"),&Ki_CLD,1);
	readVarArray(APCgainsInputFile,std::string("Kp_APC"),&Kp_APC,1);
	readVarArray(APCgainsInputFile,std::string("Ki_APC"),&Ki_APC,1);
	//readVarArray(APCgainsInputFile,std::string("Kp_SAT"),&Kp_SAT,1);


	middle = 10;
	average_tso = 0;
	next_lut = targetT;
   	alfa = new double[nTurbines];
	yaw_APC = new double[nTurbines-1];
		
	old_power_demand  = new double[nTurbines];
	old_alpha_coeff   = new double[nTurbines];
	//std::string APCgainsInputFile = currentPath + "APC_gains.in";
	//readVarArray(APCgainsInputFile,std::string("Kp_APC"),&Kp_APC,1);
	//readVarArray(APCgainsInputFile,std::string("Ki_APC"),&Ki_APC,1);

	//std::cout << "Kp_APC = " << Kp_APC << std::endl;
	old_alpha_coeff[0] = 0.4;
	old_alpha_coeff[1] = 0.25;
	old_alpha_coeff[2] = 0.35;
	old_power_demand[0] = 3370000;
	old_power_demand[1] = 3370000;	
	old_power_demand[2] = 3370000;
	
	return;
}


void SuperController::opendLoop_apc(int Rank, MPI_Comm fastMPIComm)
{
	if(Rank!=0)
	{
     	   return;
        }
	float time = globStates[0];
	float dt = globStates[2];
	if (time < apc_time[time_table_idx]) // if cfd timestep is below idx of the TSO table, keep interpolating there, otherwise, move to the next (TSOs) timestep!!! 	
	{
		coeffLine = (apc_power[time_table_idx]-apc_power[time_table_idx-1])/(apc_time[time_table_idx]-apc_time[time_table_idx-1]); // angular coefficient to interpolate power at timestep from lookup ref TSO signal
		demanded_farm_P = apc_power[time_table_idx] + coeffLine*(time-apc_time[time_table_idx]); // 1D interpolation
	}	
	else
	{
		coeffLine = (apc_power[time_table_idx+1]-apc_power[time_table_idx])/(apc_time[time_table_idx+1]-apc_time[time_table_idx]); 
		demanded_farm_P = apc_power[time_table_idx+1] + coeffLine*(time-apc_time[time_table_idx+1]); // 1D interpolation
		time_table_idx = time_table_idx + 1;
	}

	avrSWAP_S[0][5] = alfa_1 * (demanded_farm_P);
	avrSWAP_S[1][5] = alfa_2 * (demanded_farm_P);
	avrSWAP_S[2][5] = alfa_3 * (demanded_farm_P);

	for (int i=0;i<nTurbines;i++)
	{
		MPI_Send(&(avrSWAP_S[i][5]),1,MPI_FLOAT,i,i+40,fastMPIComm);
	}	
	return;
}

void SuperController::closedLoop_apc(int Rank, MPI_Comm fastMPIComm)
{
	if(Rank!=0)
	{
     	   return;
        }
	float time = globStates[0];
	float   dt = globStates[2];
	if (time < apc_time[time_table_idx]) // if cfd timestep is below idx of the TSO table, keep interpolating there, otherwise, move to the next (TSOs) timestep!!! 	
	{
		coeffLine = (apc_power[time_table_idx]-apc_power[time_table_idx-1])/(apc_time[time_table_idx]-apc_time[time_table_idx-1]); // angular coefficient to interpolate power at timestep from lookup ref TSO signal
		demanded_farm_P = apc_power[time_table_idx] + coeffLine*(time-apc_time[time_table_idx]); // 1D interpolation
	}	
	else
	{
		coeffLine = (apc_power[time_table_idx+1]-apc_power[time_table_idx])/(apc_time[time_table_idx+1]-apc_time[time_table_idx]); 
		demanded_farm_P = apc_power[time_table_idx+1] + coeffLine*(time-apc_time[time_table_idx+1]); // 1D interpolation
		time_table_idx = time_table_idx + 1;
	}

	int Ns = 0; // number of saturated turbines
	double tol = 0.0001; // tolerance to check if a=1/3 (optimal axial induction)

	float rel_time = time-globStates[1];
	double rated_power = 3370000;
	double sum_power_check = 0.0;
	
	alpha_coeff[0] = alfa_1;
	alpha_coeff[1] = alfa_2;
	alpha_coeff[2] = alfa_3;

	for (int i=0;i<nTurbines;i++)
	{
		sum_power_check += avrSWAP_S[i][11];
		isSaturated[i] = 0;
		if(( ( (avrSWAP_S[i][11]+36630) < avrSWAP_S[i][5] ) && (avrSWAP_S[i][8] < 1.5) )) // || (avrSWAP_S[i][5]>=rated_power) ) // || ( (contatore[i] > 0) && (contatore[i] < 4000) )	)		
		{		
			isSaturated[i] = 1;
			Ns = Ns + 1;
			std::cout << "Turbine " << i << " is saturated..!" << std::endl;	
		}	
	}
	
	if (sum_power_check - demanded_farm_P > 109890)
	{
		for (int i=0;i<nTurbines;i++)
		{
			isSaturated[i] = 0;
		}
		Ns = 0;
		std::cout << "VeroVerto!!!!" << std::endl;	
	}
	//***************************************************** APC part *****************************************************
	double sum_power = 0;
	double error_APC;

	double Kp_APC_gs;
	double Ki_APC_gs;
	
	if((int)Ns==nTurbines)
	{
		Kp_APC_gs = Kp_APC;	
		Ki_APC_gs = 0;
	}
	else
	{
		Kp_APC_gs = Kp_APC*std::min((double)nTurbines,(double)nTurbines/((double)nTurbines-(double)Ns));
		Ki_APC_gs = Ki_APC*std::min((double)nTurbines,(double)nTurbines/((double)nTurbines-(double)Ns));
	}

	//std::cout << "KP APC=" << Kp_APC_gs << "- KI APC=" << Ki_APC_gs << std::endl;

	for (int i=0;i<nTurbines;i++)
	{
		sum_power = sum_power + avrSWAP_S[i][11];
	}	
	
	error_APC = demanded_farm_P - sum_power; // Eq. (14) Vali al.
	
	sum_error_APC = sum_error_APC + Ki_APC*dt*error_APC; // error sum for integral controller

	if((int)Ns == nTurbines)
	{
		sum_error_APC = 0.0;			
	}
	previous_apc_error = error_APC;

	double backup_delta_P_ref = delta_P_ref;
	delta_P_ref = Kp_APC*error_APC + sum_error_APC; // Eq. (13) Vali

	//std::cout << "sum_error_APC=" << sum_error_APC << "- Kp_APC*error_APC =" << Kp_APC*error_APC << std::endl;

	if(delta_P_ref-backup_delta_P_ref>99000)
	{
		delta_P_ref = backup_delta_P_ref+99000;
	}
	if(delta_P_ref-backup_delta_P_ref<-99000)
	{
		delta_P_ref = backup_delta_P_ref-99000;
	}
		
	double delta_alpha_ns = 0.0;
	double somma2 = 0.0;
	double scarto = 0.0;
	double buffer_power = 5;
	double d_alpha_coeff[nTurbines];
	
	if(Ns>0) // if we have one or more saturated turbines...
	{
		for (int i=0;i<nTurbines;i++)
		{
			if(isSaturated[i] == 1) // if saturated, alfa is just how much you have available at the moment... and reduce 
			{
				alpha_coeff[i] = avrSWAP_S[i][11]/(demanded_farm_P + delta_P_ref);

				d_alpha_coeff[i] = alpha_coeff[i] - old_alpha_coeff[i];
				if(d_alpha_coeff[i]>0.0005)
				{
					alpha_coeff[i]=old_alpha_coeff[i] + 0.0005;
				}
				if(d_alpha_coeff[i]<-0.0005)
				{
					alpha_coeff[i]=old_alpha_coeff[i]-0.0005;
				}
			}
			somma2 = somma2 + alpha_coeff[i];			
		}

		scarto = 1 - somma2; // we must keep sum_alpha = 1

		for (int i=0;i<nTurbines;i++) // so we redistribute the scarto equally on non sature turbins..
		{
			if((int)Ns==nTurbines)
			{
				//alpha_coeff[i]=alpha_coeff[i]/somma2; // non dimensionalize alpha coeffs using their sum
			}
			else 
			{
				delta_alpha_ns = scarto/((float)nTurbines-(float)Ns);   // redistribute equally the set-points...
				if(isSaturated[i]!=1)					// ... on non-saturated turbines!
				{
					alpha_coeff[i]=alpha_coeff[i]+delta_alpha_ns;
				}
			}
		}
	}

	for (int i=0;i<nTurbines;i++)
	{
		old_alpha_coeff[i] = alpha_coeff[i];
		if(isSaturated[i]==0)
		{			
			avrSWAP_S[i][5] = alpha_coeff[i] * (demanded_farm_P + delta_P_ref);
		}
		double d_power_demand = avrSWAP_S[i][5] - old_power_demand[i];

		if(d_power_demand>1000.0)
		{
			avrSWAP_S[i][5]=old_power_demand[i] + 1000.0;
		}
		if(d_power_demand<-1000.0)
		{
			avrSWAP_S[i][5]=old_power_demand[i] - 1000.0;
		}

		if(avrSWAP_S[i][5]>3370000)
		{
			avrSWAP_S[i][5]=3370000;
		}
		old_power_demand[i] = avrSWAP_S[i][5];

		MPI_Send(&(avrSWAP_S[i][5]),1,MPI_FLOAT,i,i+40,fastMPIComm);
	}	
	return;
}

void SuperController::loadBalance_apc(int Rank, MPI_Comm fastMPIComm)
{
	if(Rank!=0)
	{
     	   return;
        }
	float time = globStates[0];
	float dt = globStates[2];
	if (time < apc_time[time_table_idx]) // if cfd timestep is below idx of the TSO table, keep interpolating there, otherwise, move to the next (TSOs) timestep!!! 	
	{
		coeffLine = (apc_power[time_table_idx]-apc_power[time_table_idx-1])/(apc_time[time_table_idx]-apc_time[time_table_idx-1]); // angular coefficient to interpolate power at timestep from lookup ref TSO signal
		demanded_farm_P = apc_power[time_table_idx] + coeffLine*(time-apc_time[time_table_idx]); // 1D interpolation
	}	
	else
	{
		coeffLine = (apc_power[time_table_idx+1]-apc_power[time_table_idx])/(apc_time[time_table_idx+1]-apc_time[time_table_idx]); 
		demanded_farm_P = apc_power[time_table_idx+1] + coeffLine*(time-apc_time[time_table_idx+1]); // 1D interpolation
		time_table_idx = time_table_idx + 1;
	}

	int Ns = 0; // number of saturated turbines
	double tol = 0.0001; // tolerance to check if a=1/3 (optimal axial induction)
	float rel_time = time-globStates[1];
	double rated_power = 3370000;

	if(firstAPCAction==1)
	{
		for (int i=0;i<nTurbines;i++)
		{
			carico[i] = avrSWAP_S[i][24]/1000;
		}
	}

	double sum_power_check = 0.0;
	for (int i=0;i<nTurbines;i++)
	{
		sum_power_check += avrSWAP_S[i][11];
		isSaturated[i] = 0;
		if(( ( (avrSWAP_S[i][11]+36630) < avrSWAP_S[i][5] ) && (avrSWAP_S[i][8] < 1.5) )) // || (avrSWAP_S[i][5]>=rated_power) ) // || ( (contatore[i] > 0) && (contatore[i] < 4000) )	)		
		{		
			isSaturated[i] = 1;
			Ns = Ns + 1;
			std::cout << "Turbine " << i << " is saturated..!" << std::endl;	
		}	
	}
	
	if (sum_power_check - demanded_farm_P > 109890)
	{
		for (int i=0;i<nTurbines;i++)
		{
			isSaturated[i] = 0;
		}
		Ns = 0;
		std::cout << "VeroVerto!!!!" << std::endl;	
	}
	
//***************************************************** CLD part *****************************************************
	double sum_load = 0;
	double error_CLD[nTurbines];
	double d_alpha_coeff[nTurbines];

	double Kp_CLD_gs = Kp_CLD*std::max((float)1/(float)nTurbines,((float)nTurbines-(float)Ns)/(float)nTurbines);
	double Ki_CLD_gs = Ki_CLD*std::max((float)1/(float)nTurbines,((float)nTurbines-(float)Ns)/(float)nTurbines);
	
	if(Ns == nTurbines)
	{
		Ki_CLD_gs = 0;
	}

	for (int i=0;i<nTurbines;i++)
	{
		if (isSaturated[i]==0)
		{
			carico[i] = (1-std::exp(dt*(-2.0)))*avrSWAP_S[i][24]/1000 + std::exp(dt*(-2.0))*carico[i];
			sum_load = sum_load + carico[i];
		}
	}
        
	for (int i=0;i<nTurbines;i++)
	{	
		if (isSaturated[i]==0)
		{
			std::cout << "!" << std::endl;
			error_CLD[i] =  sum_load/(nTurbines-Ns) - carico[i]; // Eq. (19) Vali al.
		}
		else
		{
			std::cout << "!!!" << std::endl;
			error_CLD[i] = 0;
		}
	}		

	for (int i=0;i<nTurbines;i++)
	{			

		sum_error_CLD[i] = sum_error_CLD[i] + Ki_CLD_gs*dt*error_CLD[i]; // error sum for integral controller
		alpha_coeff[i] = Kp_CLD_gs*error_CLD[i] + sum_error_CLD[i]; // Eq. (20) Vali al.				

	}

	if(firstAPCAction==1)
	{
		firstAPCAction=0;
	}	

//***************************************************** APC part *****************************************************
        double sum_power = 0;
        double error_APC;
        //double d_alpha_coeff[nTurbines];

        double Kp_APC_gs;
        double Ki_APC_gs;

        if((int)Ns==nTurbines)
        {
                Kp_APC_gs = Kp_APC;
                Ki_APC_gs = 0;
        }
        else
        {
                Kp_APC_gs = Kp_APC*std::min((double)nTurbines,(double)nTurbines/((double)nTurbines-(double)Ns));
                Ki_APC_gs = Ki_APC*std::min((double)nTurbines,(double)nTurbines/((double)nTurbines-(double)Ns));
        }

        //std::cout << "KP APC=" << Kp_APC_gs << "- KI APC=" << Ki_APC_gs << std::endl;

        for (int i=0;i<nTurbines;i++)
        {
                sum_power = sum_power + avrSWAP_S[i][11];
        }

        error_APC = demanded_farm_P - sum_power; // Eq. (14) Vali al.

        sum_error_APC = sum_error_APC + Ki_APC*dt*error_APC; // error sum for integral controller

        if((int)Ns == nTurbines)
        {
                sum_error_APC = 0.0;
        }
        previous_apc_error = error_APC;

        double backup_delta_P_ref = delta_P_ref;
        delta_P_ref = Kp_APC*error_APC + sum_error_APC; // Eq. (13) Vali

	if(delta_P_ref-backup_delta_P_ref>99000)
	{
		delta_P_ref = backup_delta_P_ref+99000;
	}
	if(delta_P_ref-backup_delta_P_ref<-99000)
	{
		delta_P_ref = backup_delta_P_ref-99000;
	}

	//***************************************************** CORRECTION part *****************************************************

	double delta_alpha_ns = 0.0;
	double somma2 = 0.0;
	double scarto = 0.0;
	double buffer_power = 5;

	if(Ns>0) // if we have one or more saturated turbines...
	{
		for (int i=0;i<nTurbines;i++)
		{
			if(isSaturated[i] == 1) // if saturated, alfa is just how much you have available at the moment... and reduce 
			{
				alpha_coeff[i] = avrSWAP_S[i][11]/(demanded_farm_P + delta_P_ref);

				d_alpha_coeff[i] = alpha_coeff[i] - old_alpha_coeff[i];
				if(d_alpha_coeff[i]>0.0005)
				{
					alpha_coeff[i]=old_alpha_coeff[i] + 0.0005;
				}
				if(d_alpha_coeff[i]<-0.0005)
				{
					alpha_coeff[i]=old_alpha_coeff[i]-0.0005;
				}
				//avrSWAP_S[i][5] = avrSWAP_S[i][5] - 146520*dt/80.0; // wind tunnel time shifts
			}
			somma2 = somma2 + alpha_coeff[i];			
		}

		scarto = 1 - somma2; // we must keep sum_alpha = 1

		for (int i=0;i<nTurbines;i++) // so we redistribute the scarto equally on non sature turbins..
		{
			if((int)Ns==nTurbines)
			{
				//alpha_coeff[i]=alpha_coeff[i]/somma2; // non dimensionalize alpha coeffs using their sum
			}
			else 
			{
				delta_alpha_ns = scarto/((float)nTurbines-(float)Ns);   // redistribute equally the set-points...
				if(isSaturated[i]!=1)					// ... on non-saturated turbines!
				{
					alpha_coeff[i]=alpha_coeff[i]+delta_alpha_ns;
				}
			}
		}
	}

	for (int i=0;i<nTurbines;i++)
	{
		old_alpha_coeff[i] = alpha_coeff[i];
		if(isSaturated[i]==0)
		{			
			avrSWAP_S[i][5] = alpha_coeff[i] * (demanded_farm_P + delta_P_ref);
		}
		double d_power_demand = avrSWAP_S[i][5] - old_power_demand[i];

		if(d_power_demand>1000.0)
		{
			avrSWAP_S[i][5]=old_power_demand[i] + 1000.0;
		}
		if(d_power_demand<-1000.0)
		{
			avrSWAP_S[i][5]=old_power_demand[i] - 1000.0;
		}

		if(avrSWAP_S[i][5]>3370000)
		{
			avrSWAP_S[i][5]=3370000;
		}
		old_power_demand[i] = avrSWAP_S[i][5];

		
		MPI_Send(&(avrSWAP_S[i][5]),1,MPI_FLOAT,i,i+40,fastMPIComm);
	}	
	return;
}


void SuperController::maxReserve_apc(int Rank, MPI_Comm fastMPIComm)
{
	if(Rank!=0)
	{
     	   return;
        }
	float time = globStates[0];
	float dt = globStates[2];
	if (time < apc_time[time_table_idx]) // if cfd timestep is below idx of the TSO table, keep interpolating there, otherwise, move to the next (TSOs) timestep!!! 	
	{
		coeffLine = (apc_power[time_table_idx]-apc_power[time_table_idx-1])/(apc_time[time_table_idx]-apc_time[time_table_idx-1]); // angular coefficient to interpolate power at timestep from lookup ref TSO signal
		demanded_farm_P = apc_power[time_table_idx] + coeffLine*(time-apc_time[time_table_idx]); // 1D interpolation
	}	
	else
	{
		coeffLine = (apc_power[time_table_idx+1]-apc_power[time_table_idx])/(apc_time[time_table_idx+1]-apc_time[time_table_idx]); 
		demanded_farm_P = apc_power[time_table_idx+1] + coeffLine*(time-apc_time[time_table_idx+1]); // 1D interpolation
		time_table_idx = time_table_idx + 1;
	}
	//***************************************************** LUT *****************************************************
	float lut_intervene = 30.0;
	average_tso = average_tso + demanded_farm_P;

	if (time > next_lut)
	{
		if(firstAPCAction==1)
		{
			average_tso = average_tso;
		}
		else
		{
			average_tso = average_tso/(lut_intervene/dt);
		}		
		next_lut = next_lut + lut_intervene;
		start = 0;
		finish = 50;
		differenza = finish - start;
        
		std::cout << "!!! average_tso " << average_tso << std::endl;

		do
		{
			if( (average_tso) > power_lut[middle-1] && (average_tso) < power_lut[middle+1])
			{

		 		if((average_tso) > power_lut[middle])
				{
					indice = middle;
				}
				else
				{
					indice = middle - 1;
				}
				break;
			}	  
			else if((average_tso) > power_lut[middle])
			{
				start = middle + 1;
			}
			else if((average_tso) < power_lut[middle])
			{
				finish = middle  - 1; //start = middle - 1;
			}
			differenza = finish - start;
			middle = (start + finish)/2;
			if(differenza==0)
			{
				indice = middle;
			}
		}
		while (power_lut[middle] != (average_tso) && differenza > 0);
		
		std::cout << "middle ... " << middle << std::endl;


		if(middle == 50)
		{
			alfa[0] = derate0[50];
			alfa[1] = derate1[50];
			alfa[2] = derate2[50];
			yaw_APC[0] = yaw0_APC[50];
			yaw_APC[1] = yaw1_APC[50];
		}
		if(middle == 0)
		{
			alfa[0] = derate0[0];
			alfa[1] = derate1[0];
			alfa[2] = derate2[0];
			ang_coeff = (yaw0_APC[middle])/(power_lut[middle]-power_lut[middle]/2);
		    	quota     = yaw0_APC[middle]   - ang_coeff*power_lut[middle];
		    	yaw_APC[0]   = ang_coeff*(average_tso) + quota;

		    	ang_coeff = (yaw1_APC[middle])/(power_lut[middle]-power_lut[middle]/2);
		    	quota     = yaw1_APC[middle]  - ang_coeff*power_lut[middle];
		    	yaw_APC[1]   = ang_coeff*(average_tso) + quota;
		}
		else
		{
			ang_coeff = (derate0[middle+1]-derate0[indice])/(power_lut[middle+1]-power_lut[indice]);
			quota     = derate0[middle+1]   - ang_coeff*power_lut[middle+1];
			alfa[0]   = ang_coeff*(average_tso) + quota;

			ang_coeff = (derate1[middle+1]-derate1[indice])/(power_lut[middle+1]-power_lut[indice]);
			quota     = derate1[middle+1]  - ang_coeff*power_lut[middle+1];
			alfa[1]   = ang_coeff*(average_tso) + quota;

			ang_coeff = (derate2[middle+1]-derate2[indice])/(power_lut[middle+1]-power_lut[indice]);
			quota     = derate2[middle+1]   - ang_coeff*power_lut[middle+1];
			alfa[2]   = ang_coeff*(average_tso) + quota;

			ang_coeff = (yaw0_APC[middle+1]-yaw0_APC[indice])/(power_lut[middle+1]-power_lut[indice]);
		    quota     = yaw0_APC[middle+1]   - ang_coeff*power_lut[middle+1];
		    yaw_APC[0]   = ang_coeff*(average_tso) + quota;

		    ang_coeff = (yaw1_APC[middle+1]-yaw1_APC[indice])/(power_lut[middle+1]-power_lut[indice]);
		    quota     = yaw1_APC[middle+1]  - ang_coeff*power_lut[middle+1];
		    yaw_APC[1]   = ang_coeff*(average_tso) + quota;

		}
		average_tso = 0.0;
	}

// *********************************************************************************************************************************************************************
	float error_yaw0 = - avrSWAP_S[0][9] + yaw_APC[0];
	float error_yaw1 = - avrSWAP_S[1][9] + yaw_APC[1];
	if(time > 17)
	{
		float incremento_yaw0 = 0.1*error_yaw0;
		float incremento_yaw1 = 0.1*error_yaw1;

		if(error_yaw0*error_yaw0 < 0.01)
		{
			avrSWAP_S[0][3] = yaw_APC[0];
		}
		else if(incremento_yaw0 > 0.008)
		{
			incremento_yaw0 =  0.008;
			avrSWAP_S[0][3] = avrSWAP_S[0][3] + incremento_yaw0;
		}
		else if(incremento_yaw0 < -0.008)
		{
			incremento_yaw0 = -0.008;
			avrSWAP_S[0][3] = avrSWAP_S[0][3] + incremento_yaw0;
		}

		if(error_yaw1*error_yaw1 < 0.01)
	        {
	                avrSWAP_S[1][3] = yaw_APC[1];
	        }
	        else if(incremento_yaw1 > 0.008)
	        {
	                incremento_yaw1 =  0.008;  
	                avrSWAP_S[1][3] = avrSWAP_S[1][3] + incremento_yaw1;
	        }
	        else if(incremento_yaw1 < -0.008) 
	        {
	                incremento_yaw1 = -0.008;
	                avrSWAP_S[1][3] = avrSWAP_S[1][3] + incremento_yaw1;
	        }
	}

	int Ns = 0; // number of saturated turbines
	double tol = 0.0001; // tolerance to check if a=1/3 (optimal axial induction)
	
	float rel_time = time-globStates[1];
	double rated_power = 3370000;

	if(firstAPCAction==1)
	{
		old_alpha_coeff[0] = alfa[0];
		old_alpha_coeff[1] = alfa[1];
		old_alpha_coeff[2] = alfa[2];
		old_power_demand[0] = alfa[0]*demanded_farm_P;
		old_power_demand[1] = alfa[1]*demanded_farm_P;
		old_power_demand[2] = alfa[2]*demanded_farm_P;
		firstAPCAction=0;
	}

	double sum_power_check = 0.0;
	for (int i=0;i<nTurbines;i++)
	{
		sum_power_check += avrSWAP_S[i][11];
		isSaturated[i] = 0;
		if(( ( (avrSWAP_S[i][11]+36630) < avrSWAP_S[i][5] ) && (avrSWAP_S[i][8] < 1.5) )) // || (avrSWAP_S[i][5]>=rated_power) ) // || ( (contatore[i] > 0) && (contatore[i] < 4000) )	)		
		{		
			isSaturated[i] = 1;
			Ns = Ns + 1;
			std::cout << "Turbine " << i << " is saturated..!" << std::endl;	
		}	
	}
	
	if (sum_power_check - demanded_farm_P > 109890)
	{
		for (int i=0;i<nTurbines;i++)
		{
			isSaturated[i] = 0;
		}
		Ns = 0;
	}
	

	avrSWAP_S[2][3] = 0.0; 
	for (int i=0;i<nTurbines;i++)
	{
		MPI_Send(&(avrSWAP_S[i][3]),1,MPI_FLOAT,i,i+80,fastMPIComm);
	}

//***************************************************** APC part *****************************************************
        double sum_power = 0;
        double error_APC;
        double d_alpha_coeff[nTurbines];

        double Kp_APC_gs;
        double Ki_APC_gs;

        if((int)Ns==nTurbines)
        {
                Kp_APC_gs = Kp_APC;
                Ki_APC_gs = 0;
        }
        else
        {
                Kp_APC_gs = Kp_APC*std::min((double)nTurbines,(double)nTurbines/((double)nTurbines-(double)Ns));
                Ki_APC_gs = Ki_APC*std::min((double)nTurbines,(double)nTurbines/((double)nTurbines-(double)Ns));
        }

        //std::cout << "KP APC=" << Kp_APC_gs << "- KI APC=" << Ki_APC_gs << std::endl;

        for (int i=0;i<nTurbines;i++)
        {
                sum_power = sum_power + avrSWAP_S[i][11];
        }

        error_APC = demanded_farm_P - sum_power; // Eq. (14) Vali al.

        sum_error_APC = sum_error_APC + Ki_APC*dt*error_APC; // error sum for integral controller

        if((int)Ns == nTurbines)
        {
                sum_error_APC = 0.0;
        }
        previous_apc_error = error_APC;

        double backup_delta_P_ref = delta_P_ref;
        delta_P_ref = Kp_APC*error_APC + sum_error_APC; // Eq. (13) Vali

        if(delta_P_ref-backup_delta_P_ref>99000)
        {
                delta_P_ref = backup_delta_P_ref+99000;
        }
        if(delta_P_ref-backup_delta_P_ref<-99000)
        {
                delta_P_ref = backup_delta_P_ref-99000;
        }
	
	
	double delta_alpha_ns = 0.0;
	double somma2 = 0.0;
	double scarto = 0.0;
	double buffer_power = 5;
	double margin = 0.0;
	double numPTurb = 0;

	if(Ns>0) // if we have one or more saturated turbines...
	{
		for (int i=0;i<nTurbines;i++)
		{
			if(isSaturated[i] == 1) // if saturated, alfa is just how much you have available at the moment... and reduce 
			{
				alfa[i] = avrSWAP_S[i][11]/(demanded_farm_P + delta_P_ref);

				d_alpha_coeff[i] = alfa[i] - old_alpha_coeff[i];
				if(d_alpha_coeff[i]>0.0001)
				{
					alfa[i]=old_alpha_coeff[i] + 0.0001;
				}
				if(d_alpha_coeff[i]<-0.0001)
				{
					alfa[i]=old_alpha_coeff[i]-0.0001;
				}
				//avrSWAP_S[i][5] = avrSWAP_S[i][5] - 146520*dt/80.0; // wind tunnel time shifts
			}
			somma2 = somma2 + alfa[i];			
		}

		scarto = 1 - somma2; // we must keep sum_alpha = 1

		for (int i=0;i<nTurbines;i++) // so we redistribute the scarto equally on non sature turbins..
		{
			if((int)Ns==nTurbines)
			{
				//alpha_coeff[i]=alpha_coeff[i]/somma2; // non dimensionalize alpha coeffs using their sum
			}
			else 
			{
				delta_alpha_ns = scarto/((float)nTurbines-(float)Ns);   // redistribute equally the set-points...
				if(isSaturated[i]!=1)					// ... on non-saturated turbines!
				{
					alfa[i]=alfa[i]+delta_alpha_ns;
				}
			}
		}
	}

	for (int i=0;i<nTurbines;i++)
	{
		old_alpha_coeff[i] = alfa[i];
		if(isSaturated[i]==0)
		{			
			avrSWAP_S[i][5] = alfa[i] * (demanded_farm_P + delta_P_ref);
		}
		double d_power_demand = avrSWAP_S[i][5] - old_power_demand[i];
	
		if(d_power_demand>1000.0)
		{
			avrSWAP_S[i][5]=old_power_demand[i] + 1000.0;
		}
		if(d_power_demand<-1000.0)
		{
			avrSWAP_S[i][5]=old_power_demand[i] - 1000.0;
		}

		if(avrSWAP_S[i][5]>3370000)
		{
			avrSWAP_S[i][5]=3370000;
		}
		old_power_demand[i] = avrSWAP_S[i][5];

		
		MPI_Send(&(avrSWAP_S[i][5]),1,MPI_FLOAT,i,i+40,fastMPIComm);
	}	
	return;
}











