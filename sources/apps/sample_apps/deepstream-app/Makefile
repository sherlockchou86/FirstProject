################################################################################
# Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
################################################################################

APP:= deepstream-app

SRCS:= $(wildcard *.c)
SRCS+= $(wildcard ../../apps-common/src/*.c)

INCS:= $(wildcard *.h)

PKGS:= gstreamer-1.0 gstreamer-video-1.0 x11

OBJS:= $(SRCS:.c=.o)

CFLAGS:= -I../../apps-common/includes -I../../../includes

LIBS:= -lm -L/usr/local/deepstream -lnvdsgst_meta -lnvds_utils \
       -lgstrtspserver-1.0 -Wl,-rpath,/usr/local/deepstream

CFLAGS+= `pkg-config --cflags $(PKGS)`

LIBS+= `pkg-config --libs $(PKGS)`

all: $(APP)

.o: .c $(INCS) Makefile
	$(CC) -c $@ $(CFLAGS) $<

$(APP): $(OBJS) Makefile
	$(CC) -o $(APP) $(OBJS) $(LIBS)

clean:
	rm -rf $(OBJS) $(APP)
