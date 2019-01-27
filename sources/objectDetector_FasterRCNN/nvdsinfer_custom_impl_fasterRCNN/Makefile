###############################################################################
#
# Copyright (c) 2018 NVIDIA CORPORATION.  All Rights Reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#
###############################################################################

CC:= g++

CFLAGS:= -Wall -std=c++11 -shared -fPIC
CFLAGS+= -I../../includes

LIBS:= -lnvinfer -lnvinfer_plugin
LFLAGS:= -Wl,--start-group $(LIBS) -Wl,--end-group

SRCFILES:= nvdsparsebbox_fasterRCNN.cpp nvdsiplugin_fasterRCNN.cpp \
           nvdsinitinputlayers_fasterRCNN.cpp
TARGET_LIB:= libnvdsinfer_custom_impl_fasterRCNN.so

all: $(TARGET_LIB)

$(TARGET_LIB) : $(SRCFILES)
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

clean:
	rm -rf $(TARGET_LIB)
