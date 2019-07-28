build:
	gcc ./main.c ./depends/radiotap/radiotap.c ./alsa.c -o main -lpcap -lasound

build_pulse:
	gcc ./pulse_test.c -o pulse_test -lpulse -lm

clean:
	$(RM) main
