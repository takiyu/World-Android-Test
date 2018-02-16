#include <jni.h>

#include "./world-example.h"

#include <string>
#include <sstream>

extern "C" {
JNIEXPORT jstring JNICALL
Java_com_example_hellojni_HelloJni_stringFromJNI( JNIEnv* env,
                                                  jobject thiz );
};

JNIEXPORT jstring JNICALL
Java_com_example_hellojni_HelloJni_stringFromJNI( JNIEnv* env,
                                                  jobject thiz ) {

    const std::string DATA_DIR = "/sdcard/world-test-data";

    const std::string WAV_FILEPATH = DATA_DIR + "/vaiueo2d.wav";
    const std::string F0_FILEPATH = DATA_DIR + "/out_f0.dat";
    const std::string SPEC_FILEPATH = DATA_DIR + "/out_spec.dat";
    const std::string APER_FILEPATH = DATA_DIR + "/out_aper.dat";

    // Execute example
    std::stringstream log_ss;
    bool ret = RunWorldExample(WAV_FILEPATH, F0_FILEPATH, SPEC_FILEPATH,
                               APER_FILEPATH, log_ss);

    // Create output string
    std::stringstream ss;
    ss << "World example result: ";
    if (ret) {
        ss << "Success";
    } else {
        ss << "Failure";
    }
    ss << std::endl;

    ss << std::endl;
    ss << "message:" << std::endl;
    ss << log_ss.str();

    return env->NewStringUTF(ss.str().c_str());
}
