all: mresource
mresource: mresource.c
test: mresource
	./mrtest.sh
clean:
	\rm -f mresource
