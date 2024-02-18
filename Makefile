d-build:
	docker build ./ -t kos:1.1.1.40u20.04
	docker build ./http_server -t perl-mojolicious:0.1

network:
	docker network rm -f kosnet
	docker network create --subnet=172.16.0.0/16 kosnet

kos:
	docker run --volume="`pwd`:/data" --name traffic-light-kos -w /data --user user --net kosnet --ip 172.16.0.2 -it --rm kos:1.1.1.40u20.04 bash

http:
	docker run -p "3000:3000" --volume="`pwd`/http_server/script:/usr/share/pmsrv" -dit --net kosnet --ip 172.16.0.100 --rm --name central-server perl-mojolicious

develop: network http kos
	docker stop central-server
	docker network rm -f kosnet

build:
	./cross-build.sh

sim:
	./cross-build-sim.sh

clean:
	rm -rf build