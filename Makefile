d-build:
	docker build ./ -t kos:1.1.1.40u20.04
	docker build ./http_server -t perl-mojolicious:0.1

develop:
	docker run --volume="`pwd`:/data" --name traffic-light-kos --user user -it --rm kos:1.1.1.40u20.04 bash

http:
	docker run -p "3000:3000" --volume="`pwd`/http_server/script:/usr/share/pmsrv" -dit --rm --name central-server perl-mojolicious

dev-app: http develop
	docker stop central-server

build:
	./cross-build.sh

sim:
	./cross-build-sim.sh

clean:
	rm -rf build