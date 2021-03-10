#include <iostream>
#include <vector>
#include <iterator>
#include <sstream>
#include <fstream>
#include <functional>

void count_lines_and_dimensions(const std::string &in_file, std::size_t &n_elem, std::size_t &n_dim) noexcept {
    std::ifstream is(in_file);
    std::string line, buf;
    int cnt = 0;
    n_dim = 0;
    while (std::getline(is, line)) {
        if (n_dim == 0) {
            std::istringstream iss(line);
            std::vector<std::string> results(std::istream_iterator<std::string>{iss},
                    std::istream_iterator<std::string>());
            n_dim = results.size();
        }
        ++cnt;
    }
    n_elem = cnt;
    is.close();
}

std::streampos get_file_size(std::string const &in_file) {
    std::streampos fsize = 0;
    std::ifstream file(in_file, std::ios::in | std::ios::binary);

    fsize = file.tellg();
    file.seekg(0, std::ios::end);
    fsize = file.tellg() - fsize;
    file.close();

    return fsize;
}

/*
void read_input_hdf5(const std::string &in_file, h_vec<float> &v_points, int &n_coord, int &n_dim,
        int const n_nodes, int i_node) noexcept {
#ifdef HDF5_ON
    //        std::cout << "HDF5 Reading Start: " << std::endl;


        // TODO H5F_ACC_RDONLY ?
        hid_t file = H5Fopen(in_file.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
//        hid_t dset = H5Dopen1(file, "DBSCAN");
//        hid_t dset = H5Dopen1(file, i_node < (n_nodes / 2)? "xyz_1" : "xyz_2");
        hid_t dset = H5Dopen1(file, "xyz");
        hid_t fileSpace= H5Dget_space(dset);

        // Read dataset size and calculate chunk size
        hsize_t count[2];
        H5Sget_simple_extent_dims(fileSpace, count,NULL);
        n_coord = count[0];
        n_dim = count[1];
//        std::cout << "HDF5 total size: " << n_coord << std::endl;

//        hsize_t block_size =  get_block_size(i_node >= (n_nodes / 2)? i_node - (n_nodes / 2): i_node, n_coord, n_nodes / 2);
//        hsize_t block_offset =  get_block_offset(i_node >= (n_nodes / 2)? i_node - (n_nodes / 2): i_node, n_coord, n_nodes / 2);
        hsize_t block_size =  get_block_size(i_node, n_coord, n_nodes);
        hsize_t block_offset =  get_block_offset(i_node, n_coord, n_nodes);
//        std::cout << "i_node: " << i_node << " offset:size " << block_offset << " : " <<  block_size << std::endl;
        hsize_t offset[2] = {block_offset, 0};
        count[0] = block_size;
        v_points.resize(block_size * n_dim);

        hid_t memSpace = H5Screate_simple(2, count, NULL);
        H5Sselect_hyperslab(fileSpace,H5S_SELECT_SET,offset, NULL, count, NULL);
        H5Dread(dset, H5T_IEEE_F32LE, memSpace, fileSpace,H5P_DEFAULT, &v_points[0]);

        H5Dclose(dset);
        H5Fclose(file);

#endif
}
*/

bool read_bin(std::string const &in_file, std::vector<float> &v_data, std::size_t &n_coord, std::size_t &n_dim) noexcept {
    std::ifstream ifs(in_file, std::ios::in | std::ifstream::binary);
    if (!ifs.is_open())
        return false;
    int coords, dims;
    ifs.read((char *) &coords, sizeof(int));
    ifs.read((char *) &dims, sizeof(int));
    n_coord = static_cast<std::size_t>(coords);
    n_dim = static_cast<std::size_t>(dims);
    std::cout << "read bin coords: " << coords << " dim: " << dims << std::endl;
    auto feature_offset = 2 * sizeof(int);
    v_data.resize(n_coord * n_dim);
    ifs.seekg(feature_offset, std::istream::beg);
    ifs.read((char *) &v_data[0], n_coord * n_dim * sizeof(float));
    ifs.close();
    return true;
}


