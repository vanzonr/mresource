all: mresource
mresource: mresource.c
	gcc -o $@ $^ -Os -s
test: mresource
	./mrtest.sh
clean:
	\rm -f mresource
