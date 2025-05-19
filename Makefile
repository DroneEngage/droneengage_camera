CXX=clang++
#CXX=g++
EXE=de_webrtc
BIN=bin
BUILD = build
SRC = src
OBJS = $(BUILD)/main.o \
       $(BUILD)/configFile.o \
	   $(BUILD)/localConfigFile.o \
	   $(BUILD)/helpers.o \
       $(BUILD)/util_rpi.o \
	   $(BUILD)/udpClient.o \
	   $(BUILD)/de_module.o \
	   $(BUILD)/webrtc_plugin.o \
	   $(BUILD)/getopt_cpp.o \
	   $(BUILD)/lodepng.o \
	   $(BUILD)/webrtc_source.o \
	   $(BUILD)/webrtc_video_dev_capturer.o \
	   $(BUILD)/webrtc_video_recorder.o \
	   $(BUILD)/webrtc_userMedia.o \
	   $(BUILD)/webrtc_peerConnectionManager.o \
	   $(BUILD)/webrtc_SetSessionDescriptionObserver.o \
	   $(BUILD)/webrtc_video_encoder_factory.o \
	   $(BUILD)/webrtc_capturerTrackSource.o \
	   $(BUILD)/webrtc_fakeAudioCaptureModule.o \
	   
	   

SRCS = ../$(SRC)/main.cpp \
       ../$(SRC)/de_common/configFile.cpp \
	   ../$(SRC)/de_common/localConfigFile.cpp \
	   ../$(SRC)/helpers/helpers.cpp \
       ../$(SRC)/helpers/util_rpi.cpp \
	   ../$(SRC)/de_common/udpClient.cpp \
	   ../$(SRC)/de_common/de_module.cpp \
	   ../$(SRC)/webrtc_plugin.cpp \
	   ../$(SRC)/helpers/getopt_cpp.cpp \
	   ../$(SRC)/3rdparty/LodePNG/lodepng.cpp \
	   ../$(SRC)/webrtc/webrtc_source.cpp \
	   ../$(SRC)/webrtc/webrtc_video_dev_capturer.cpp \
	   ../$(SRC)/webrtc/webrtc_video_recorder.cpp \
	   ../$(SRC)/webrtc/webrtc_userMedia.cpp \
	   ../$(SRC)/webrtc/webrtc_fakeAudioCaptureModule.cpp \
	   ../$(SRC)/webrtc/webrtc_capturerTrackSource.cpp \
	   ../$(SRC)/webrtc/webrtc_peerConnectionManager.cpp \
	   ../$(SRC)/webrtc/webrtc_SetSessionDescriptionObserver.cpp \
	   ../$(SRC)/webrtc/webrtc_video_encoder_factory.cpp \
	   
	   

# Include paths
INCLUDE := -I../src/ \
           -I../lib/webrtc-local/include \
		   -I../lib/webrtc-local/include/rtc_base \
		   -I../lib/webrtc-local/include/rtc_base/strings \
           -I../lib/webrtc-local/include/third_party \
           -I../lib/webrtc-local/include/third_party/libyuv \
           -I../lib/webrtc-local/include/third_party/libyuv/include \
           -I../lib/webrtc-local/include/third_party/abseil-cpp \
           -I../lib/webrtc-local/include/base/allocator/partition_allocator/src

LIBS = -lpthread    -fexceptions    -ljsoncpp     -ldl -lX11 -lexpat -ljpeg
LIBS_RELEASE = $(LIBS) -L../lib/webrtc-local/lib/Release/
LIBS_DEBUG = $(LIBS) -L./lib/webrtc-local/lib/Debug/libwebrtc.a 
CXXFLAGS =
CXXFLAGS_RELEASE= $(CXXFLAGS) -DRELEASE -s   -Werror=unused-variable -Werror=unused-result
CXXFLAGS_DEBUG= $(CXXFLAGS)  -DDEBUG -g



release: webrtc.so.release
	$(CXX)  -o $(BIN)/$(EXE).so  $(CXXFLAGS_RELEASE)     $(OBJS)  $(LIBS_RELEASE)  ; 
	@echo "building finished ..."; 
	@echo "DONE."

debug: webrtc.so.debug 
	$(CXX)  -o $(BIN)/$(EXE).so  $(CXXFLAGS_DEBUG)     $(OBJS)  $(LIBS_DEBUG)  ; 
	@echo "building finished ..."; 
	@echo "DONE."

webrtc.so.release: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXX)   -DWEBRTC_POSIX $(CXXFLAGS_RELEASE)  -c   $(SRCS)  $(INCLUDE)  ; \
	cd .. ; 
	@echo "compliling finished ..."

webrtc.so.debug: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXX)   -DWEBRTC_POSIX $(CXXFLAGS_DEBUG)  -c   $(SRCS)  $(INCLUDE)  ; \
	cd .. ; 
	@echo "compiling finished ..."

copy: clean
	mkdir -p $(BIN); \
	cp config.*.json $(BIN);
	cp ./src/media_recorder/scripts/script*.sh $(BIN);  
	@echo "copying finished ..."
	

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BIN) $(BUILD)
	@echo "Cleaning finished."

.PHONY: all release debug copy clean