bool read_el(std::string const &in_file, std::vector<float> &v_data, std::size_t n_dim, int const sample_rate) noexcept {
    // first pass to find highest dimension
    std::ifstream pre(in_file, std::ifstream::in);
    if (!pre.is_open())
        return false;
    std::string line, buf, first, second;
    std::stringstream ss;
    std::string::size_type sz;
    n_dim = 0;
    int n_lines = 0;

    while (std::getline(pre, line)) {
        ++n_lines;
        ss.str(std::string());
        ss.clear();
        ss << line;
        while (ss >> buf) {
            auto pos = buf.find(':');
            if (pos != std::string::npos) {
                first = buf.substr(0, pos);
                second = buf.substr(pos+1);
                int val = std::stoi(first, &sz);
                if (val > n_dim) {
                    n_dim = val;
                }
            }
        }
    }
    // add the class label
    ++n_dim;
    v_data.resize(n_lines * n_dim, 0);

    std::ifstream is(in_file, std::ifstream::in);
    if (!is.is_open())
        return false;

    int read_head = 0;
    n_lines = 0;
    while (std::getline(is, line)) {
        if (read_head++ % sample_rate > 0)
            continue;
        ss.str(std::string());
        ss.clear();
        ss << line;
        while (ss >> buf) {
            auto pos = buf.find(':');
            if (pos != std::string::npos) {
                first = buf.substr(0, pos);
                second = buf.substr(pos+1);
                int val = std::stoi(first, &sz);
                v_data[n_lines*n_dim+val] = static_cast<float>(atof(second.c_str()));
            } else {
                // class label
                float label = static_cast<float>(atof(buf.c_str()));
                if (label == 0)
                    label = -1;
                v_data[n_lines*n_dim] = label;
            }
        }
        ++n_lines;
    }
    is.close();
    return true;
}

bool read_csv(std::string const &in_file, std::vector<float> &v_data, std::size_t const n_dim, int const sample_rate) noexcept {
    std::ifstream is(in_file, std::ifstream::in);
    if (!is.is_open())
        return false;
    std::string line, buf;
    std::stringstream ss;
    int read_head = 0;

    while (std::getline(is, line)) {
        if (read_head++ % sample_rate > 0)
            continue;
        ss.str(std::string());
        ss.clear();
        ss << line;
        for (int j = 0; j < n_dim; j++) {
            ss >> buf;
            v_data.push_back(static_cast<float>(atof(buf.c_str())));
        }
    }
    is.close();
    return true;
}

bool write_csv(std::string const &out_file, std::vector<float> const &v_data, std::size_t const n_elem, std::size_t const n_dim) {
    std::ofstream os(out_file, std::ios::out);
    if (!os.is_open()) {
        return false;
    }
    for (std::size_t i = 0; i < n_elem; ++i) {
        for (std::size_t j = 0; j < n_dim; ++j) {
            os << v_data[i * n_dim + j] << " ";
        }
        os << std::endl;
    }
    os.flush();
    os.close();
    return true;
}

bool write_bin(std::string const &out_file, std::vector<float> const &v_data, std::size_t const n_elem, std::size_t const n_dim) {
    std::ofstream os(out_file, std::ios::out | std::ofstream::binary);
    if (!os.is_open()) {
        return false;
    }

    int elem = n_elem;
    int dims = n_dim;
    os.write(reinterpret_cast<const char *>(&elem), sizeof(int));
    os.write(reinterpret_cast<const char *>(&dims), sizeof(int));
    os.write((char *) &v_data[0], elem * dims * sizeof(float));
    os.flush();
    os.close();
    return true;
}

