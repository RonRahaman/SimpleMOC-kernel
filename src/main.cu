#include "SimpleMOC-kernel_header.h"

int main( int argc, char * argv[] )
{
	int version = 0;

	srand(time(NULL));

	Input I = set_default_input();
	read_CLI( argc, argv, &I );

	logo(version);

	print_input_summary(I);

	// Build Source Data
	Source_Arrays SA_h, SA_d;
	Source * sources_h = initialize_sources(I, &SA_h); 
	Source * sources_d = initialize_device_sources( I, &SA_h, &SA_d, sources_h); 
	
	// Build Exponential Table
	Table table = buildExponentialTable();
	Table * table_d;
	cudaMalloc((void **) &table_d, sizeof(Table));
	cudaMemcpy(table_d, &table, sizeof(Table), cudaMemcpyHostToDevice);

	// Setup CUDA blocks / threads
	int n_blocks = sqrt(I.segments);
	dim3 blocks(n_blocks, n_blocks);
	if( blocks.x * blocks.y < I.segments )
		blocks.x++;
	if( blocks.x * blocks.y < I.segments )
		blocks.y++;
	assert( blocks.x * blocks.y >= I.segments );
	
	// Setup CUDA RNG on Device
	curandState * RNG_states;
	cudaMalloc((void **)&RNG_states, I.segments * sizeof(curandState));
	setup_kernel<<<blocks, I.egroups>>>(RNG_states);

	// Allocate Some Flux State vectors to randomly pick from
	float * flux_states;
	int N_flux_states = 1000000;
	cudaMalloc((void **) &flux_states, 1000000 * I.egroups * sizeof(float));
	init_flux_states<<< blocks, I.egroups >>> ( flux_states, N_flux_states, I, RNG_states );

	center_print("SIMULATION", 79);
	border_print();
	printf("Attentuating fluxes across segments...\n");

	double start, stop;

	// Run Simulation Kernel Loop
	start = get_time();
	run_kernel <<< blocks, I.egroups >>> (I, sources_d, &SA_d, table_d, 
			RNG_states, flux_states, N_flux_states);
	stop = get_time();

	printf("Simulation Complete.\n");

	border_print();
	center_print("RESULTS SUMMARY", 79);
	border_print();

	double tpi = ((double) (stop - start) /
			(double)I.segments / (double) I.egroups) * 1.0e9;
	printf("%-25s%.3lf seconds\n", "Runtime:", stop-start);
	printf("%-25s%.3lf ns\n", "Time per Intersection:", tpi);
	border_print();

	return 0;
}
