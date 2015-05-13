all: mresource
mresource: mresource.c
	gcc -o $@ $^ -Os
test: mresource
	./mrtest.sh
clean:
	\rm -f mresource
