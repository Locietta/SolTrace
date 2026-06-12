#include "simdata_bridge.h"
#include "simulation_data/simulation_data.hpp"
#include "types.h"
#include "ray_source.hpp"
#include "sun.hpp"
#include "stage_element.hpp"
#include "single_element.hpp"
#include "virtual_element.hpp"
#include <optix_runner.hpp>
#include <native_runner.hpp>
#include "simdata_io.hpp"
#include <simulation_result.hpp>
#include <embree_runner/embree_runner.hpp>
#include <glm/gtc/type_ptr.hpp>

// Private

void convert_user_sun_data(const std::vector<double>& sun_shape_angle, const std::vector<double>& sun_shape_intensity,
    std::vector<double>& sun_shape_angle_reduced, std::vector<double>& sun_shape_intensity_reduced)
{
    if (sun_shape_angle.size() == 0 || sun_shape_intensity.size() == 0 || sun_shape_angle.size() != sun_shape_intensity.size())
    {
        sun_shape_angle_reduced = sun_shape_angle;
        sun_shape_intensity_reduced = sun_shape_intensity;
        return;
    }

    int n_pts_full = sun_shape_angle.size();
    int start_position = (n_pts_full - 1) / 2;

    sun_shape_angle_reduced.clear();
    sun_shape_intensity_reduced.clear();

    for (int i = start_position; i < n_pts_full; i++)
    {
        sun_shape_angle_reduced.push_back(sun_shape_angle[i]);
        sun_shape_intensity_reduced.push_back(sun_shape_intensity[i]);
    }

    return;
}

int assign_raydata_from_hitpoints(const std::vector<float4>& hp_vec, const std::vector<uint_fast64_t>& raynumber_vec,
    TSystem* sys)
{
    // Assign raydata to TSystem (for legacy GUI)
    sys->AllRayData.Clear();
    for (TStage* stage : sys->StageList)
        sys->AllRayData.Merge(stage->RayData);
    int i_element = 0;
    for (float4 element : hp_vec)
    {
        double stage_optix = element.x; // This is the DEPTH, NOT the stage in the soltrace sense
        double PosRaySurfStage[3] = { element.y, element.z, element.w };
        double CosRaySurfStage[3] = { 0,0,0 };  // Don't have cos reported from optix
        int element_number = 1; // Don't get element number from optix
        int raynumber = raynumber_vec[i_element];

        // Only add ray data if it is Not the sun ray ('stage' 0)
        if (stage_optix != 0)
        {
            sys->StageList[0]->RayData.Append(PosRaySurfStage, CosRaySurfStage, element_number,
                stage_optix, raynumber);
        }
        i_element++;
    }

    for (TStage* stage : sys->StageList)
        sys->AllRayData.Merge(stage->RayData);

    return 0;
}

int set_tstage_parameters(TSystem* sys_legacy, const SolTrace::NativeRunner::TSystem& sys_native)
{
    const int n_stage_native = sys_native.StageList.size();
    const int n_stage_legacy = sys_legacy->StageList.size();

    if (n_stage_native != n_stage_legacy)
        return static_cast<int>(ConversionErrors::STAGE_PROP_ERROR);

    for (int i = 0; i < n_stage_native; i++)
    {
        TStage* stage_legacy = sys_legacy->StageList[i];
        SolTrace::NativeRunner::tstage_ptr stage_native = sys_native.StageList[i];

        stage_native->MultiHitsPerRay = stage_legacy->MultiHitsPerRay;
        stage_native->Virtual = stage_legacy->Virtual;
        stage_native->TraceThrough = stage_legacy->TraceThrough;
    }

    return static_cast<int>(ConversionErrors::SUCCESS);
}

