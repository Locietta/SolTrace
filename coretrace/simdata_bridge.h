
#ifndef SIMDATA_BRIDGE_H
#define SIMDATA_BRIDGE_H

#include "simulation_data/simulation_data.hpp"
#include "types.h"

enum class ConversionErrors
{
	SUCCESS = 0,
	NOT_SUPPORTED_ERROR,
	APERTURE_ERROR,
	SURFACE_ERROR,
	ELEMENT_ADD_ERROR,
	STAGE_ADD_ERROR,
	STAGE_PROP_ERROR,
	END
};

inline const char* get_error_message(ConversionErrors err_enum)
{
	switch (err_enum)
	{
		case(ConversionErrors::NOT_SUPPORTED_ERROR):
			return "Feature not supported";
		case(ConversionErrors::APERTURE_ERROR):
			return "Aperture error";
		case(ConversionErrors::SURFACE_ERROR):
			return "Surface error";
		case(ConversionErrors::ELEMENT_ADD_ERROR):
			return "Element add to stage error";
		case(ConversionErrors::STAGE_ADD_ERROR):
			return "Stage add to scene error";
		default:
			return "No error";
	}
}

inline const char* get_error_message(int err_int)
{
	if (err_int < static_cast<int>(ConversionErrors::SUCCESS) ||
		err_int >= static_cast<int>(ConversionErrors::END))
		return "Incompatible error int";
	return get_error_message(static_cast<ConversionErrors>(err_int));
}

int convert_tsystem_to_sim_data(TSystem* sys, const int seed, SolTrace::Data::SimulationData& sd);

int run_native_runner(SolTrace::Data::SimulationData& sd, TSystem* sys, int nthreads);

int run_native_file_runner(TSystem* sys, const char* file_name, int nthreads);

int run_optix_runner(SolTrace::Data::SimulationData& sd, TSystem* sys);

int run_optix_file_runner(TSystem* sys, const char* file_name);

int run_embree_runner(SolTrace::Data::SimulationData& sd, TSystem* sys, const int nthreads);

#endif