void print_help() {
    std::cout << std::endl << std::endl;
    std::cout << "Usage: [executable] -i [input file] -o [output file] -s [sample rate]" << std::endl;
    std::cout << "    -s [sample rate] : Optional parameter to subsample 1 in [s] points, e.g. s=2 selects every other element" << std::endl;
    std::cout << "Supported Input Types:" << std::endl;
    std::cout << ".csv: Text file with one sample/point per line and features/dimensions separated by a space delimiter" << std::endl;
    std::cout << ".bin: Custom binary file format" << std::endl;
    std::cout << ".h5: HDF5 file format" << std::endl;
}

int main(int argc, char** argv) {

    std::string input_file;
    std::string output_file;
    int sample_rate = 1;

    if (argc < 5 || argc % 2 == 0) {
        std::cerr << "Wrong number of arguments" << std::endl;
        print_help();
        exit(0);
    }

    for (int i = 1; i < argc; i += 2) {
        std::string str(argv[i]);
        if (str == "-i") {
            input_file = argv[i+1];
        } else if (str == "-o") {
            output_file = argv[i+1];
        } else if (str == "-s") {
            sample_rate = std::stoi(argv[i+1]);
        }
    }

    std::cout << "Input file: " << input_file << std::endl;
    std::cout << "Output file: " << output_file << std::endl;

    std::vector<float> v_data;
    std::size_t n_elem, n_dim;
    if (input_file.find(".csv") != std::string::npos) {
        count_lines_and_dimensions(input_file, n_elem, n_dim);
        std::cout << "Found " << n_elem << " elements" << " and " << n_dim << " dimensions" << std::endl;
        std::cout << "Reading csv data.." << std::flush;
        if (!read_csv(input_file, v_data, n_dim, sample_rate)) {
            std::cout << std::endl << "Input file could not be opened, does it exist ?" << std::endl;
            exit(-1);
        }
        n_elem /= sample_rate;
        std::cout << " done!" << std::endl;
    } else if (input_file.find(".bin") != std::string::npos) {
        std::vector<float> v_data_all;
        std::cout << "Reading bin data.." << std::flush;
        if (!read_bin(input_file, v_data_all, n_elem, n_dim)) {
            std::cout << std::endl << "Input file could not be opened, does it exist ?" << std::endl;
            exit(-1);
        }
        std::cout << " done!" << std::endl;
        std::cout << "Found " << n_elem << " elements" << " and " << n_dim << " dimensions" << std::endl;
        for (std::size_t i = 0; i < v_data_all.size() / n_dim; ++i) {
            if (i % sample_rate == 0) {
                for (std::size_t j = 0; j < n_dim; ++j) {
                    v_data.push_back(v_data_all[i * n_dim + j]);
                }
            }
        }
        n_elem /= sample_rate;
    } else if (input_file.find(".el") != std::string::npos) {
        std::vector<float> v_data_all;
        count_lines_and_dimensions(input_file, n_elem, n_dim);
        std::cout << "Found " << n_elem << " elements" << " and " << n_dim << " dimensions" << std::endl;
        std::cout << "Reading el data.." << std::flush;
        if (!read_el(input_file, v_data, n_dim, sample_rate)) {
            std::cout << std::endl << "Input file could not be opened, does it exist ?" << std::endl;
            exit(-1);
        }
        n_elem /= sample_rate;
        std::cout << " done!" << std::endl;
    } else if (input_file.find(".h5") != std::string::npos) {

    } else {
        std::cerr << "input file format not supported" << std::endl;
        print_help();
        exit(-1);
    }
    std::cout << "Read " << v_data.size() << " floats" << std::endl;

    std::cout << "Writing " << n_elem << " elements.." << std::flush;
    if (output_file.find(".csv") != std::string::npos) {
        write_csv(output_file, v_data, n_elem, n_dim);
        std::cout << " done!" << std::endl;
    } else if (output_file.find(".bin") != std::string::npos) {
        write_bin(output_file, v_data, n_elem, n_dim);
        std::cout << " done!" << std::endl;
    } else if (output_file.find(".h5") != std::string::npos) {
        // TODO
    }

    std::cout << std::endl << "Data Transformation has been successfully completed" << std::endl;


    return 0;
}
