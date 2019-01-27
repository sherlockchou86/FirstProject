################################################################################
# Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#################################################################################

CUDA_VER:=10.0
CXX:= g++
SRCS:= gstdsexample.cpp
INCS:= $(wildcard *.h)
LIB:=libgstnvdsexample.so

DEP:=dsexample_lib/libdsexample.a
DEP_FILES:=$(wildcard dsexample_lib/dsexample_lib.* )
DEP_FILES-=$(DEP)

CFLAGS+= -fPIC \
	 -I /usr/local/cuda-$(CUDA_VER)/include \
	 -I ../../includes

LIBS := -shared -Wl,-no-undefined \
	-L dsexample_lib -ldsexample \
	-L/usr/local/cuda-$(CUDA_VER)/lib64/ -lcudart \
	-lnppc -lnppig -lnpps -lnppicc -lnppidei \
	-L/usr/local/deepstream/ -lnvdsgst_helper -lnvdsgst_meta \
	-Wl,-rpath,/usr/local/deepstream/

OBJS:= $(SRCS:.cpp=.o)

PKGS:= gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0 opencv
CFLAGS+=$(shell pkg-config --cflags $(PKGS))
LIBS+=$(shell pkg-config --libs $(PKGS))

all: $(LIB)

%.o: %.cpp $(INCS) Makefile
	@echo $(CFLAGS)
	$(CXX) -c -o $@ $(CFLAGS) $<

$(LIB): $(OBJS) $(DEP) Makefile
	@echo $(CFLAGS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

$(DEP): $(DEP_FILES)
	$(MAKE) -C dsexample_lib/

install: $(LIB)
	cp -rv $(LIB) /usr/lib/x86_64-linux-gnu/gstreamer-1.0/

clean:
	rm -rf $(OBJS) $(LIB)
