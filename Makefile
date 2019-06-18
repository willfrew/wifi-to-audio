build:
	gcc ./main.c ./depends/radiotap/radiotap.c -o main -lpcap

clean:
	$(RM) main
