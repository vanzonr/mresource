all: mresource
mresource: mresource.c
	gcc -o $@ $^ -O1 -s
test: mresource
	./mrtest.sh
clean:
	\rm -f mresource
