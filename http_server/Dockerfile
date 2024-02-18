FROM perl:latest
RUN apt-get update && apt-get upgrade -y
RUN curl -L https://cpanmin.us | perl - -M https://cpan.metacpan.org -n Mojolicious
RUN cpanm Time::Moment

WORKDIR /usr/share/pmsrv
CMD [ "morbo", "./pmsrv.pl" ]
