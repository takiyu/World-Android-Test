#include "world-example.h"
#include <sstream>

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fstream>

#include <stdint.h>
#include <sys/time.h>

// For .wav input/output functions.
#include "audioio.h"

// WORLD core functions.
// Note: win.sln uses an option in Additional Include Directories.
// To compile the program, the option "-I $(SolutionDir)..\src" was set.
#include "world/d4c.h"
#include "world/dio.h"
#include "world/matlabfunctions.h"
#include "world/cheaptrick.h"
#include "world/stonemask.h"

// Linux porting section: implement timeGetTime() by gettimeofday(),
#ifndef DWORD
#define DWORD uint32_t
#endif
DWORD timeGetTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    DWORD ret = static_cast<DWORD>(tv.tv_usec / 1000 + tv.tv_sec * 1000);
    return ret;
}

//-----------------------------------------------------------------------------
// struct for WORLD
// This struct is an option.
// Users are NOT forced to use this struct.
//-----------------------------------------------------------------------------
typedef struct {
    double frame_period;
    int fs;

    double *f0;
    double *time_axis;
    int f0_length;

    double **spectrogram;
    double **aperiodicity;
    int fft_size;
} WorldParameters;


namespace {

    /**
     *  Displaying wav file information
     *
     *   @param fs : the sampling frequence
     *   @param nbit : the number of bits to code the signal
     *   @param x_length : the number of samples
     */
    std::string DisplayInformation(int fs, int nbit, int x_length)
    {
        std::stringstream log_ss;
        log_ss << "File information" << std::endl;
        log_ss << "Sampling : " << fs << " Hz " << nbit << " Bit" << std::endl;
        log_ss << "Length " << x_length << " [sample]" << std::endl;
        log_ss << "Length " << static_cast<double>(x_length) / fs <<  "[sec]" << std::endl;
        return log_ss.str();
    }

    /**
     *  F0 estimation function
     *
     *    @param x : the signal samples
     *    @param x_length : the number of samples
     *    @param world_parameters : the world structure which is going to contains the F0 values (double format)
     *     in the f0 structure field
     */
    std::string F0Estimation(double *x, int x_length, WorldParameters *world_parameters)
    {
        std::stringstream log_ss;

        DioOption option = {0};
        InitializeDioOption(&option);

        // Modification of the option
        // When you You must set the same value.
        // If a different value is used, you may suffer a fatal error because of a
        // illegal memory access.
        option.frame_period = world_parameters->frame_period;

        // Valuable option.speed represents the ratio for downsampling.
        // The signal is downsampled to fs / speed Hz.
        // If you want to obtain the accurate result, speed should be set to 1.
        option.speed = 1;

        // You should not set option.f0_floor to under world::kFloorF0.
        // If you want to analyze such low F0 speech, please change world::kFloorF0.
        // Processing speed may sacrify, provided that the FFT length changes.
        option.f0_floor = 71.0;

        // You can give a positive real number as the threshold.
        // Most strict value is 0, but almost all results are counted as unvoiced.
        // The value from 0.02 to 0.2 would be reasonable.
        option.allowed_range = 0.1;

        // Parameters setting and memory allocation.
        world_parameters->f0_length = GetSamplesForDIO(world_parameters->fs,
                                                       x_length, world_parameters->frame_period);
        world_parameters->f0 = new double[world_parameters->f0_length];
        world_parameters->time_axis = new double[world_parameters->f0_length];
        double *refined_f0 = new double[world_parameters->f0_length];

        log_ss << std::endl << "Analysis" << std::endl;
        DWORD elapsed_time = timeGetTime();
        Dio(x, x_length, world_parameters->fs, &option, world_parameters->time_axis,
            world_parameters->f0);

        log_ss << "DIO: " << timeGetTime() - elapsed_time << " [msec]" << std::endl;

        // StoneMask is carried out to improve the estimation performance.
        elapsed_time = timeGetTime();
        StoneMask(x, x_length, world_parameters->fs, world_parameters->time_axis,
                  world_parameters->f0, world_parameters->f0_length, refined_f0);

        log_ss << "StoneMask: " << timeGetTime() - elapsed_time << " [msec]" << std::endl;

        for (int i = 0; i < world_parameters->f0_length; ++i)
            world_parameters->f0[i] = refined_f0[i];

        delete[] refined_f0;
        return log_ss.str();
    }

