#include "SimpleMOC-kernel_header.h"

int main( int argc, char * argv[] )
{
	int version = 1;

	srand(time(NULL));

	Input I = set_default_input();
	read_CLI( argc, argv, &I );

	logo(version);

	print_input_summary(I);
	
	center_print("INITIALIZATION", 79);
	border_print();

	// Setup CUDA blocks / threads
	// int n_blocks = sqrt(I.segments);
	// dim3 blocks(n_blocks, n_blocks);
	// if( blocks.x * blocks.y < I.segments )
	// 	blocks.x++;
	// if( blocks.x * blocks.y < I.segments )
	// 	blocks.y++;
	// assert( blocks.x * blocks.y >= I.segments );

	// Setup OCCA device and info
  int outer_dim = sqrt(I.segments);
  int inner_dim = I.egroups;
  occaDevice device;
  occaKernelInfo kinfo = occaGenKernelInfo();
  occaKernelInfoAddDefine(lookupInfo, "outer_dim0", occaLong(outer_dim));
  occaKernelInfoAddDefine(lookupInfo, "outer_dim1", occaLong(outer_dim));
  occaKernelInfoAddDefine(lookupInfo, "inner_dim", occaLong(inner_dim));

	// Build Source Data
	printf("Building Source Data Arrays...\n");
	Source_Arrays SA_h; 
  OCCA_Source_Arrays SA_d;
	Source * sources_h = initialize_sources(I, &SA_h); 
	occaMemory sources_d = initialize_occa_sources( I, &SA_h, &SA_d, sources_h, device); 
  device.finish()
	
	// Build Exponential Table
	printf("Building Exponential Table...\n");
	Table table = buildExponentialTable();
	Table * table_d;
	CUDA_CALL( cudaMalloc((void **) &table_d, sizeof(Table)) );
	CUDA_CALL( cudaMemcpy(table_d, &table, sizeof(Table), cudaMemcpyHostToDevice) );

	// Setup RNG on Device
	// NOTE - CUDA RNG is not going to work with OCCA - so we're going to revert
	// to the standard linear congruential generation algorithm while preserving
	// the limited state streams concept (i.e., # RNG states << # GPU threads)
	printf("Setting up RNG...\n");
	unsigned long * RNG_states_h = (unsigned long *) malloc( I.streams * sizeof(unsigned long));
	unsigned long * RNG_states;
	// Init states to something
	unsigned long time_of_exec = time(NULL);
	for( int i = 0; i < I.streams; i++ )
		RNG_states_h[i] = time_of_exec + i + 1;
	CUDA_CALL( cudaMalloc((void **)&RNG_states, I.streams * sizeof(unsigned long)) );
	CUDA_CALL( cudaMemcpy(RNG_states, RNG_states_h, I.streams * sizeof(unsigned long), cudaMemcpyHostToDevice));
	free(RNG_states_h); // as we don't need host states anymore
	cudaDeviceSynchronize();

	// Allocate Some Flux State vectors to randomly pick from
	printf("Setting up Flux State Vectors...\n");
	float * flux_states;
	int N_flux_states = 10000;
	assert( I.segments >= N_flux_states );
	CUDA_CALL( cudaMalloc((void **) &flux_states, N_flux_states * I.egroups * sizeof(float)) );
	init_flux_states<<< blocks, I.egroups >>> ( flux_states, N_flux_states, I, RNG_states );


	printf("Initialization Complete.\n");
	border_print();
	center_print("SIMULATION", 79);
	border_print();
	cudaDeviceSynchronize();
	printf("Attentuating fluxes across segments...\n");

	// CUDA timer variables
	cudaEvent_t start, stop;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);
	float time = 0;

	// Run Simulation Kernel Loop
	cudaEventRecord(start, 0);
	run_kernel <<< blocks, I.egroups >>> (I, sources_d, SA_d, table_d, 
			RNG_states, flux_states, N_flux_states);
	CudaCheckError();
	cudaEventRecord(stop, 0);
	cudaEventSynchronize(start);
	cudaEventSynchronize(stop);
	cudaEventElapsedTime(&time, start, stop);
	cudaDeviceSynchronize();

	// Copy final data back to host, for kicks.
	float * host_flux_states = (float*) malloc(N_flux_states * I.egroups * sizeof(float));
	CUDA_CALL( cudaMemcpy( host_flux_states, flux_states, N_flux_states * I.egroups * sizeof(float), cudaMemcpyDeviceToHost));

	printf("Simulation Complete.\n");

	border_print();
	center_print("RESULTS SUMMARY", 79);
	border_print();

	double tpi = ((double) (time/1000.0) /
			(double)I.segments / (double) I.egroups) * 1.0e9;
	printf("%-25s%.3f seconds\n", "Runtime:", time / 1000.0);
	printf("%-25s%.3lf ns\n", "Time per Intersection:", tpi);
	border_print();

	return 0;
}