void get_raydata_from_native_tsys(TSystem* sys_legacy, const SolTrace::NativeRunner::TSystem& sys_native)
{
    // Copy sun stats (needed for flux normalization)
    sys_legacy->Sun.MinXSun = sys_native.Sun.MinXSun;
    sys_legacy->Sun.MaxXSun = sys_native.Sun.MaxXSun;
    sys_legacy->Sun.MinYSun = sys_native.Sun.MinYSun;
    sys_legacy->Sun.MaxYSun = sys_native.Sun.MaxYSun;
    sys_legacy->SunRayCount = static_cast<st_uint_t>(sys_native.SunRayCount);

    for (TStage* s : sys_legacy->StageList)
        s->RayData.Clear();
    sys_legacy->AllRayData.Clear();

    // Copy from native TOTAL (aggregate) ray list instead of per stage
    const auto nCount = sys_native.RayData.Count();
    const auto nStage = sys_native.StageList.size();
    for (uint_fast64_t i = 0; i < nCount; ++i)
    {
        // Native TRayData::ray_t layout (Query API)
        glm::dvec3 pos_glob, cos_glob;
        int element = 0, stageIdx1 = 0;
        uint_fast64_t raynum = 0;
        SolTrace::Result::RayEvent ray_event;

        // Get next ray entry
        if (!sys_native.RayData.Query(
            (unsigned int)i,
            pos_glob,            // pos
            cos_glob,            // cos
            &element,       // element
            &stageIdx1,     // stage
            &raynum,        // ray number
            &ray_event))       // event
        {
            continue;
        }

        // Skip Create rays
        if (ray_event == SolTrace::Result::RayEvent::CREATE)
            continue;

        // Skip exit rays for last stage
        if (ray_event == SolTrace::Result::RayEvent::EXIT && stageIdx1 == nStage)
            continue;

        if (stageIdx1 < 1 || (size_t)stageIdx1 > sys_native.StageList.size())
            continue; // out-of-range stage index, skip

        // Need to convert native runner GLOBAL coords to stage
        glm::dvec3 pos_stage, cos_stage;
        SolTrace::NativeRunner::TStage stage = *sys_native.StageList[stageIdx1 - 1];
        SolTrace::Data::TransformToLocal(pos_glob, cos_glob, stage.Origin, stage.RRefToLoc, pos_stage, cos_stage);

        sys_legacy->AllRayData.Append(
            glm::value_ptr(pos_stage),
            glm::value_ptr(cos_stage),
            element,        // negative if absorbed already encoded
            stageIdx1,      // 1-based
            raynum);
    }
}

// Public