    /**
     *  Spectral envelope estimation function
     *
     *    @param x : the signal samples
     *    @param x_length : the number of samples
     *    @param world_parameters : the world structure which is going to contains the spectrogram (double values)
     *    in the spectrogram structure field
     */
    std::string SpectralEnvelopeEstimation(double *x, int x_length,
                                           WorldParameters *world_parameters)
    {
        std::stringstream log_ss;

        CheapTrickOption option;
        InitializeCheapTrickOption(world_parameters->fs, &option);

        // This value may be better one for HMM speech synthesis.
        // Default value is -0.09.
        option.q1 = -0.15;

        // Important notice (2016/02/02)
        // You can control a parameter used for the lowest F0 in speech.
        // You must not set the f0_floor to 0.
        // It will cause a fatal error because fft_size indicates the infinity.
        // You must not change the f0_floor after memory allocation.
        // You should check the fft_size before excucing the analysis/synthesis.
        // The default value (71.0) is strongly recommended.
        // On the other hand, setting the lowest F0 of speech is a good choice
        // to reduce the fft_size.
        option.f0_floor = 71.0;

        // Parameters setting and memory allocation.
        world_parameters->fft_size =
            GetFFTSizeForCheapTrick(world_parameters->fs, &option);
        world_parameters->spectrogram = new double *[world_parameters->f0_length];
        for (int i = 0; i < world_parameters->f0_length; ++i) {
            world_parameters->spectrogram[i] =
                new double[world_parameters->fft_size / 2 + 1];
        }

        DWORD elapsed_time = timeGetTime();
        CheapTrick(x, x_length, world_parameters->fs, world_parameters->time_axis,
                   world_parameters->f0, world_parameters->f0_length, &option,
                   world_parameters->spectrogram);

        log_ss << "CheapTrick: " << timeGetTime() - elapsed_time << " [msec]" << std::endl;
        return log_ss.str();
    }

    /**
     *  Aperiodicity envelope estimation function
     *
     *    @param x : the signal samples
     *    @param x_length : the number of samples
     *    @param world_parameters : the world structure which is going to contains the aperidicity (double values)
     *    in the aperiodicity structure field
     */
    std::string AperiodicityEstimation(double *x, int x_length,
                                       WorldParameters *world_parameters)
    {
        std::stringstream log_ss;

        D4COption option;
        InitializeD4COption(&option);

        // Parameters setting and memory allocation.
        world_parameters->aperiodicity = new double *[world_parameters->f0_length];
        for (int i = 0; i < world_parameters->f0_length; ++i) {
            world_parameters->aperiodicity[i] =
                new double[world_parameters->fft_size / 2 + 1];
        }

        DWORD elapsed_time = timeGetTime();
        // option is not implemented in this version. This is for future update.
        // We can use "NULL" as the argument.
        D4C(x, x_length, world_parameters->fs, world_parameters->time_axis,
            world_parameters->f0, world_parameters->f0_length,
            world_parameters->fft_size, &option, world_parameters->aperiodicity);

        log_ss << "D4C: " << timeGetTime() - elapsed_time << " [msec]" << std::endl;
        return log_ss.str();
    }

    void DestroyMemory(WorldParameters *world_parameters) {
        delete[] world_parameters->time_axis;
        delete[] world_parameters->f0;
        for (int i = 0; i < world_parameters->f0_length; ++i) {
            delete[] world_parameters->spectrogram[i];
            delete[] world_parameters->aperiodicity[i];
        }
        delete[] world_parameters->spectrogram;
        delete[] world_parameters->aperiodicity;
    }

}  // namespace


