# World-jni

Android JNI test of [World](https://github.com/mmorise/World).

This application execute the same thing as `World/examples/analysis_synthesis/analysis.cpp`.

## Build and Install (debug)
```
./gradlew build
./gradlew installArm8Debug
```

## Data Push
```
adb push world-test-data /sdcard/
```
