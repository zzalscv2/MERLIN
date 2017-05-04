//Include relevant C++ headers

#include <iostream> // input/output
#include <sstream> // handles string streams
#include <string>
#include <map>
#include <set>
#include <ctime> // used for random seed
#include <sys/stat.h> //to use mkdir

//include relevant MERLIN headers
#include "AcceleratorModel/ApertureSurvey.h"
#include "AcceleratorModel/ControlElements/Klystron.h"

#include "BeamDynamics/ParticleTracking/ParticleBunchConstructor.h"
#include "BeamDynamics/ParticleTracking/ParticleTracker.h"
#include "BeamDynamics/ParticleTracking/ParticleBunchTypes.h"
#include "BeamDynamics/ParticleTracking/Integrators/SymplecticIntegrators.h"
#include "BeamDynamics/TrackingSimulation.h"
#include "BeamDynamics/TrackingOutputASCII.h"
#include "BeamDynamics/TrackingOutputAV.h"

#include "Collimators/CollimateProtonProcess.h"
#include "Collimators/ScatteringProcess.h"
#include "Collimators/ScatteringModel.h"
#include "Collimators/CollimatorDatabase.h"
#include "Collimators/Material.h"
#include "Collimators/MaterialDatabase.h"
#include "Collimators/ApertureConfiguration.h"
#include "Collimators/Dustbin.h"
#include "Collimators/CollimatorSurvey.h"
#include "Collimators/FlukaLosses.h"


#include "MADInterface/MADInterface.h"

#include "NumericalUtils/PhysicalUnits.h"
#include "NumericalUtils/PhysicalConstants.h"

#include "Random/RandomNG.h"

#include "RingDynamics/Dispersion.h"

// C++ std namespace, and MERLIN PhysicalUnits namespace

using namespace std;
using namespace PhysicalUnits;

// Forward function declarations: the following functions will be used to find a specific element in the accelerator lattice
vector<AcceleratorComponent*> SortAcceleratorModel(AcceleratorModel* model);
int FindElementLatticePosition(string RequestedElement, AcceleratorModel* model);
bool SortComponent(const AcceleratorComponent* first, const AcceleratorComponent* last);

// Main function, this executable can be run with the arguments number_of_particles seed

//e.g. for 1000 particles and a seed of 356: ./test 1000 356

