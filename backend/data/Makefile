gtree_build: gtree_build.cpp
	g++ -std=c++0x -O2 gtree_build.cpp -L/usr/local/lib/ -lmetis -o gtree_build

gtree_gen: gtree_build
	./gtree_build

GPTree_build: GPTree.cpp
	g++ -std=c++0x -O2 GPTree.cpp -L/usr/local/lib/ -lmetis -o GPTree_build

GPTree_gen: GPTree_build
	./GPTree_build

trans:
	python trans.py

clean:
	rm gtree_build
	rm GPTree_build
	rm cab.object
	rm road.cedge
	rm road.gtree
	rm road.minds
	rm road.paths
	rm GP_Tree.data

.PHONY : clean trans
