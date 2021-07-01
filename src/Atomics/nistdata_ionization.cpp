/* This file was auto-generated by 'get_nist.py' on 2021-07-01 16:32:22 */

#include "DREAM/nistdata.h"

/* H */
const len_t H_ionization_Z = 1;
const real_t H_ionization_data[1] = {13.5984345997};
/* He */
const len_t He_ionization_Z = 2;
const real_t He_ionization_data[2] = {24.5873890110,54.4177654860};
/* Li */
const len_t Li_ionization_Z = 3;
const real_t Li_ionization_data[3] = {5.3917149960,75.6400970000,122.4543591400};
/* Be */
const len_t Be_ionization_Z = 4;
const real_t Be_ionization_data[4] = {9.3226990000,18.2111500000,153.8962050000,217.7185861000};
/* C */
const len_t C_ionization_Z = 6;
const real_t C_ionization_data[6] = {11.2602880000,24.3831540000,47.8877800000,64.4935200000,392.0905180000,489.9931980000};
/* O */
const len_t O_ionization_Z = 8;
const real_t O_ionization_data[8] = {13.6180550000,35.1211200000,54.9355400000,77.4135000000,113.8990000000,138.1189000000,739.3268300000,871.4098830000};
/* Ne */
const len_t Ne_ionization_Z = 10;
const real_t Ne_ionization_data[10] = {21.5645410000,40.9629700000,63.4233000000,97.1900000000,126.2470000000,157.9340000000,207.2710000000,239.0970000000,1195.8078400000,1362.1991600000};
/* Ar */
const len_t Ar_ionization_Z = 18;
const real_t Ar_ionization_data[18] = {15.7596119000,27.6296700000,40.7350000000,59.5800000000,74.8400000000,91.2900000000,124.4100000000,143.4567000000,422.6000000000,479.7600000000,540.4000000000,619.0000000000,685.5000000000,755.1300000000,855.5000000000,918.3750000000,4120.6657000000,4426.2229000000};
const len_t nist_ionization_n = 8;
struct nist_data nist_ionization_table[8] = {
	{"H",H_ionization_Z,H_ionization_data},
	{"He",He_ionization_Z,He_ionization_data},
	{"Li",Li_ionization_Z,Li_ionization_data},
	{"Be",Be_ionization_Z,Be_ionization_data},
	{"C",C_ionization_Z,C_ionization_data},
	{"O",O_ionization_Z,O_ionization_data},
	{"Ne",Ne_ionization_Z,Ne_ionization_data},
	{"Ar",Ar_ionization_Z,Ar_ionization_data}
};