int main(int argc, char* argv[])
{
    int seed = (int)time(NULL);     // seed for random number generators
    int iseed = (int)time(NULL);	// seed for random number generators
    int npart = 1E4;                // number of particles to track
    int nturns = 10;                 // number of turns to track
	bool DoTwiss = 1;				// run twiss and align to beam envelope etc?
	bool beam1 = 0;
	 
    //~ if (argc >=2){npart = atoi(argv[1]);}
    if (argc >=3){seed = atoi(argv[2]);}

	seed =0;
    RandomNG::init(seed);

    double beam_energy = 6500.0;

    cout << "npart=" << npart << " nturns=" << nturns << " beam energy = " << beam_energy << endl;

	string start_element;
	if(beam1){	    start_element = "TCP.C6L7.B1"; }  // HORIZONTAL COLLIMATOR (x)
	else{		    start_element = "TCP.C6R7.B2"; }   // HORIZONTAL COLLIMATOR (x)
	    
    // Define useful variables
    double beam_charge = 1.1e11;
    double normalized_emittance = 3.5e-6;
    double gamma = beam_energy/PhysicalConstants::ProtonMassMeV/PhysicalUnits::MeV;
	double beta = sqrt(1.0-(1.0/pow(gamma,2)));
	double emittance = normalized_emittance/(gamma*beta);
	
	//~ string directory = "/afs/cern.ch/user/h/harafiqu/public/MERLIN";	//lxplus harafiqu
	//~ string directory = "/home/haroon/git/Merlin/HR";				//iiaa1
		string directory = "/home/HR/Downloads/MERLIN_HRThesis/MERLIN";					//M11x	
	//~ string directory = "/afs/cern.ch/work/a/avalloni/private/MerlinforFluka/MERLIN";	//lxplus avalloni
	
	//~ string input_dir = "/UserSim/data/6p5TeV_RunII_FlatTop_B2/";
	string input_dir = "/Thesis/data/AV/";
	
	string pn_dir, case_dir, bunch_dir, lattice_dir, fluka_dir, dustbin_dir;			
	
	string output_dir = "/Build/Thesis/outputs/6p5TeV/";
	string batch_directory="27APR_test/";
	 
	string full_output_dir = (directory+output_dir);
	mkdir(full_output_dir.c_str(), S_IRWXU);
	
	full_output_dir = (directory+output_dir+batch_directory);
	mkdir(full_output_dir.c_str(), S_IRWXU);
	
	fluka_dir = full_output_dir + "Fluka/";    
	mkdir(fluka_dir.c_str(), S_IRWXU);  
		
	bool every_bunch			= 1;		// output whole bunch every turn in a single file
	bool rf_test				= 1;
	bool output_initial_bunch 	= 1;
	bool output_final_bunch 	= 1;
		if (output_initial_bunch || output_final_bunch || every_bunch){
			bunch_dir = (full_output_dir+"Bunch_Distn/"); 	mkdir(bunch_dir.c_str(), S_IRWXU); 		
		}	
	
	bool output_fluka_database 	= 1;
	bool output_twiss			= 1;		
		if(output_twiss){ lattice_dir = (full_output_dir+"LatticeFunctions/"); mkdir(lattice_dir.c_str(), S_IRWXU); }	
	
	bool collimation_on 		= 1;
		if(collimation_on){
			dustbin_dir = full_output_dir + "LossMap/"; 	mkdir(dustbin_dir.c_str(), S_IRWXU);		
		}		
	bool use_sixtrack_like_scattering = 0;
	bool scatterplot			= 0;
	bool jawinelastic			= 0;
	bool jawimpact				= 0;
	
	bool ap_survey				= 0;
	bool coll_survey			= 0;
	bool output_particletracks	= 0;
	
	bool cleaning				= 0;
		if(cleaning){
			collimation_on		= 1;
			every_bunch			= 0;	
			output_initial_bunch= 1;
			output_final_bunch	= 1;
		}
	
	bool symplectic				= 1;
	bool sixD					= 0;
	
/************************************
*	ACCELERATORMODEL CONSTRUCTION	*
************************************/
	cout << "MADInterface" << endl;

	MADInterface* myMADinterface;
    if(beam1)
		myMADinterface = new MADInterface( directory+input_dir+"Twiss_6p5TeV.tfs", beam_energy );
    else
		myMADinterface = new MADInterface( directory+input_dir+"Twiss_6p5TeV_flat_top_beam2.tfs", beam_energy );
	cout << "MADInterface Done" << endl;

    //~ myMADinterface->TreatTypeAsDrift("RFCAVITY");
    //~ myMADinterface->TreatTypeAsDrift("SEXTUPOLE");
    //~ myMADinterface->TreatTypeAsDrift("OCTUPOLE");

    myMADinterface->ConstructApertures(false);

    AcceleratorModel* myAccModel = myMADinterface->ConstructModel();    
	
	std::vector<RFStructure*> RFCavities;
	myAccModel->ExtractTypedElements(RFCavities,"ACS*");
	Klystron* Kly1 = new Klystron("KLY1",RFCavities);
	Kly1->SetVoltage(0.0);
	Kly1->SetPhase(pi/2);    


/************
*	TWISS	*
************/

    int start_element_number = myAccModel->FindElementLatticePosition(start_element.c_str());
    
    cout << "Found start element TCP.C6L7 at element number " << start_element_number << endl;

	LatticeFunctionTable* myTwiss = new LatticeFunctionTable(myAccModel, beam_energy);
	myTwiss->UseDefaultFunctions();
	myTwiss->AddFunction(1,6,3);
    myTwiss->AddFunction(2,6,3);
    myTwiss->AddFunction(3,6,3);
    myTwiss->AddFunction(4,6,3);
    myTwiss->AddFunction(6,6,3);
    
    double bscale1 = 1e-22;    
    if(DoTwiss){    
		while(true)
		{
		cout << "start while(true) to scale bend path length" << endl;
			// If we are running a lattice with no RF, the TWISS parameters
			// will not be calculated correctly. This is because some are
			// calculated from using the eigenvalues of the one turn map,
			// which is not complete with RF (i.e. longitudinal motion) 
			// switched off. In order to compensate for this we use the 
			// ScaleBendPath function which calls a RingDeltaT process 
			// and attaches it to the TWISS tracker. RingDeltaT process 
			// adjusts the ct and dp values such that the TWISS may be 
			// calculated and there are no convergence errors
			myTwiss->ScaleBendPathLength(bscale1);
			myTwiss->Calculate();

			// If Beta_x is a number (as opposed to -nan) then we have 
			// calculated the correct TWISS parameters, otherwise the loop
			//  keeps running
			if(!std::isnan(myTwiss->Value(1,1,1,0))) {break;}
			bscale1 *= 2;
			cout << "\n\ttrying bscale = " << bscale1<< endl;
		}
	}
	
	Dispersion* myDispersion = new Dispersion(myAccModel, beam_energy);
    myDispersion->FindDispersion(start_element_number);
	
	if (output_twiss){
		ostringstream twiss_output_file; 
		twiss_output_file << (lattice_dir+"LatticeFunctions.dat");
		ofstream twiss_output(twiss_output_file.str().c_str());
		if(!twiss_output.good()){ std::cerr << "Could not open twiss output file" << std::endl; exit(EXIT_FAILURE); } 
		myTwiss->PrintTable(twiss_output);
	}
	
	if(sixD)
	Kly1->SetVoltage(2.0);
	
/************************
*	Collimator set up	*
************************/

	cout << "Collimator Setup" << endl;   
    
    MaterialDatabase* myMaterialDatabase = new MaterialDatabase();    
    CollimatorDatabase* collimator_db;
    if(beam1)
		collimator_db = new CollimatorDatabase( directory+input_dir+"Collimator_6p5TeV.txt", myMaterialDatabase,  true);
    else
		collimator_db = new CollimatorDatabase( directory+input_dir+"Collimator_6p5TeV_flat_top_beam2.txt", myMaterialDatabase,  true);
   
    collimator_db->MatchBeamEnvelope(true);
    collimator_db->UseMiddleJawHalfGap();
    collimator_db->EnableJawAlignmentErrors(false);

    collimator_db->SetJawPositionError(0.0 * nanometer);
    collimator_db->SetJawAngleError(0.0 * microradian);
    collimator_db->SelectImpactFactor(start_element, 1.0e-6);
    
    double impact = 6;
    // Finally we set up the collimator jaws to appropriate sizes
    try{
		if(DoTwiss)
        impact = collimator_db->ConfigureCollimators(myAccModel, emittance, emittance, myTwiss);
		else
        collimator_db->ConfigureCollimators(myAccModel);
    }
	catch(exception& e){ std::cout << "Exception caught: " << e.what() << std::endl; exit(1); }
    if(std::isnan(impact)){ cerr << "Impact is nan" << endl; exit(1); }
    cout << "Impact factor number of sigmas: " << impact << endl;
    
    if(output_fluka_database && seed == 1){
		ostringstream fd_output_file;
		fd_output_file << (full_output_dir+"fluka_database.txt");

		ofstream* fd_output = new ofstream(fd_output_file.str().c_str());
		collimator_db->OutputFlukaDatabase(fd_output);
		delete fd_output;
	}
    delete collimator_db;

/****************************
*	Aperture Configuration	*
****************************/

	cout << "Aperture Setup" << endl;   
	
	ApertureConfiguration* myApertureConfiguration;
    if(beam1) 
		myApertureConfiguration = new ApertureConfiguration(directory+input_dir+"Aperture_6p5TeV.tfs",1);   
    else   
		myApertureConfiguration = new ApertureConfiguration(directory+input_dir+"Aperture_6p5TeV_beam2.tfs",1);      
    
    myApertureConfiguration->ConfigureElementApertures(myAccModel);
    delete myApertureConfiguration;

	if(ap_survey){
		ApertureSurvey* myApertureSurvey = new ApertureSurvey(myAccModel, full_output_dir, 0.1, 0);
	}
		
	if(coll_survey){
		CollimatorSurvey* CollSurvey = new CollimatorSurvey(myAccModel, emittance, emittance, myTwiss); 
		ostringstream cs_output_file;
		cs_output_file << full_output_dir << "coll_survey.txt";
		ofstream* cs_output = new ofstream(cs_output_file.str().c_str());
		if(!cs_output->good()) { std::cerr << "Could not open collimator survey output" << std::endl; exit(EXIT_FAILURE); }   
		CollSurvey->Output(cs_output, 20);			
		delete cs_output;
	}

/********************
*	Beam Settings	*
********************/

    BeamData mybeam;

    mybeam.charge = beam_charge/npart;
    mybeam.p0 = beam_energy;
    mybeam.beta_x = myTwiss->Value(1,1,1,start_element_number)*meter;
    mybeam.beta_y = myTwiss->Value(3,3,2,start_element_number)*meter;
    mybeam.alpha_x = -myTwiss->Value(1,2,1,start_element_number);
    mybeam.alpha_y = -myTwiss->Value(3,4,2,start_element_number);
    
    // Minimum and maximum sigma for HEL Halo Distribution
    //~ mybeam.min_sig_x = 5.5;
    //~ mybeam.max_sig_x = 5.54;
    //~ mybeam.min_sig_y = 0;
    //~ mybeam.max_sig_y = 3;
   
    // Dispersion
    mybeam.Dx=myDispersion->Dx;
    mybeam.Dy=myDispersion->Dy;
    mybeam.Dxp=myDispersion->Dxp;
    mybeam.Dyp=myDispersion->Dyp;

    mybeam.emit_x = emittance * meter;
    mybeam.emit_y = emittance * meter;


    //Beam centroid
    mybeam.x0=myTwiss->Value(1,0,0,start_element_number);
    mybeam.xp0=myTwiss->Value(2,0,0,start_element_number);
    mybeam.y0=myTwiss->Value(3,0,0,start_element_number);
    mybeam.yp0=myTwiss->Value(4,0,0,start_element_number);
    mybeam.ct0=myTwiss->Value(5,0,0,start_element_number);

    mybeam.sig_z = ((2.51840894498383E-10 * 299792458)) * meter;
    mybeam.sig_dp =  (1.129E-4);

    // X-Y coupling
    mybeam.c_xy=0.0;
    mybeam.c_xyp=0.0;
    mybeam.c_xpy=0.0;
    mybeam.c_xpyp=0.0;

    delete myDispersion;

///////////
// BUNCH //
///////////

    // As we are tracking protons we create a proton bunch   
    ProtonBunch* myBunch;
    int node_particles = npart;

    // horizontalHaloDistribution1 is a halo in xx' plane, zero in yy'
    // horizontalHaloDistribution2 is a halo in xx' plane, gaussian in yy'
    ParticleBunchConstructor* myBunchCtor = new ParticleBunchConstructor(mybeam, node_particles, HorizontalHaloDistributionWithLimits);
    //~ ParticleBunchConstructor* myBunchCtor = new ParticleBunchConstructor(mybeam, node_particles, horizontalHaloDistribution1);

    myBunch = myBunchCtor->ConstructParticleBunch<ProtonBunch>();
    delete myBunchCtor;

    myBunch->SetMacroParticleCharge(mybeam.charge);
    
//create string stream to add distribution and filename
ostringstream hbunch_output_file;
// add together the directory name and filename
//hbunch_output_file << full_output_dir  << "initial_bunch.txt";
hbunch_output_file << full_output_dir << "_" << seed  << "initial_bunch.txt";

//create an output file stream using the above directory+name
ofstream* hbunch_output = new ofstream(hbunch_output_file.str().c_str());
//check that the file cna be opened (that the directory exists and is accessible)
if(!hbunch_output->good()) { std::cerr << "Could not open initial halo bunch output" << std::endl; exit(EXIT_FAILURE); }   
// output the bunch
myBunch->Output(*hbunch_output);			
// delete the pointer
delete hbunch_output;	
    
    
// Our bunch is now complete and ready for tracking & collimation

/////////////////////
// ParticleTracker //
/////////////////////

    // We need to create an AcceleratorModel iterator to iterate through the AcceleratorComponents in the tracker
    // The GetRing function takes the starting element number as an argument, and will return an interator that can be used for more than one turn
    AcceleratorModel::RingIterator beamline = myAccModel->GetRing(start_element_number);
    ParticleTracker* myParticleTracker = new ParticleTracker(beamline, myBunch);
    //~ myParticleTracker->SetIntegratorSet(new ParticleTracking::SYMPLECTIC::StdISet());
    myParticleTracker->SetIntegratorSet(new ParticleTracking::TRANSPORT::StdISet());
    
    //~ string tof = "Tracking_output_file.txt";
    //~ string TrackingOutputfile = full_output_dir+tof;
    
	//~ TrackingOutputASCII* myTrackingOutputASCII=new TrackingOutputASCII (TrackingOutputfile);
	//~ myTrackingOutputASCII->SuppressUnscattered (npart +1);
	//~ myTrackingOutputASCII->output_all = 1;
	
	//~ myParticleTracker->SetOutput(myTrackingOutputASCII);
	//~ string tof = "Tracking_output_file.txt";
    //~ string t_o_f = full_output_dir+tof;
    
/////////////////////
// TrackingOutput  //
/////////////////////
    //~ ostringstream trackingparticles_sstream;
    //~ trackingparticles_sstream << full_output_dir<<"Tracking_output_file_"<< npart << "_" << seed << std::string(".txt");     
    //~ string trackingparticles_file = trackingparticles_sstream.str().c_str();     
     
    //~ TrackingOutputAV* myTrackingOutputAV = new TrackingOutputAV(trackingparticles_file);
    //~ myTrackingOutputAV->SetSRange(0, 27000);
    //~ myTrackingOutputAV->SetTurn(1);
    //~ myTrackingOutputAV->output_all = 1;
     
    //~ myParticleTracker->SetOutput(myTrackingOutputAV);

/////////////////////////
// Collimation Process //
/////////////////////////
    // Finally we create any PhysicsProcesses and assign them to the tracker
    // In this case we only need the collimation process

    // We declare our process, every PhysicsProcess takes a priority and a mode, as we have no other processes the priority is irrelevent
    // As we have set up no modes in the CollimateParticleProcess, the mode is also irrelevent
    // We also give the output file created earlier as an argument to CollimateProtonProcess
    //~ CollimateProtonProcess* myCollimateProcess =new CollimateProtonProcess(2, 4, col_output);
    CollimateProtonProcess* myCollimateProcess =new CollimateProtonProcess(2, 4, NULL);
    
    // As well as the standard output we will create a special LossMapDustbin, this will automatically sort and collate all losses
	// into a single file that we can use to plot
	LossMapDustbin* myLossMapDustbin = new LossMapDustbin;
	myCollimateProcess->SetDustbin(myLossMapDustbin);   
	
	FlukaDustbin* myFlukaDustbin = new FlukaDustbin;
	myCollimateProcess->SetDustbin(myFlukaDustbin);

	FlukaLosses* myFlukaLosses = new FlukaLosses;
	myCollimateProcess->SetFlukaLosses(myFlukaLosses);
	
    // If the ScatterAtCollimator flag is true, collimation involves a full scattering simulation, if it is false, any particle to hit a collimator jaw is lost
    myCollimateProcess->ScatterAtCollimator(true);
   
    // We must assign a ScatteringModel to the CollimateProtonProcess, this will allow us to choose the type of scattering that will be used
    ScatteringModel* myScatter = new ScatteringModel;
    if(beam1){
		myScatter->SetScatterPlot("TCP.C6L7.B1");
		myScatter->SetJawImpact("TCP.C6L7.B1");
		myScatter->SetScatterPlot("TCP.B6L7.B1");
		myScatter->SetJawImpact("TCP.D6L7.B1");
	}
	else{ 
		//myScatter->SetScatterPlot("TCP.C6R7.B2");
		myScatter->SetJawImpact("TCP.C6R7.B2");
		//myScatter->SetScatterPlot("TCSG.B4R7.B2");
		myScatter->SetJawImpact("TCSG.B4R7.B2");	
		myScatter->SetJawImpact("TCSG.B5R7.B2");	
		//myScatter->SetScatterPlot("TCP.D6R7.B2");
		myScatter->SetJawImpact("TCP.D6R7.B2");
		myScatter->SetJawImpact("TCP.B6R7.B2");
		
		myScatter->SetJawInelastic("TCP.C6R7.B2");     
        myScatter->SetJawInelastic("TCSG.B4R7.B2");
        myScatter->SetJawInelastic("TCP.D6R7.B2");     
        myScatter->SetJawInelastic("TCP.B6R7.B2");
	}

    // MERLIN contains various ScatteringProcesses; namely the following
    // Rutherford, Elastic pn, Elastic pN, Single Diffractive, and Inelastic
    // Each of these have a MERLIN and SixTrack like version (except the inelastic)
    // As well as this there are 2 types of ionisation; simple (SixTrack like), and advanced
    // There exist 5 pre-defined set ups which may be used:
    // 0: ST,    1: ST + Adv. Ionisation,    2: ST + Adv. Elastic,    3: ST + Adv. SD,     4: MERLIN
    // Where ST = SixTrack like, Adv. = Advanced, SD = Single Diffractive, and MERLIN includes all advanced scattering

    // Below we select the simplest case (0); SixTrack like scattering and ionisation

    if(use_sixtrack_like_scattering){
        myScatter->SetScatterType(0);
    }
    else{
        myScatter->SetScatterType(4);
    }

    // We must assign the ScatteringModel to the CollimateProtonProcess
    myCollimateProcess->SetScatteringModel(myScatter);

    // SetLossThreshold will stop the simulation if the percentage given (in this case 200%) of particles are lost
    // This case is obviously not possible and is used to make sure that the simulation is not stopped in the collimation process
    myCollimateProcess->SetLossThreshold(200.0);

    // The bin size is used to define the maximum step size in the collimator, as well as the output bin size, here it is set to 0.1m
    myCollimateProcess->SetOutputBinSize(0.1);

    // Finally we attach the process to the tracker, note that the tracker runs processes before tracking
    // The collimation process operates such that any particle outside the aperture in a collimator is collimated (scattered etc)
    // A particle outside the aperture in any other element is not collimated, instead it is immediately lost
    myParticleTracker->AddProcess(myCollimateProcess);

//////////////////
// TRACKING RUN //
//////////////////

    // Now all we have to do is create a loop for the number of turns and use the Track() function to perform tracking   
    for (int turn=1; turn<=nturns; turn++)
    {
        // This line will give us an update of how many particles have survived after each turn
        cout << "Turn " << turn <<"\tParticle number: " << myBunch->size() << endl;

        myParticleTracker->Track(myBunch);

        // An escape clause so that we do not needlessly track when if no particles have survived
        if( myBunch->size() <= 1 ) break;
    }

    // Flushing the output file attempts to conclude all writing to the file, then it is closed and deleted
    //~ col_output->flush();
    //~ col_output->close();
    //~ delete col_output;

	
	/*********************************************************************
	**	Output Jaw Impact
	*********************************************************************/
	myScatter->OutputJawImpact(full_output_dir,seed);
	myScatter->OutputScatterPlot(full_output_dir,seed);	
    myScatter->OutputJawInelastic(full_output_dir,seed);
    
	/*********************************************************************
	** OUTPUT FLUKA DUSTBIN 
	*********************************************************************/
	ostringstream fluka_dustbin_file;
	fluka_dustbin_file << full_output_dir<<std::string("fluka_losses_")<< npart << "_" << seed << std::string(".txt");	   
	  
	ofstream* fluka_dustbin_output = new ofstream(fluka_dustbin_file.str().c_str());	
	if(!fluka_dustbin_output->good())    {
        std::cerr << "Could not open dustbin loss file" << std::endl;
        exit(EXIT_FAILURE);
    } 	
	myFlukaDustbin->Finalise(); 
	myFlukaDustbin->Output(fluka_dustbin_output); 
	
	/*********************************************************************
	** OUTPUT FLUKA LOSSES 
	*********************************************************************/	
	ostringstream fluka_file;
	fluka_file << fluka_dir << std::string("fluka_new_losses_")<< npart << "_" << seed << std::string(".txt");	
	
	ofstream* fluka_output1 = new ofstream(fluka_file.str().c_str());   
	if(!fluka_output1->good()){ std::cerr << "Could not open fluka loss file" << std::endl; exit(EXIT_FAILURE); }  
	
	myFlukaLosses->Finalise();
	myFlukaLosses->Output(fluka_output1);
	delete fluka_output1;  
  
  
   /*********************************************************************
	** OUTPUT LOSSMAP  
	*********************************************************************/
	ostringstream dustbin_file;
	dustbin_file << full_output_dir<<"Dustbin_losses_"<< npart << "_" << seed << std::string(".txt");	
	ofstream* dustbin_output = new ofstream(dustbin_file.str().c_str());	
	if(!dustbin_output->good())    {
        std::cerr << "Could not open dustbin loss file" << std::endl;
        exit(EXIT_FAILURE);
    }   
	
	myLossMapDustbin->Finalise(); 
	myLossMapDustbin->Output(dustbin_output); 
   
    // These lines tell us how many particles we tracked, how many survived, and how many were lost
    cout << "npart: " << npart << endl;
    cout << "left: " << myBunch->size() << endl;
    cout << "absorbed: " << npart - myBunch->size() << endl;

    // Cleanup our pointers on the stack for completeness
    delete myMaterialDatabase;
    delete myBunch;
    delete myTwiss;
    delete myAccModel;
    delete myMADinterface;

    return 0;
}



// The following functions will be used to find a specific element in the accelerator lattice
// Firstly a simple comparison function to establish which element comes first in the lattice depending on its (s) position

bool SortComponent(const AcceleratorComponent* first, const AcceleratorComponent* last){
    return (first->GetComponentLatticePosition() < last->GetComponentLatticePosition());
}

// Next a function that returns a vector of AcceleratorComponents that have been sorted in order of position using the previous function

vector<AcceleratorComponent*> SortAcceleratorModel(AcceleratorModel* model){
    vector<AcceleratorComponent*> elements;

    model->ExtractTypedElements(elements,"*"); //This line extracts all elements as the wildcard * is used

    //Now sort the elements in the appropriate lattice order
    sort(elements.begin(), elements.end(),SortComponent);
    return elements;
}

// Finally a function to find the position of an element in the lattice using the previous function

int FindElementLatticePosition(string RequestedElement, AcceleratorModel* model){
    vector<AcceleratorComponent*> elements = SortAcceleratorModel(model);
    size_t nelm = elements.size();
    for(size_t n=0; n<nelm; n++){
        if(elements[n]->GetName() == RequestedElement){
            cout << "Found " << RequestedElement << " at " << n << " of " << nelm << endl;
            return n;
        }
    }
    return 0;
}