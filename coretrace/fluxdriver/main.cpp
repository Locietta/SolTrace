/**
 * @file main.cpp
 * @brief OptiX flux-map driver for cylindrical-receiver tower fields.
 *
 * Runs the OptiX runner in chunks (re-seeding between chunks so they are
 * statistically independent) and bins receiver hits into a cylinder-surface
 * flux map on the fly, so multi-billion-ray budgets never materialize as ray
 * records on the host all at once.
 *
 * Only sun->mirror->receiver paths (receiver hit at depth 2 following a
 * reflection at depth 1) are binned: direct sun-on-receiver hits and
 * multi-bounce paths are excluded for parity with forward MCRT codes that
 * start rays on the mirrors. Optionally applies the standard clear-day
 * atmospheric attenuation per ray using the mirror-to-receiver distance,
 * matching RMCRT / diffspt.
 *
 * Usage:
 *   fluxdriver <input.json> --out flux.npy --meta meta.json --total N
 *              [--chunk M] [--dni W_M2] [--grid COLS,ROWS] [--atten 0|1]
 *              [--max-factor F] [--seed S]
 *
 * The receiver is auto-detected as the single element with a CYLINDER
 * surface; the map convention is rows from the receiver bottom up and
 * columns from compass North winding N->E->S->W (frame: x=East, y=Up,
 * z=South), i.e. col = atan2(x-cx, -(z-cz)) / 2pi * ncols.
 */

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "simulation_data_export.hpp"
#include "optix_runner.hpp"
#include "core/soltrace_constants.h"

namespace {

struct Options {
    std::string input_file;
    std::string out_npy;
    std::string out_meta;
    uint64_t total_rays = 0;
    uint64_t chunk_rays = 20000000ULL;
    double dni = 1000.0;
    int grid_cols = 0; // 0 -> derive from receiver size at 0.1 m
    int grid_rows = 0;
    bool attenuation = true;
    uint64_t max_factor = 200; // max launched rays per run = chunk * factor
    uint64_t seed = 0;         // 0 -> use seed from the JSON
};

double eta_atmospheric(double d) {
    if (d <= 1000.0)
        return 0.99331 - 0.0001176 * d + 1.97e-8 * d * d;
    return std::exp(-0.0001106 * d);
}

void write_npy_float32(const std::string &path, const std::vector<double> &data, int rows, int cols) {
    std::string header = "{'descr': '<f4', 'fortran_order': False, 'shape': (" +
                         std::to_string(rows) + ", " + std::to_string(cols) + "), }";
    size_t header_len = header.size() + 1; // newline
    size_t total = 10 + header_len;
    size_t padded = (total + 63) / 64 * 64;
    header.append(padded - total, ' ');
    header.push_back('\n');

    std::ofstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("cannot open output file: " + path);
    const char magic[] = "\x93NUMPY\x01\x00";
    f.write(magic, 8);
    uint16_t hlen = static_cast<uint16_t>(header.size());
    f.write(reinterpret_cast<const char *>(&hlen), 2);
    f.write(header.data(), header.size());
    std::vector<float> out(data.size());
    for (size_t i = 0; i < data.size(); ++i)
        out[i] = static_cast<float>(data[i]);
    f.write(reinterpret_cast<const char *>(out.data()), out.size() * sizeof(float));
}

bool parse_args(int argc, char *argv[], Options &opt) {
    if (argc < 2)
        return false;
    opt.input_file = argv[1];
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char *name) -> std::string {
            if (i + 1 >= argc)
                throw std::runtime_error(std::string("missing value for ") + name);
            return argv[++i];
        };
        if (a == "--out")
            opt.out_npy = next("--out");
        else if (a == "--meta")
            opt.out_meta = next("--meta");
        else if (a == "--total")
            opt.total_rays = std::stoull(next("--total"));
        else if (a == "--chunk")
            opt.chunk_rays = std::stoull(next("--chunk"));
        else if (a == "--dni")
            opt.dni = std::stod(next("--dni"));
        else if (a == "--grid") {
            std::string v = next("--grid");
            if (std::sscanf(v.c_str(), "%d,%d", &opt.grid_cols, &opt.grid_rows) != 2)
                throw std::runtime_error("--grid expects COLS,ROWS");
        } else if (a == "--atten")
            opt.attenuation = std::stoi(next("--atten")) != 0;
        else if (a == "--max-factor")
            opt.max_factor = std::stoull(next("--max-factor"));
        else if (a == "--seed")
            opt.seed = std::stoull(next("--seed"));
        else
            throw std::runtime_error("unknown option " + a);
    }
    return !opt.out_npy.empty() && opt.total_rays > 0;
}

} // namespace

