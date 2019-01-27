################################################################################
# Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#################################################################################

CXX:= gcc
SRCS:= gstnvmsgconv.c
INCS:= gstnvmsgconv.h
LIB:=libnvdsgst_msgconv.so

CFLAGS+= -fPIC \
	 -I../../includes \
	 -I../../libs/nvmsgconv/

LIBS := -shared -Wl,-no-undefined -ldl\
	-L/usr/local/deepstream/ -lnvdsgst_helper -lnvdsgst_meta -lnvds_msgconv \
	-Wl,-rpath,/usr/local/deepstream/

OBJS:= $(SRCS:.c=.o)

PKGS:= gstreamer-1.0 gstreamer-base-1.0
CFLAGS+= `pkg-config --cflags $(PKGS)`
LIBS+= `pkg-config --libs $(PKGS)`

all: $(LIB)

%.o: %.c $(INCS) Makefile
	@echo $(CFLAGS)
	$(CXX) -c -o $@ $(CFLAGS) $<

$(LIB): $(OBJS) $(DEP) Makefile
	$(CXX) -o $@ $(OBJS) $(LIBS)

install: $(LIB)
	cp -rv $(LIB) /usr/lib/x86_64-linux-gnu/gstreamer-1.0/

clean:
	rm -rf $(OBJS) $(LIB)
