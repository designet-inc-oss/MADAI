madai_LIBDIR=-L/lib64 -lpthread
madai_CFLAGS = -Wall
madai_LDFLAGS = -lmilter

bin_PROGRAMS = madai

madai_SOURCES = madai.c log.c thread_control.c milter.c config.c libdg/libdgconfig.c smtp.c

install-exec-local:install-madai-configfile

install-madai-configfile:
	sh ../copy_configfile.sh 
