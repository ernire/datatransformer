#include <iostream>
#include <vector>
#include <iterator>
#include <sstream>
#include <fstream>
#include <functional>

void count_lines_and_dimensions(const std::string &in_file, int &lines, int &dimensions) noexcept {
    std::ifstream is(in_file);
    std::string line, buf;
    int cnt = 0;
    dimensions = 0;
    while (std::getline(is, line)) {
        if (dimensions == 0) {
            std::istringstream iss(line);
            std::vector<std::string> results(std::istream_iterator<std::string>{iss},
                    std::istream_iterator<std::string>());
            dimensions = results.size();
        }
        ++cnt;
    }
    lines = cnt;
    is.close();
}

bool csv_to_bin(std::string const &in_file, std::string const &out_file, std::function<void(int, int)> const &meta_callback) {
    int n_dim = 0;
    int n_sample = 0;
    count_lines_and_dimensions(in_file, n_sample, n_dim);
    meta_callback(n_sample, n_dim);
    std::ifstream is(in_file, std::ios::in | std::ifstream::binary);
    std::ofstream os(out_file, std::ios::out | std::ofstream::binary);
    if (!is.is_open() || !os.is_open()) {
        return false;
    }

    os.write(reinterpret_cast<const char *>(&n_sample), sizeof(int));
    os.write(reinterpret_cast<const char *>(&n_dim), sizeof(int));
    is.clear();
    is.seekg(0, std::istream::beg);
    std::string line;
    float line_features[n_dim];
    int line_cnt = 0;
    int one_tenth = n_sample / 10;
    int percent = 0;
    while (std::getline(is, line)) {
        std::istringstream iss(line);
        // class
        for (int i = 0; i < n_dim; ++i) {
            iss >> line_features[i];
        }
        for (int i = 0; i < n_dim; ++i) {
            os.write(reinterpret_cast<const char *>(&line_features[i]), sizeof(float));
        }
        if (++line_cnt % one_tenth == 0) {
            percent += 10;
            std::cout << "Finished: " << percent << "%" << std::endl;
        }
    }
    os.flush();
    os.close();
    is.close();
    return true;
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

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Please specify an input file and an output file (in that order)" << std::endl;
    }
    std::string input_file(argv[1]);
    std::string output_file(argv[2]);
    std::cout << "Transforming " << input_file << " (" << get_file_size(input_file) << " bytes) into " << output_file << std::endl;
    if (input_file.find(".csv") != std::string::npos || output_file.find(".csv") != std::string::npos) {
        std::cout << "Note: using space as a delimiter for csv file" << std::endl;
    }

    if (input_file.find(".csv") != std::string::npos && output_file.find(".bin") != std::string::npos) {
        bool is_success = csv_to_bin(input_file, output_file, [](auto const n_sample, auto const n_dim) -> void {
            std::cout << "Number of elements/samples: " << n_sample << std::endl;
            std::cout << "Number of features/dimensions: " << n_dim << std::endl;
        });
        if (is_success) {
            std::cout << "File successfully transformed" << std::endl;
        } else {
            std::cout << "Error occurred, file has NOT been transformed" << std::endl;
        }
    }

    return 0;
}