int main(int argc, char *argv[]) {
    Options opt;
    try {
        if (!parse_args(argc, argv, opt)) {
            std::cerr << "Usage: fluxdriver <input.json> --out flux.npy --total N [--meta meta.json]\n"
                         "                  [--chunk M] [--dni X] [--grid COLS,ROWS] [--atten 0|1]\n"
                         "                  [--max-factor F] [--seed S]\n";
            return EXIT_FAILURE;
        }
    } catch (const std::exception &e) {
        std::cerr << "Argument error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    SimulationData simData;
    std::cout << "Loading " << opt.input_file << "...\n";
    auto t_load0 = std::chrono::steady_clock::now();
    simData.import_json_file(opt.input_file);
    double load_s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t_load0).count();
    std::cout << "  loaded " << simData.get_number_of_elements() << " elements in " << load_s << " s\n";

    // Locate the (single) cylinder receiver element and its dimensions.
    int32_t receiver_id = OptixCSP::kElementIdUnassigned;
    double recv_cx = 0, recv_cy = 0, recv_cz = 0, recv_R = 0, recv_H = 0;
    for (auto iter = simData.get_const_iterator(); !simData.is_at_end(iter); ++iter) {
        element_ptr el = iter->second;
        if (!el->is_enabled() || !el->is_single() || el->get_surface() == nullptr)
            continue;
        if (el->get_surface()->get_type() == SurfaceType::CYLINDER) {
            if (receiver_id != OptixCSP::kElementIdUnassigned)
                throw std::runtime_error("multiple cylinder elements found; expected exactly one receiver");
            receiver_id = static_cast<int32_t>(el->get_id());
            auto cyl = std::dynamic_pointer_cast<Cylinder>(el->get_surface());
            auto rect = std::dynamic_pointer_cast<Rectangle>(el->get_aperture());
            auto origin = el->get_origin_global();
            recv_cx = origin.x;
            recv_cy = origin.y;
            recv_cz = origin.z;
            recv_R = cyl->radius;
            recv_H = rect ? rect->y_length() : 0.0;
        }
    }
    if (receiver_id == OptixCSP::kElementIdUnassigned)
        throw std::runtime_error("no cylinder receiver element found");
    if (opt.grid_cols <= 0 || opt.grid_rows <= 0) {
        opt.grid_cols = static_cast<int>(std::ceil(2.0 * PI * recv_R / 0.1));
        opt.grid_rows = static_cast<int>(std::ceil(recv_H / 0.1));
    }
    std::cout << "Receiver element id " << receiver_id << ": R=" << recv_R << " H=" << recv_H
              << " center=(" << recv_cx << "," << recv_cy << "," << recv_cz << ")"
              << " grid " << opt.grid_cols << "x" << opt.grid_rows << "\n";

    OptixRunner runner;
    if (runner.initialize() != SolTrace::Runner::RunnerStatus::SUCCESS) {
        std::cerr << "Error: OptiX runner init failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Setting up OptiX scene...\n";
    auto t_setup0 = std::chrono::steady_clock::now();
    if (runner.setup_simulation(&simData) != SolTrace::Runner::RunnerStatus::SUCCESS) {
        std::cerr << "Error: OptiX setup failed\n";
        return EXIT_FAILURE;
    }
    double setup_s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t_setup0).count();
    std::cout << "  setup in " << setup_s << " s\n";

    OptixCSP::SolTraceSystem *sys = runner.get_optix_system();
    const uint64_t base_seed = opt.seed != 0 ? opt.seed : static_cast<uint64_t>(simData.get_simulation_parameters().seed);

    // Each chunk is an independent estimate of the same flux map; chunks are
    // combined as a weighted average (weight = actual hit rays in the chunk).
    std::vector<double> energy(static_cast<size_t>(opt.grid_cols) * opt.grid_rows, 0.0);
    std::vector<double> chunk_energy(energy.size(), 0.0);
    const double two_pi = 2.0 * PI;
    const double y0 = recv_cy - 0.5 * recv_H;

    std::vector<double> chunk_trace_s;
    uint64_t total_launched = 0, total_hit = 0, total_binned = 0;
    double weight_sum = 0.0;
    uint64_t remaining = opt.total_rays;
    int n_chunks = static_cast<int>((opt.total_rays + opt.chunk_rays - 1) / opt.chunk_rays);
    int chunk_index = 0;

    while (remaining > 0) {
        const uint64_t n = std::min<uint64_t>(remaining, opt.chunk_rays);
        sys->set_runtime_seed(base_seed + 0x9E3779B97F4A7C15ULL * static_cast<uint64_t>(chunk_index + 1));
        sys->set_number_of_rays(n, n * opt.max_factor);

        auto t0 = std::chrono::steady_clock::now();
        if (runner.run_simulation() != SolTrace::Runner::RunnerStatus::SUCCESS) {
            std::cerr << "Error: simulation failed at chunk " << chunk_index << "\n";
            return EXIT_FAILURE;
        }
        double trace_s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        chunk_trace_s.push_back(trace_s);

        const uint64_t n_sun = runner.get_N_sun_rays();
        const uint64_t n_hit = runner.get_number_rays_traced();
        const double sun_area = runner.get_sun_plane_area();
        const double ppr = opt.dni * sun_area / static_cast<double>(n_sun);
        if (chunk_index == 0)
            std::cout << "  sun plane area: " << sun_area << " m^2, ppr(chunk0): " << ppr << " W\n";
        total_launched += n_sun;
        total_hit += n_hit;
        std::fill(chunk_energy.begin(), chunk_energy.end(), 0.0);

        // Walk compacted hit records: each ray group = CREATE followed by hits.
        const auto &recs = sys->get_hit_records();
        float px = 0, py = 0, pz = 0;
        int prev_depth = -1;
        int32_t prev_elem = OptixCSP::kElementIdUnassigned;
        for (const auto &r : recs) {
            const int depth = static_cast<int>(r.hit_point.x + 0.5f);
            if (r.hit_type == OptixCSP::HitType::HIT_CREATE) {
                px = r.hit_point.y;
                py = r.hit_point.z;
                pz = r.hit_point.w;
                prev_depth = 0;
                prev_elem = OptixCSP::kElementIdRayGen;
                continue;
            }
            const float x = r.hit_point.y, y = r.hit_point.z, z = r.hit_point.w;
            if (r.element_id == receiver_id && depth == 2 && prev_depth == 1 && prev_elem != receiver_id) {
                double w = ppr;
                if (opt.attenuation) {
                    const double dx = x - px, dy = y - py, dz = z - pz;
                    w *= eta_atmospheric(std::sqrt(dx * dx + dy * dy + dz * dz));
                }
                double u = std::atan2(static_cast<double>(x) - recv_cx, -(static_cast<double>(z) - recv_cz)) / two_pi;
                u -= std::floor(u);
                int col = std::min(static_cast<int>(u * opt.grid_cols), opt.grid_cols - 1);
                double v = (static_cast<double>(y) - y0) / recv_H;
                int row = std::min(std::max(static_cast<int>(v * opt.grid_rows), 0), opt.grid_rows - 1);
                chunk_energy[static_cast<size_t>(row) * opt.grid_cols + col] += w;
                ++total_binned;
            }
            px = x;
            py = y;
            pz = z;
            prev_depth = depth;
            prev_elem = r.element_id;
        }

        if (n_hit > 0) {
            const double w_chunk = static_cast<double>(n_hit);
            for (size_t i = 0; i < energy.size(); ++i)
                energy[i] += w_chunk * chunk_energy[i];
            weight_sum += w_chunk;
        }

        ++chunk_index;
        remaining -= n;
        std::cout << "  chunk " << chunk_index << "/" << n_chunks << ": " << n_hit << " hit rays ("
                  << n << " requested), " << n_sun << " launched, trace " << trace_s << " s\n";
    }

    if (weight_sum > 0.0)
        for (auto &e : energy)
            e /= weight_sum;

    // Convert binned energy (W per pixel) to flux density (W/m^2).
    const double pixel_area = (two_pi * recv_R / opt.grid_cols) * (recv_H / opt.grid_rows);
    double total_power = 0.0;
    for (auto &e : energy) {
        total_power += e;
        e /= pixel_area;
    }

    write_npy_float32(opt.out_npy, energy, opt.grid_rows, opt.grid_cols);
    double trace_total = 0.0;
    for (double t : chunk_trace_s)
        trace_total += t;
    std::cout << "Receiver power: " << total_power / 1000.0 << " kW, binned rays: " << total_binned
              << ", trace total " << trace_total << " s\n";

    if (!opt.out_meta.empty()) {
        std::ofstream m(opt.out_meta);
        m << "{\n"
          << "  \"total_rays_requested\": " << opt.total_rays << ",\n"
          << "  \"chunk_rays\": " << opt.chunk_rays << ",\n"
          << "  \"n_chunks\": " << chunk_index << ",\n"
          << "  \"rays_launched\": " << total_launched << ",\n"
          << "  \"rays_hit\": " << total_hit << ",\n"
          << "  \"rays_binned\": " << total_binned << ",\n"
          << "  \"receiver_power_kw\": " << total_power / 1000.0 << ",\n"
          << "  \"setup_s\": " << setup_s << ",\n"
          << "  \"load_s\": " << load_s << ",\n"
          << "  \"trace_total_s\": " << trace_total << ",\n"
          << "  \"attenuation\": " << (opt.attenuation ? "true" : "false") << ",\n"
          << "  \"grid\": [" << opt.grid_rows << ", " << opt.grid_cols << "],\n"
          << "  \"chunk_trace_s\": [";
        for (size_t i = 0; i < chunk_trace_s.size(); ++i)
            m << (i ? ", " : "") << chunk_trace_s[i];
        m << "]\n}\n";
    }

    std::cout << "Done.\n";
    return EXIT_SUCCESS;
}
