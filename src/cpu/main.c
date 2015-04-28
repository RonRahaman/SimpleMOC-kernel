#include "SimpleMOC-kernel_header.h"

int main( int argc, char * argv[] )
{
	int version = 2;

#ifdef PAPI
	papi_serial_init();
#endif

#ifdef VERIFY
	srand(20000);
#else
	srand(time(NULL));
#endif

	Input * I = set_default_input();
	read_CLI( argc, argv, I );

	logo(version);

#ifdef OPENMP
	omp_set_num_threads(I->nthreads); 
#endif

	print_input_summary(I);

	// Build Source Data
	Source * S = initialize_sources(I); 
	float * state_flux_vals = initialize_state_flux_vals(I);
	int * QSR_vals = initialize_QSR_vals(I);
	int * FAI_vals = initialize_FAI_vals(I);

	// Build Exponential Table
	Table * table = buildExponentialTable( 0.01, 10.0 );

	center_print("SIMULATION", 79);
	border_print();
	printf("Attentuating fluxes across segments...\n");

	double start, stop;

	// Run Simulation Kernel Loop
	start = get_time();
	run_kernel(I, S, table, state_flux_vals, QSR_vals, FAI_vals);
	stop = get_time();

	printf("Simulation Complete.\n");

	unsigned long long csum = verify_FSR_flux(I, S);

	border_print();
	center_print("RESULTS SUMMARY", 79);
	border_print();

	double tpi = ((double) (stop - start) /
			(double)I->segments / (double) I->egroups) * 1.0e9;
	printf("%-25s%.3lf seconds\n", "Runtime:", stop-start);
	printf("%-25s%.3lf ns\n", "Time per Intersection:", tpi);
	printf("%-25s%lld\n", "Checksum:", csum);
	border_print();

	return 0;
}
