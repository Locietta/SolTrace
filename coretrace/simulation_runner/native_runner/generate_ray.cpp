#include "generate_ray.hpp"

#include "simulation_data_export.hpp"

namespace SolTrace::NativeRunner {

double halton(uint_fast64_t index, uint_fast64_t base)
{
	double f = 1.0;
	double result = 0;
	while (index > 0)
	{
		f = f / base;
		result = result + f * (index % base);
		index = index / base;
	}
	return result;
}

void GenerateRay(
    MTRand& myrng,
    const glm::dvec3& PosSunStage,
    glm::dvec3& Origin,
    glm::dmat3& RLocToRef,
    TSun* Sun,
	uint_fast64_t sample_index,
    glm::dvec3& PosRayGlobal,
    glm::dvec3& CosRayGlobal,
    glm::dvec3& PosRaySun,
	int& ErrorFlag)
{
	/*{This procedure generates a randomly located ray in the x-y plane of the sun coordinate system in
	 the z direction of the sun coord. system, checks to see that the ray is within the region of interest
	 defined by the spatial extent of the elements of Stage as seen from the sun
	 and ultimately transforms that ray to the global coord. system.   The z-axis of the sun coord. system points
	 towards the Stage coord. system origin.

	 Input
		   - Seed = Seed for random number generator
		   - Sun = Sun data record of type TSun
		   - Origin = Primary Stage origin
		   - RLocToRef = transformation matrix from local to reference frame
	 Output
		   - PosRayGlobal = Position of ray in Global coordinate system
		   - CosRayGlobal = Direction cosines of ray in Global coordinate system} */

	double XRaySun = 0.0, YRaySun = 0.0, ZRaySun = 0.0;
    glm::dvec3 CosRaySun(0.0, 0.0, 0.0);
    glm::dvec3 PosRayStage(0.0, 0.0, 0.0);
    glm::dvec3 CosRayStage(0.0, 0.0, 0.0);
	int NegPosSign = 0;
	ErrorFlag = 0;
    PosRaySun = glm::dvec3(0.0);

    // ZRaySun := 0.0;  //Origin of rays in xy plane of sun coord system.
    ZRaySun = -10000.0; // changed 5/1/00.  rays originate from well bebind the sun coordinate system xy
                        //  plane which has been translated to primary stage origin.         This value has been reduced signficantly because of numerical issues in tracing rays from sun
                        //  to the closer form solution for a cylinder.  It used to 1e6 and has been reduced to 1e4, which should still be sufficient.   10-26-09 Wendelin

    //{Generate random rays inside of region of interest or from point source}

    if (Sun->PointSource) // fixed this on 3-18-13
    {
        PosRayGlobal = Sun->Origin;

        if (myrng() <= 0.5)
			NegPosSign = -1;
		else
			NegPosSign = 1;

		CosRayGlobal[0] = NegPosSign * myrng(); // random direction for x part of ray vector

		if (myrng() <= 0.5)
			NegPosSign = -1;
		else
			NegPosSign = 1;

		CosRayGlobal[1] = NegPosSign * myrng(); // random direction for y part of ray vector

		if (myrng() <= 0.5)
			NegPosSign = -1;
		else
			NegPosSign = 1;

		CosRayGlobal[2] = NegPosSign * myrng(); // random direction for z part of ray vector

        double CosRayGMag = glm::length(CosRayGlobal);

		CosRayGlobal /= CosRayGMag;
	}
	else
	{
		switch (Sun->GenTypeIndex)
		{
			case(SolTrace::Data::GenType::RANDOM):
			{
				// following changed on 09/26/05 to more efficiently generate rays relative to element center of mass in primary stage
				/*{XRaySun := 2.0*MaxRad*ran3(Seed) - MaxRad;  //ran3 produces results independent of platform.
				YRaySun := 2.0*MaxRad*ran3(Seed) - MaxRad;
				if (XRaySun*XRaySun + YRaySun*YRaySun) > MaxRad*MaxRad then goto GENRAY;
				XRaySun := Xcm + XRaySun;  //adjust location of generated rays about element center of mass
				YRaySun := Ycm + YRaySun;}*/

				XRaySun = Sun->MinXSun + (Sun->MaxXSun - Sun->MinXSun) * myrng(); // uses a rectangular region of interest about the primary
				YRaySun = Sun->MinYSun + (Sun->MaxYSun - Sun->MinYSun) * myrng(); // stage. Added 09/26/05

				// std::cout << "MinXSun: " << Sun->MinXSun
				// 		  << "\nMaxXSun: " << Sun->MaxXSun
				// 		  << "\nMinYSun: " << Sun->MinYSun
				// 		  << "\nMaxYSun: " << Sun->MaxYSun
				// 		  << "\nXRaySun: " << XRaySun
				// 		  << "\nYRaySun:" << YRaySun
				// 		  << "\nR1: " << (XRaySun - Sun->MinXSun) / (Sun->MaxXSun - Sun->MinXSun)
				// 		  << "\nR2: " << (YRaySun - Sun->MinYSun) / (Sun->MaxYSun - Sun->MinXSun)
				// 		  << std::endl;

				//{Offload ray location and direction cosines into sun array}
				PosRaySun = {XRaySun, YRaySun, ZRaySun};
				CosRaySun = {0.0, 0.0, 1.0};
				break;
			}
			case(SolTrace::Data::GenType::HALTON):
			{
				const double u = halton(sample_index, 2);
				const double v = halton(sample_index, 3);

				XRaySun = Sun->MinXSun + (Sun->MaxXSun - Sun->MinXSun) * u;
				YRaySun = Sun->MinYSun + (Sun->MaxYSun - Sun->MinYSun) * v;

				//{Offload ray location and direction cosines into sun array}
				PosRaySun = {XRaySun, YRaySun, ZRaySun};
				CosRaySun = {0.0, 0.0, 1.0};
				break;
			}
			default:
			{
				ErrorFlag = -1;	// TODO: Assign specific int to errorflag?
			}
		}

		

        //{Transform ray locations and dir cosines into Stage system}
        Data::TransformToReference(PosRaySun,
                                   CosRaySun,
                                   PosSunStage,
                                   Sun->RLocToRef,
                                   PosRayStage,
                                   CosRayStage);

        //{Transform ray locations and dir cosines into global system}
        Data::TransformToReference(PosRayStage,
                                   CosRayStage,
                                   Origin,
                                   RLocToRef,
                                   PosRayGlobal,
                                   CosRayGlobal);
    }

    return;
}

} // namespace SolTrace::NativeRunner
