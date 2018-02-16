#ifndef WORLD_EXAMPLE_180216
#define WORLD_EXAMPLE_180216

#include <string>
#include <sstream>

bool RunWorldExample(const std::string& wav_filepath,
                     const std::string& out_f0_filepath,
                     const std::string& out_spectrum_filepath,
                     const std::string& out_aperiodicity_filepath,
                     std::stringstream& log_ss);


#endif