int convert_tsystem_to_sim_data(TSystem* sys, const int seed, SolTrace::Data::SimulationData &sd)
{
	// Sun
	{
		// Check if sun is point source (not supported)
		if (sys->Sun.PointSource == true)
			return static_cast<int>(ConversionErrors::NOT_SUPPORTED_ERROR);

		// Make simdata Sun
		auto sun = SolTrace::Data::make_ray_source<SolTrace::Data::Sun>();

		// Set sun position
		// TODO: Is this valid if using Lat, day, hour?
		sun->set_position(sys->Sun.Origin[0], sys->Sun.Origin[1], sys->Sun.Origin[2]);

        // Set sun shape
        SolTrace::Data::SunShape sun_shape = SolTrace::Data::char_to_sunshape(sys->Sun.ShapeIndex);
        double Sigma = sys->Sun.Sigma;
        double HalfWidth = sys->Sun.Sigma;  // Gaussian and pillbox parameters are stored in Sun.Sigma
        //std::vector<double> user_angle = sys->Sun.SunShapeAngle;
        //std::vector<double> user_intensity = sys->Sun.SunShapeIntensity;
        std::vector<double> user_angle_formatted, user_intensity_formatted;
        convert_user_sun_data(sys->Sun.SunShapeAngle, sys->Sun.SunShapeIntensity,
            user_angle_formatted, user_intensity_formatted);
        sun->set_shape(sun_shape, Sigma, HalfWidth, 0.0, user_angle_formatted, user_intensity_formatted);
        
        // Attach sun to simulation data
        sd.add_ray_source(sun);
	}

	// Stages
    {
        // Loop through stages
        int i_stage = 0;
        for (TStage* stage_legacy : sys->StageList)
        {
            // Make stage
            SolTrace::Data::stage_ptr stage = SolTrace::Data::make_stage(i_stage);
            stage->set_origin(stage_legacy->Origin[0], stage_legacy->Origin[1], stage_legacy->Origin[2]);
            stage->set_aim_vector(stage_legacy->AimPoint[0], stage_legacy->AimPoint[1], stage_legacy->AimPoint[2]);
            stage->set_zrot(stage_legacy->ZRot);
            stage->compute_coordinate_rotations();

            // Check if virtual
            bool is_virtual = stage_legacy->Virtual;
            
            // Add elements to stage
            int i_element = 0;
            for (TElement* el_legacy : stage_legacy->ElementList)
            {
                // Make element
                SolTrace::Data::element_ptr element;
                if (is_virtual == true)
                    element = SolTrace::Data::make_element<SolTrace::Data::VirtualElement>();
                else
                    element = SolTrace::Data::make_element<SolTrace::Data::SingleElement>();

                // Make aperture
                SolTrace::Data::ApertureType aperture_type = SolTrace::Data::char_to_aperture(el_legacy->ShapeIndex);
                if (aperture_type == SolTrace::Data::ApertureType::APERTURE_UNKNOWN)
                    return static_cast<int>(ConversionErrors::APERTURE_ERROR);
                std::vector<double> aperture_args = { el_legacy->ParameterA, el_legacy->ParameterB,
                    el_legacy->ParameterC, el_legacy->ParameterD, el_legacy->ParameterE,
                    el_legacy->ParameterF, el_legacy->ParameterG, el_legacy->ParameterH };
                SolTrace::Data::aperture_ptr aperture_ptr = SolTrace::Data::Aperture::make_aperture_from_type(
                    aperture_type, aperture_args);
                if (aperture_ptr == nullptr)
                    return static_cast<int>(ConversionErrors::APERTURE_ERROR);
                element->set_aperture(aperture_ptr);

                // Make surface
                SolTrace::Data::SurfaceType surface_type = SolTrace::Data::char_to_surface(el_legacy->SurfaceIndex);
                if (surface_type == SolTrace::Data::SurfaceType::SURFACE_UNKNOWN)
                    return static_cast<int>(ConversionErrors::SURFACE_ERROR);
                // TODO: validate surface args (more than just vertexcurv X and Y)
                std::vector<double> surface_args = { el_legacy->VertexCurvX, el_legacy->VertexCurvY };
                SolTrace::Data::surface_ptr surface_ptr = SolTrace::Data::make_surface_from_type(surface_type, surface_args);
                if (surface_ptr == nullptr)
                    return static_cast<int>(ConversionErrors::SURFACE_ERROR);
                element->set_surface(surface_ptr);

                // TODO: move this somewhere more appropriate
                if (surface_type == SolTrace::Data::SurfaceType::CYLINDER)
                {
                    aperture_ptr = element->get_aperture();
                    auto rect = std::dynamic_pointer_cast<SolTrace::Data::Rectangle>(aperture_ptr);
                    auto cyl = std::dynamic_pointer_cast<SolTrace::Data::Cylinder>(surface_ptr);
                    if (rect == nullptr || cyl == nullptr)
                    {
                        throw std::invalid_argument("This should not happen!");
                    }
                    rect->set_x_length(2.0 * cyl->radius);
                    rect->set_x_coord(-1.0 * cyl->radius);
                }

                // Set element position and orientation
                element->set_origin(el_legacy->Origin[0], el_legacy->Origin[1], el_legacy->Origin[2]);
                element->set_aim_vector(el_legacy->AimPoint[0], el_legacy->AimPoint[1], el_legacy->AimPoint[2]);
                element->set_zrot(el_legacy->ZRot);

                // Set optical properties
                if (is_virtual == false)
                {
                    // Convert legacy front/back optics into new OpticalPropertySet
                    auto &front = el_legacy->Optics->Front;
                    auto &back = el_legacy->Optics->Back;

                    SolTrace::Data::InteractionType interaction_type = SolTrace::Data::int_to_interaction(el_legacy->InteractionType);
                    double refrac_front = front.RefractiveIndex[0];
                    double refrac_back = back.RefractiveIndex[0];

                    SolTrace::Data::OpticalPropertySet optics_set(interaction_type, refrac_front, refrac_back);

                    // Front face
                    SolTrace::Data::DistributionType dist_front = SolTrace::Data::char_to_distribution(front.DistributionType);
                    optics_set.set_properties(SolTrace::Data::OpticalSide::Front,
                                              dist_front,
                                              front.Transmissivity,
                                              front.Reflectivity,
                                              front.RMSSlopeError,
                                              front.RMSSpecError);

                    // Back face
                    SolTrace::Data::DistributionType dist_back = SolTrace::Data::char_to_distribution(back.DistributionType);
                    optics_set.set_properties(SolTrace::Data::OpticalSide::Back,
                                              dist_back,
                                              back.Transmissivity,
                                              back.Reflectivity,
                                              back.RMSSlopeError,
                                              back.RMSSpecError);

                    auto ref = sd.add_optical_property_set(optics_set);
                    element->set_optical_property_set(ref);
                }

                // Set element name
                element->set_name(std::to_string(i_element));
                
                // Enable/disable element
                if (el_legacy->Enabled)
                    element->enable();
                else
                    element->disable();

                // Attach to stage
                if (!SolTrace::Data::Element::is_success(stage->add_element(element)))
                    return static_cast<int>(ConversionErrors::ELEMENT_ADD_ERROR);

                i_element++;
            }

            // Add stage to simulation data
            if (!SolTrace::Data::Element::is_success(sd.add_stage(stage)))
                return static_cast<int>(ConversionErrors::STAGE_ADD_ERROR);

            i_stage++;
        }
    }
	
    // Parameters
    {
        SolTrace::Data::SimulationParameters& par = sd.get_simulation_parameters();
        par.number_of_rays = sys->sim_raycount;
        par.max_number_of_rays = sys->sim_raymax;
        par.seed = seed;
        par.include_sun_shape_errors = sys->sim_errors_sunshape;
        par.include_optical_errors = sys->sim_errors_optical;
    }

	return static_cast<int>(ConversionErrors::SUCCESS);
}

