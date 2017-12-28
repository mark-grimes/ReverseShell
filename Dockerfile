FROM scratch

ADD dockerSystemLibs /
ADD ReverseShellServer /usr/bin/

WORKDIR /data
ENTRYPOINT ["ReverseShellServer"]
