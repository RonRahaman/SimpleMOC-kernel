#include "SimpleMOC-kernel_header.h"

// QSR_id = [0,I->source_regions-1]
// FAI_id = [0,I->fine_axial_intervals-1]
// float * FSR_flux = &S[QSR_id].fine_flux[FAI_id * I->egroups];
// FSR_flux[g] += tally[g];

unsigned long long verify_FSR_flux(Input * I, Source * S)
{
	char line[256];
	unsigned long long csum = 0;
	for( int i = 0; i < I->source_regions; i++ )
		for( int j = 0; j < I->fine_axial_intervals; j++ )
			for( int k = 0; k < I->egroups; k++ )
			{
				//sprintf(line, "%.8lf", S[i].fine_flux[j * I->egroups + k]);
				//csum += hash(line, 10000);
				csum += S[i].fine_flux[j * I->egroups + k];
			}
	return csum;
}

unsigned int hash(unsigned char *str, int nbins)
{
	unsigned int hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c;

	return hash % nbins;
}