int run_native_runner(SolTrace::Data::SimulationData& sd, TSystem* sys, int nthreads)
{
    // Make native runner
    SolTrace::NativeRunner::NativeRunner runner;
    
    // Initialize
    SolTrace::Runner::RunnerStatus sts = runner.initialize();
    
    // Set native runner specific parameters
    if (sys->sim_dynamic_group) runner.enable_power_tower();
    else runner.disable_power_tower();

    // Setup simualtion (convert simulation data to TSystem)
    sts = runner.setup_simulation(&sd);

    // Set number of threads
    runner.set_number_of_threads(nthreads);

    // Set stage parameters (specific to native runner)
    set_tstage_parameters(sys, *runner.get_system());

    // Run simulation
    sts = runner.run_simulation();

    // Collect results
    SolTrace::Result::SimulationResult result;
    sts = runner.report_simulation(&result, 1);

    // Directly using TSystem object to get raydata (for now)
    const SolTrace::NativeRunner::TSystem* tsys_native = runner.get_system();

    // Copy raydata to legacy
    get_raydata_from_native_tsys(sys, *tsys_native);

    return -1;
}

int run_native_file_runner(TSystem* sys, const char* file_name, int nthreads)
{
    // Make simulation data
    SolTrace::Data::SimulationData sd;

    // Load stinput file to simulation data
    bool success = sd.import_from_file(file_name);

    // Make native runner
    SolTrace::NativeRunner::NativeRunner runner;

    // Initialize
    SolTrace::Runner::RunnerStatus sts = runner.initialize();

    // Set native runner specific parameters
    if (sys->sim_dynamic_group) runner.enable_power_tower();
    else runner.disable_power_tower();

    // Setup simualtion (convert simulation data to TSystem)
    sts = runner.setup_simulation(&sd);

    // Set number of threads
    runner.set_number_of_threads(nthreads);

    // Run simulation
    sts = runner.run_simulation();

    // Collect results
    SolTrace::Result::SimulationResult result;
    sts = runner.report_simulation(&result, 1);

    // Directly using TSystem object to get raydata (for now)
    const SolTrace::NativeRunner::TSystem* tsys_native = runner.get_system();

    // Copy raydata to legacy
    get_raydata_from_native_tsys(sys, *tsys_native);

    return 0;
}

int run_optix_runner(SolTrace::Data::SimulationData& sd, TSystem* sys)
{
    OptixRunner runner;
    SolTrace::Runner::RunnerStatus sts = runner.initialize();
    sts = runner.setup_simulation(&sd);
    sts = runner.run_simulation_core();

    std::vector<float4> hp_vec;
    std::vector<uint_fast64_t> raynumber_vec;
    std::vector<int32_t> element_id_vec;
    runner.get_hp_output(hp_vec, raynumber_vec, element_id_vec);

    assign_raydata_from_hitpoints(hp_vec, raynumber_vec, sys);
    
    return 0;
}

int run_embree_runner(SolTrace::Data::SimulationData& sd, TSystem* sys, const int nthreads)
{
    // Make embree runner
    SolTrace::EmbreeRunner::EmbreeRunner runner;

    // Initialize
    SolTrace::Runner::RunnerStatus sts = runner.initialize();

    // Setup simualtion (convert simulation data to TSystem)
    sts = runner.setup_simulation(&sd);

    // Set number of threads
    runner.set_number_of_threads(nthreads);

    // Set stage parameters (specific to native runner)
    set_tstage_parameters(sys, *runner.get_system());

    // Run simulation
    sts = runner.run_simulation();

    // Collect results
    SolTrace::Result::SimulationResult result;
    sts = runner.report_simulation(&result, 1);

    // Directly using TSystem object to get raydata (for now)
    const SolTrace::EmbreeRunner::TSystem* tsys_native = runner.get_system();

    // Copy raydata to legacy
    get_raydata_from_native_tsys(sys, *tsys_native);

    return -1;
}