bool RunWorldExample(const std::string& wav_filepath,
                     const std::string& out_f0_filepath,
                     const std::string& out_spectrum_filepath,
                     const std::string& out_aperiodicity_filepath,
                     std::stringstream& log_ss) {
    log_ss << "Input wav: " << wav_filepath << std::endl;

    int x_length = GetAudioLength(wav_filepath.c_str());
    if (x_length <= 0) {
        log_ss << "Failed" << std::endl;
        log_ss << "(Please turn on storage permission)" << std::endl;
        return false;
    }
    double *x = new double[x_length];

    // wavread() must be called after GetAudioLength().
    int fs, nbit;
    wavread(wav_filepath.c_str(), &fs, &nbit, x);
    log_ss << DisplayInformation(fs, nbit, x_length);

    // 2016/02/02
    // A new struct is introduced to implement safe program.
    WorldParameters world_parameters;

    // You must set fs and frame_period before analysis/synthesis.
    world_parameters.fs = fs;

    // 5.0 ms is the default value.
    // Generally, the inverse of the lowest F0 of speech is the best.
    // However, the more elapsed time is required.
    world_parameters.frame_period = 5.0;


    //---------------------------------------------------------------------------
    // Analysis part
    //---------------------------------------------------------------------------

    // F0 estimation
    log_ss << F0Estimation(x, x_length, &world_parameters);

    // Spectral envelope estimation
    log_ss << SpectralEnvelopeEstimation(x, x_length, &world_parameters);

    // Aperiodicity estimation by D4C
    log_ss << AperiodicityEstimation(x, x_length, &world_parameters);

    log_ss << "fft size = " << world_parameters.fft_size << std::endl;


    //---------------------------------------------------------------------------
    // Saving part
    //---------------------------------------------------------------------------

    // F0 saving
    std::ofstream out_f0(out_f0_filepath.c_str(), std::ios::out | std::ios::binary);
    log_ss << "Save F0: " << out_f0_filepath << std::endl;
    if(!out_f0)
    {
        log_ss << "Cannot open file" << std::endl;
        return false;
    }

    out_f0.write(reinterpret_cast<const char*>(world_parameters.f0),
                 std::streamsize(world_parameters.f0_length * sizeof(double)));
    out_f0.close();

    // Spectrogram saving
    std::ofstream out_spectrogram(out_spectrum_filepath,  std::ios::out | std::ios::binary);
    log_ss << "Save spectrogram: " << out_spectrum_filepath << std::endl;
    if(!out_spectrogram)
    {
        log_ss << "Cannot open file" << std::endl;
        return false;
    }

    // write the sampling frequency
    out_spectrogram.write(reinterpret_cast<const char*>(&world_parameters.fs),
                 std::streamsize( sizeof(world_parameters.fs) ) );

    // write the sampling frequency
    out_spectrogram.write(reinterpret_cast<const char*>(&world_parameters.frame_period),
                 std::streamsize( sizeof(world_parameters.frame_period) ) );

    // write the spectrogram data
    for (int i=0; i<world_parameters.f0_length; i++)
    {
        out_spectrogram.write(reinterpret_cast<const char*>(world_parameters.spectrogram[i]),
                              std::streamsize((world_parameters.fft_size / 2 + 1) * sizeof(double)));
    }

    out_spectrogram.close();

    // Aperiodicity saving
    std::ofstream out_aperiodicity(out_aperiodicity_filepath,  std::ios::out | std::ios::binary);
    log_ss << "Save aperidicity: " << out_aperiodicity_filepath << std::endl;
    if(!out_aperiodicity)
    {
        log_ss << "Cannot open file" << std::endl;
        return false;
    }

    for (int i=0; i<world_parameters.f0_length; i++)
    {
        out_aperiodicity.write(reinterpret_cast<const char*>(world_parameters.aperiodicity[i]),
                               std::streamsize((world_parameters.fft_size / 2 + 1) * sizeof(double)));
    }

    out_aperiodicity.close();


    //---------------------------------------------------------------------------
    // Cleaning part
    //---------------------------------------------------------------------------
    delete[] x;
    DestroyMemory(&world_parameters);

    log_ss << "Complete !!" << std::endl;

    return true;
}
