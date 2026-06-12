#include "sun.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "json_helpers.hpp"
#include "simdata_io.hpp"

namespace SolTrace::Data {

Sun::Sun(const nlohmann::ordered_json& jnode)
{
    std::string my_shape_string = jnode.at("my_shape");
    this->my_shape = get_enum_from_string(my_shape_string, SunShapeMap, SunShape::UNKNOWN);
    if (my_shape == SunShape::UNKNOWN)
    {
        throw std::runtime_error("Error reading sun shape");
    }

    this->sigma = json_get_double(jnode, "sigma");
    this->half_width = json_get_double(jnode, "half_width");
    this->circumsolar_ratio = json_get_double(jnode, "csr");
    std::vector<double> user_a = jnode.at("user_angle");
    this->user_angle = user_a;
    std::vector<double> user_i = jnode.at("user_intensity");
    this->user_intensity = user_i;
    std::string gen_type_string = jnode.at("gen_type");
    this->my_gen_type = get_enum_from_string(gen_type_string, GenTypeMap, GenType::UNKNOWN);
    if (my_gen_type == GenType::UNKNOWN)
    {
        throw std::runtime_error("Error reading gen type");
    }

    std::array<double, 3> pos = jnode.at("pos").get<std::array<double, 3>>();
    glm::dvec3 pos_vec = from_array(pos);
    this->my_position = pos_vec;
}

void Sun::set_gaussian_distribution(double _sigma)
{
    if (_sigma <= 0.0)
    {
        throw std::invalid_argument("Gaussian distribution sigma must be positive");
    }
    if (std::isnan(_sigma) || std::isinf(_sigma))
    {
        throw std::invalid_argument("Gaussian distribution sigma must be finite");
    }

    sigma = _sigma;
}

void Sun::set_pillbox_distribution(double _half_width)
{
    if (_half_width <= 0.0)
    {
        throw std::invalid_argument("Pillbox distribution half_width must be positive");
    }
    if (std::isnan(_half_width) || std::isinf(_half_width))
    {
        throw std::invalid_argument("Pillbox distribution half_width must be finite");
    }

    half_width = _half_width;
}

void Sun::set_buie_csr_distribution(double _csr)
{
    if (_csr < 0.0 || _csr > 0.8)
    {
        throw std::invalid_argument("Buie CSR must be in the range [0, 0.8]");
    }
    if (std::isnan(_csr) || std::isinf(_csr))
    {
        throw std::invalid_argument("Buie CSR must be finite");
    }
    circumsolar_ratio = _csr;
}

void Sun::calculate_buie_parameters(double& kappa, double& gamma)
{
    // Calculate kappa and gamma parameters
    // Creates the Buie (2003) sun shape based on CSR
    // [1] Buie, D., Dey, C., & Bosi, S. (2003). The effective size of the solar cone for solar concentrating systems. Solar energy, 74(2003), 417-427.
    // [2] Buie, D., Monger, A., & Dey, C. (2003). Sun shape distributions for terrestrial solar simulations. Solar Energy, 74(March 2003), 113-122.
    double csr = this->get_circumsolar_ratio();
    double chi;
    if (csr > 0.145)
        chi = -0.04419909985804843 + csr * (1.401323894233574 + csr * (-0.3639746714505299 + csr * (-0.9579768560161194 + 1.1550475450828657 * csr)));
    else if (csr > 0.035)
        chi = 0.022652077593662934 + csr * (0.5252380349996234 + (2.5484334534423887 - 0.8763755326550412 * csr) * csr);
    else
        chi = 0.004733749294807862 + csr * (4.716738065192151 + csr * (-463.506669149804 + csr * (24745.88727411664 + csr * (-606122.7511711778 + 5521693.445014727 * csr))));
    
    kappa = 0.9 * log(13.5 * chi) * pow(chi, -0.3);
    gamma = 2.2 * log(0.52 * chi) * pow(chi, 0.43) - 0.1;
}

void Sun::set_user_defined_distribution(std::vector<double> _user_angle,
                                        std::vector<double> _user_intensity)
{
    if (_user_angle.empty())
    {
        throw std::invalid_argument("User-defined distribution requires non-empty angle vector");
    }
    if (_user_intensity.empty())
    {
        throw std::invalid_argument("User-defined distribution requires non-empty intensity vector");
    }
    if (_user_angle.size() != _user_intensity.size())
    {
        throw std::invalid_argument("User-defined distribution angle and intensity vectors must have same size");
    }

    // Check for valid angle values
    for (const auto &angle : _user_angle)
    {
        if (std::isnan(angle) || std::isinf(angle))
        {
            throw std::invalid_argument("User-defined distribution angles must be finite");
        }
        if (angle < 0.0)
        {
            throw std::invalid_argument("User-defined distribution angles must be non-negative");
        }
    }

    // Check for valid intensity values
    for (const auto &intensity : _user_intensity)
    {
        if (std::isnan(intensity) || std::isinf(intensity))
        {
            throw std::invalid_argument("User-defined distribution intensities must be finite");
        }
        if (intensity < 0.0)
        {
            throw std::invalid_argument("User-defined distribution intensities must be non-negative");
        }
    }

    // Check that angles are in ascending order
    if (!std::is_sorted(_user_angle.begin(), _user_angle.end()))
    {
        throw std::invalid_argument("User-defined distribution angles must be in ascending order");
    }

    user_angle = std::move(_user_angle);
    user_intensity = std::move(_user_intensity);
}

void Sun::set_shape(SunShape shape,
                    double _sigma,
                    double _half_width,
                    double _csr,
                    std::vector<double> _user_angle,
                    std::vector<double> _user_intensity)
{
    this->my_shape = shape;

    // Clear arguments
    sigma = std::numeric_limits<double>::quiet_NaN();
    half_width = std::numeric_limits<double>::quiet_NaN();
    circumsolar_ratio = std::numeric_limits<double>::quiet_NaN();
    user_angle.clear();
    user_intensity.clear();

    switch (shape)
    {
    case (SunShape::GAUSSIAN):
        set_gaussian_distribution(_sigma);
        break;
    case (SunShape::PILLBOX):
        set_pillbox_distribution(_half_width);
        break;
    case (SunShape::LIMBDARKENED):
        break;
    case (SunShape::BUIE_CSR):
        set_buie_csr_distribution(_csr);
        break;
    case (SunShape::USER_DEFINED):
        set_user_defined_distribution(std::move(_user_angle), std::move(_user_intensity));
        break;
    default:
        throw std::invalid_argument("Unknown distribution type");
        break;
    }

    return;
}

double Sun::get_max_sun_angle(double gaussian_coverage) const   // [mrad]
{
    switch (this->my_shape)
    {
        case (SunShape::PILLBOX):
            return this->half_width;
        case (SunShape::GAUSSIAN):
        {
            // Returns maximum angle given the desired coverage area
            // With 100% coverage area, r is inf
            if (std::isnan(gaussian_coverage) || std::isinf(gaussian_coverage) ||
                gaussian_coverage <= 0.0 || gaussian_coverage >= 1.0)
            {
                throw std::runtime_error(
                    "Sun::get_max_sun_angle: coverage fraction must be in (0, 1) for gaussian sunshape");
            }
            if (std::isnan(this->sigma) || std::isinf(this->sigma) || this->sigma <= 0.0)
            {
                throw std::runtime_error(
                    "Sun::get_max_sun_angle: invalid sigma for gaussian sunshape");
            }
            const double r = this->sigma * std::sqrt(-2.0 * std::log(1.0 - gaussian_coverage));
            return r;
        }
        case (SunShape::LIMBDARKENED):
            return 4.65;    // Will need to update if we make this user input
        case (SunShape::BUIE_CSR):
            return 43.6;    // Constant
        case (SunShape::USER_DEFINED):
        {
            if (this->user_angle.empty())
            {
                throw std::runtime_error("Sun::get_max_sun_angle: user-defined sun shape has no angles");
            }
            return *std::max_element(this->user_angle.begin(), this->user_angle.end());
        }
        default:
        {
            throw std::runtime_error("Sun::get_max_sun_angle: sun shape not currently supported");
        }
    }
}

double Sun::get_max_intensity() const
{
    switch (this->my_shape)
    {
        case (SunShape::USER_DEFINED):
        {
            if (this->user_intensity.empty())
            {
                throw std::runtime_error("Sun::get_max_intensity: user-defined sun shape has no intensities");
            }
            return *std::max_element(this->user_intensity.begin(), this->user_intensity.end());
        }  
        default:
            return 1;
    }
}

void Sun::write_json(nlohmann::ordered_json& jnode)
{
    jnode["source_type"] = "Sun";
    std::string shape_string = SolTrace::Data::SunShapeMap.at(this->my_shape);
    jnode["my_shape"] = shape_string;
    jnode["sigma"] = this->sigma;
    jnode["half_width"] = this->half_width;
    jnode["csr"] = this->circumsolar_ratio;
    jnode["user_angle"] = this->user_angle;
    jnode["user_intensity"] = this->user_intensity;
    jnode["gen_type"] = SolTrace::Data::GenTypeMap.at(this->my_gen_type);
    jnode["pos"] = to_array(this->my_position);
}

} // namespace SolTrace::Data
